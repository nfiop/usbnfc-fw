#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "platform/platform.h"

#include "pico/stdlib.h"
#include "qcbor/qcbor_encode.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"
#include "cobs.h"

#define MAX_CBOR 128
#define MAX_FRAME 132

static uint8_t rx_frame[MAX_FRAME];
static size_t rx_len;
static bool rx_overflow;

static void send_response(int64_t seq, const char *status,
                          const char *result)
{
    uint8_t cbor[MAX_CBOR];
    uint8_t frame[MAX_FRAME];

    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, UsefulBuf_FROM_BYTE_ARRAY(cbor));

    QCBOREncode_OpenMap(&ctx);
    QCBOREncode_AddInt64ToMapSZ(&ctx, "seq", seq);
    QCBOREncode_AddSZStringToMapSZ(&ctx, "status", status);
    QCBOREncode_AddSZStringToMapSZ(&ctx, "result", result);
    QCBOREncode_CloseMap(&ctx);

    UsefulBufC encoded;
    if (QCBOREncode_Finish(&ctx, &encoded) != QCBOR_SUCCESS)
        return;

    cobs_encode_result cr =
        cobs_encode(frame, sizeof(frame) - 1,
                    encoded.ptr, encoded.len);

    if (cr.status != COBS_ENCODE_OK)
        return;

    frame[cr.out_len] = 0;
    config_uart_send_bytes(frame, cr.out_len + 1);
}

// Replace this with your actual command dispatcher.
static bool execute_command(const char *cmd, int64_t arg,
                            const char **result)
{
    if (strcmp(cmd, "ping") == 0) {
        *result = "pong";
        return true;
    }

    if (strcmp(cmd, "led_set") == 0) {
        // Stub: arg should be 0 or 1.
        if (arg != 0 && arg != 1) {
            *result = "invalid_arg";
            return false;
        }

        // TODO: gpio_put(LED_PIN, arg != 0);
        *result = arg ? "led_on" : "led_off";
        return true;
    }

    *result = "unknown_command";
    return false;
}

static void process_packet(const uint8_t *data, size_t len)
{
    QCBORDecodeContext ctx;
    int64_t seq = -1;
    int64_t arg = 0;
    UsefulBufC cmd_buf = NULLUsefulBufC;

    QCBORDecode_Init(&ctx, (UsefulBufC){data, len},
                     QCBOR_DECODE_MODE_NORMAL);

    QCBORDecode_EnterMap(&ctx, NULL);
    QCBORDecode_GetInt64InMapSZ(&ctx, "seq", &seq);
    QCBORDecode_GetTextStringInMapSZ(&ctx, "cmd", &cmd_buf);
    QCBORDecode_GetInt64InMapSZ(&ctx, "arg", &arg);
    QCBORDecode_ExitMap(&ctx);

    if (QCBORDecode_Finish(&ctx) != QCBOR_SUCCESS ||
        seq < 0 || cmd_buf.ptr == NULL) {
        // Malformed request; seq may not be recoverable.
        return;
    }

    // Copy command into a bounded, null-terminated buffer.
    char cmd[32];
    if (cmd_buf.len == 0 || cmd_buf.len >= sizeof(cmd)) {
        send_response(seq, "NACK", "invalid_cmd");
        return;
    }
    memcpy(cmd, cmd_buf.ptr, cmd_buf.len);
    cmd[cmd_buf.len] = '\0';

    const char *result = "error";
    bool ok = execute_command(cmd, arg, &result);

    send_response(seq, ok ? "ACK" : "NACK", result);
}

static void rx_byte(uint8_t b)
{
    if (b != 0) {
        if (rx_len < sizeof(rx_frame)) {
            if (!rx_overflow)
                rx_frame[rx_len++] = b;
        } else {
            rx_overflow = true;
        }
        return;
    }

    if (!rx_overflow && rx_len > 0) {
        uint8_t decoded[MAX_FRAME];
        cobs_decode_result cr =
            cobs_decode(decoded, sizeof(decoded), rx_frame, rx_len);

        if (cr.status == COBS_DECODE_OK)
            process_packet(decoded, cr.out_len);
    }

    rx_len = 0;
    rx_overflow = false;
}

int config_main(void)
{
    uint8_t ch;
    int ret;

    start_config_mode();

    while (true) {
        ret = config_uart_read_non_blocking(&ch, 1);
        if (ret < 1)
            continue;
        rx_byte(ch);
    }
}