/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

#include "tusb.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"

#define CDC_QUEUE_SIZE 1024

static queue_t rx_queue;  // USB host -> application
static queue_t tx_queue;  // application -> USB host

static volatile bool usb_mounted = false;

/* Called by TinyUSB when the host sends CDC data.
 * TinyUSB callbacks execute on core 1 in this design.
 */
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;

    uint8_t buf[64];

    while (tud_cdc_available()) {
        uint32_t n = tud_cdc_read(buf, sizeof(buf));

        for (uint32_t i = 0; i < n; ++i) {
            // Drop bytes if the application RX queue is full.
            (void)queue_try_add(&rx_queue, &buf[i]);
        }
    }
}

void tud_mount_cb(void)
{
    usb_mounted = true;
}

void tud_umount_cb(void)
{
    usb_mounted = false;
}

bool cdc_connected(void)
{
    return usb_mounted;
}

int config_uart_send_bytes(const uint8_t *data, size_t len)
{
    int n = 0;

    while (n < len && queue_try_add(&tx_queue, &data[n]))
        ++n;

    return n;    
}

int config_uart_read_non_blocking(uint8_t *data, size_t max_len)
{
    size_t n = 0;

    while (n < max_len && queue_try_remove(&rx_queue, &data[n]))
        ++n;

    return n;
}

static void config_mode_usb_worker(void)
{
    uint8_t buf[64];
    uint32_t room;
    uint32_t n;
    uint32_t written;

    for (;;) {
        tud_task();

        // Move queued application bytes into TinyUSB's TX FIFO.
        if (tud_mounted()) {
            while (tud_cdc_write_available() > 0) {
                room = tud_cdc_write_available();
                if (room > sizeof(buf))
                    room = sizeof(buf);

                n = 0;
                while (n < room && queue_try_remove(&tx_queue, &buf[n]))
                    ++n;

                if (n == 0)
                    break;

                written = tud_cdc_write(buf, n);

                // Requeue any bytes TinyUSB couldn't accept.
                // This is normally unnecessary when room was checked,
                // but handle it defensively.
                for (uint32_t i = written; i < n; ++i)
                    (void)queue_try_add(&tx_queue, &buf[i]);
            }

            tud_cdc_write_flush();
        }

        tight_loop_contents();
    }
}

void start_config_mode(void)
{
    queue_init(&rx_queue, sizeof(uint8_t), CDC_QUEUE_SIZE);
    queue_init(&tx_queue, sizeof(uint8_t), CDC_QUEUE_SIZE);

    tusb_init();
    
    multicore_launch_core1(config_mode_usb_worker);
}
