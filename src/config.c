/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"

#include "qcbor/qcbor_encode.h"
#include "qcbor/qcbor_decode.h"
#include "qcbor/qcbor_spiffy_decode.h"
#include "qcbor/UsefulBuf.h"
#include "cobs.h"

#include "platform/platform.h"

/*
 * Frame format:
 *
 *   CBOR payload | CRC32 little-endian
 *          -> COBS encoding -> 0x00 delimiter
 *
 * CRC-32/ISO-HDLC:
 *   Polynomial: 0xEDB88320 (reflected)
 *   Init:       0xFFFFFFFF
 *   XOR out:    0xFFFFFFFF
 */

#define MAX_CBOR             128
#define CRC_SIZE             4
#define MAX_RAW_FRAME        (MAX_CBOR + CRC_SIZE)
#define MAX_FRAME            (MAX_RAW_FRAME + 2)
#define RX_FRAME_TIMEOUT_MS  100

static uint8_t rx_frame[MAX_FRAME];
static size_t rx_len;
static bool rx_discard_until_delimiter;
static bool rx_have_partial_frame;
static uint32_t rx_last_byte_us;


/* =========================================================
 * CRC-32/ISO-HDLC
 * ========================================================= */

static uint32_t crc32_compute(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (unsigned bit = 0; bit < 8; bit++) {
            if (crc & 1u)
                crc = (crc >> 1) ^ 0xEDB88320u;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFFu;
}

static void crc32_write_le(uint8_t *dst, uint32_t crc)
{
    dst[0] = (uint8_t)(crc);
    dst[1] = (uint8_t)(crc >> 8);
    dst[2] = (uint8_t)(crc >> 16);
    dst[3] = (uint8_t)(crc >> 24);
}

static uint32_t crc32_read_le(const uint8_t *src)
{
    return ((uint32_t)src[0])       |
           ((uint32_t)src[1] << 8)  |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}


/* =========================================================
 * Response transmission
 * ========================================================= */
static void send_response(int64_t seq,
                          const char *status,
                          const char *result)
{
    uint8_t cbor[MAX_CBOR];
    uint8_t raw[MAX_RAW_FRAME];
    uint8_t frame[MAX_FRAME];
    size_t raw_len;
    uint32_t crc;
    QCBOREncodeContext ctx;
    UsefulBufC encoded;

    QCBOREncode_Init(
        &ctx,
        UsefulBuf_FROM_BYTE_ARRAY(cbor)
    );

    QCBOREncode_OpenMap(&ctx);
    QCBOREncode_AddInt64ToMapSZ(&ctx, "seq", seq);
    QCBOREncode_AddSZStringToMapSZ(&ctx, "status", status);
    QCBOREncode_AddSZStringToMapSZ(&ctx, "result", result);
    QCBOREncode_CloseMap(&ctx);


    if (QCBOREncode_Finish(&ctx, &encoded) != QCBOR_SUCCESS)
        return;

    if (encoded.len > MAX_CBOR)
        return;

    /* Append CRC after the CBOR payload. */
    memcpy(raw, encoded.ptr, encoded.len);

    crc = crc32_compute(raw, encoded.len);
    crc32_write_le(raw + encoded.len, crc);

    raw_len = encoded.len + CRC_SIZE;

    /* COBS-encode payload plus CRC. Reserve one byte for delimiter. */
    cobs_encode_result cr = cobs_encode(
        frame,
        sizeof(frame) - 1,
        raw,
        raw_len
    );

    if (cr.status != COBS_ENCODE_OK)
        return;

    frame[cr.out_len] = 0x00;

    config_uart_send_bytes(frame, cr.out_len + 1);
}


/* =========================================================
 * Command dispatcher
 * ========================================================= */
static bool execute_command(const char *cmd,
                            int64_t arg,
                            const char **result)
{
    if (strcmp(cmd, "ping") == 0) {
        *result = "pong";
        return true;
    }

    if (strcmp(cmd, "led_set") == 0) {
        if (arg != 0 && arg != 1) {
            *result = "invalid_arg";
            return false;
        }

        /* gpio_put(LED_PIN, arg != 0); */

        *result = arg ? "led_on" : "led_off";
        return true;
    }

    *result = "unknown_command";
    return false;
}


/* =========================================================
 * CBOR packet processing
 * Called only after CRC verification.
 * ========================================================= */
static void process_packet(const uint8_t *data, size_t len)
{
    QCBORDecodeContext ctx;

    int64_t seq = -1;
    int64_t arg = 0;
    UsefulBufC cmd_buf = NULLUsefulBufC;
    char cmd[32];
    const char *result = "error";
    bool ok;

    QCBORDecode_Init(
        &ctx,
        (UsefulBufC){data, len},
        QCBOR_DECODE_MODE_NORMAL
    );    

    QCBORDecode_EnterMap(&ctx, NULL);
    QCBORDecode_GetInt64InMapSZ(&ctx, "seq", &seq);
    QCBORDecode_GetTextStringInMapSZ(&ctx, "cmd", &cmd_buf);
    QCBORDecode_GetInt64InMapSZ(&ctx, "arg", &arg);
    QCBORDecode_ExitMap(&ctx);

    if (QCBORDecode_Finish(&ctx) != QCBOR_SUCCESS ||
        seq < 0 ||
        cmd_buf.ptr == NULL) {
        return;
    }

    if (cmd_buf.len == 0 || cmd_buf.len >= sizeof(cmd)) {
        send_response(seq, "NACK", "invalid_cmd");
        return;
    }

    memcpy(cmd, cmd_buf.ptr, cmd_buf.len);
    cmd[cmd_buf.len] = '\0';

    ok = execute_command(cmd, arg, &result);

    send_response(seq, ok ? "ACK" : "NACK", result);
}


/* =========================================================
 * RX recovery
 * ========================================================= */

/*
 * Call when the UART driver reports lost/corrupted RX data.
 * Must run in the same serialized context as rx_byte(), or
 * shared state must be protected against concurrent access.
 */
static void rx_uart_error(void)
{
    rx_len = 0;
    rx_have_partial_frame = false;
    rx_discard_until_delimiter = true;
}

/*
 * Call periodically from the main loop or a periodic task.
 */
static void rx_poll_timeout(void)
{
    uint32_t now = time_us_32();

    if (rx_have_partial_frame &&
        (uint32_t)(now - rx_last_byte_us) >
            RX_FRAME_TIMEOUT_MS * 1000u) {

        rx_len = 0;
        rx_have_partial_frame = false;
        rx_discard_until_delimiter = true;
    }
}


/* =========================================================
 * Receive one byte
 * ========================================================= */

static void rx_byte(uint8_t b)
{
    uint8_t raw[MAX_RAW_FRAME];

    if (b == 0x00) {
        /*
         * A delimiter re-establishes frame synchronization.
         */
        if (rx_discard_until_delimiter) {
            rx_discard_until_delimiter = false;
            rx_len = 0;
            rx_have_partial_frame = false;
            return;
        }

        if (rx_len > 0) {
            cobs_decode_result cr = cobs_decode(
                raw,
                sizeof(raw),
                rx_frame,
                rx_len
            );

            if (cr.status == COBS_DECODE_OK &&
                cr.out_len >= CRC_SIZE) {

                size_t payload_len = cr.out_len - CRC_SIZE;

                uint32_t received_crc =
                    crc32_read_le(raw + payload_len);

                uint32_t calculated_crc =
                    crc32_compute(raw, payload_len);

                /*
                 * Never pass a packet to QCBOR or execute a
                 * command unless its CRC is correct.
                 */
                if (received_crc == calculated_crc &&
                    payload_len <= MAX_CBOR) {
                    process_packet(raw, payload_len);
                }
                /* CRC mismatch: silently discard the frame. */
            }
            /* Invalid COBS: discard the frame. */
        }

        rx_len = 0;
        rx_have_partial_frame = false;
        return;
    }

    /*
     * After overrun, timeout, or overflow, ignore bytes until
     * the next frame delimiter.
     */
    if (rx_discard_until_delimiter)
        return;

    rx_last_byte_us = time_us_32();
    rx_have_partial_frame = true;

    if (rx_len >= sizeof(rx_frame)) {
        rx_len = 0;
        rx_have_partial_frame = false;
        rx_discard_until_delimiter = true;
        return;
    }

    rx_frame[rx_len++] = b;
}

int config_main(void)
{
    uint8_t ch;
    int ret;

    start_config_mode(rx_poll_timeout);

    while (true) {
        rx_poll_timeout();
        ret = config_uart_read_non_blocking(&ch, 1);
        if (ret < 1) {
            if (ret != 0) {
                rx_uart_error();
            }

            continue;
        }
        rx_byte(ch);
    }
}