/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"
#include "tusb.h"

#include "class/msc/msc_device.h"
#include "class/msc/msc.h"

#include "platform/platform.h"

/*
 * RP2040 USB Mass Storage device backed by onboard QSPI flash.
 *
 * Assumptions:
 *   - Raspberry Pi Pico with 2 MiB flash
 *   - TinyUSB device stack integrated with Pico SDK
 *   - Last 256 KiB reserved exclusively for this disk
 *
 * IMPORTANT: We must exclude the disk region from the firmware
 * linker layout and firmware uploads.
 */

#define FLASH_BYTES       (2u * 1024u * 1024u)
#define DISK_BYTES        (256u * 1024u)
#define DISK_OFFSET       (FLASH_BYTES - DISK_BYTES)
#define BLOCK_SIZE        512u
#define BLOCK_COUNT       (DISK_BYTES / BLOCK_SIZE)

#define ERASE_SIZE        FLASH_SECTOR_SIZE
#define PAGE_SIZE         FLASH_PAGE_SIZE

_Static_assert(DISK_OFFSET % ERASE_SIZE == 0,
               "Disk offset must be erase aligned");
_Static_assert(DISK_BYTES % ERASE_SIZE == 0,
               "Disk size must be erase aligned");

/* Shared scratch buffers; MSC callbacks run synchronously. */
static uint8_t erase_buf[ERASE_SIZE] __attribute__((aligned(4)));
static uint8_t block_buf[BLOCK_SIZE] __attribute__((aligned(4)));

static bool disk_ejected;

/* ---------- Flash backend ---------- */

static bool disk_read_block(uint32_t lba, void *dst)
{
    if (lba >= BLOCK_COUNT || dst == NULL)
        return false;

    const uint8_t *src = (const uint8_t *)
        (XIP_BASE + DISK_OFFSET + lba * BLOCK_SIZE);

    memcpy(dst, src, BLOCK_SIZE);
    return true;
}

typedef struct {
    uint32_t offset;
    const uint8_t *data;
} flash_write_args_t;

static void flash_write_callback(void *param)
{
    const flash_write_args_t *a = param;

    uint32_t base = a->offset & ~(ERASE_SIZE - 1u);

    /* Preserve other logical blocks in the erase sector. */
    memcpy(erase_buf, (const void *)(XIP_BASE + base),
           ERASE_SIZE);

    memcpy(erase_buf + (a->offset - base),
           a->data, BLOCK_SIZE);

    flash_range_erase(base, ERASE_SIZE);

    for (uint32_t p = 0; p < ERASE_SIZE; p += PAGE_SIZE) {
        flash_range_program(base + p, erase_buf + p, PAGE_SIZE);
    }
}

static bool disk_write_block(uint32_t lba, const void *src)
{
    if (lba >= BLOCK_COUNT || src == NULL)
        return false;

    /*
     * Copy the source first: it may alias block_buf, and the
     * flash-safe callback uses its own erase_buf.
     */
    memcpy(block_buf, src, BLOCK_SIZE);

    flash_write_args_t args = {
        .offset = DISK_OFFSET + lba * BLOCK_SIZE,
        .data = block_buf
    };

    return flash_safe_execute(flash_write_callback, &args,
                              UINT32_MAX) == PICO_OK;
}

/* ---------- USB MSC callbacks ---------- */

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void)lun;
    return !disk_ejected;
}

void tud_msc_capacity_cb(uint8_t lun,
                         uint32_t *block_count,
                         uint16_t *block_size)
{
    (void)lun;
    *block_count = BLOCK_COUNT;
    *block_size = BLOCK_SIZE;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void)lun;
    return true;
}

bool tud_msc_start_stop_cb(uint8_t lun,
                           uint8_t power_condition,
                           bool start,
                           bool load_eject)
{
    (void)lun;
    (void)power_condition;

    if (load_eject)
        disk_ejected = !start;

    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun,
                          uint32_t lba,
                          uint32_t offset,
                          void *buffer,
                          uint32_t bufsize)
{
    (void)lun;

    if (disk_ejected || buffer == NULL ||
        offset >= BLOCK_SIZE || lba >= BLOCK_COUNT ||
        (uint64_t)lba * BLOCK_SIZE + offset + bufsize > DISK_BYTES)
        return -1;

    uint8_t *out = buffer;
    uint32_t done = 0;

    while (done < bufsize) {
        uint32_t pos = offset + done;
        uint32_t current_lba = lba + pos / BLOCK_SIZE;
        uint32_t in_block = pos % BLOCK_SIZE;
        uint32_t n = BLOCK_SIZE - in_block;

        if (n > bufsize - done)
            n = bufsize - done;

        if (!disk_read_block(current_lba, block_buf))
            return -1;

        memcpy(out + done, block_buf + in_block, n);
        done += n;
    }

    return (int32_t)done;
}

int32_t tud_msc_write10_cb(uint8_t lun,
                           uint32_t lba,
                           uint32_t offset,
                           uint8_t *buffer,
                           uint32_t bufsize)
{
    (void)lun;

    if (disk_ejected || buffer == NULL ||
        offset >= BLOCK_SIZE || lba >= BLOCK_COUNT ||
        (uint64_t)lba * BLOCK_SIZE + offset + bufsize > DISK_BYTES)
        return -1;

    uint32_t done = 0;

    while (done < bufsize) {
        uint32_t pos = offset + done;
        uint32_t current_lba = lba + pos / BLOCK_SIZE;
        uint32_t in_block = pos % BLOCK_SIZE;
        uint32_t n = BLOCK_SIZE - in_block;

        if (n > bufsize - done)
            n = bufsize - done;

        if (in_block == 0 && n == BLOCK_SIZE) {
            if (!disk_write_block(current_lba, buffer + done))
                return -1;
        } else {
            if (!disk_read_block(current_lba, block_buf))
                return -1;

            memcpy(block_buf + in_block, buffer + done, n);

            if (!disk_write_block(current_lba, block_buf))
                return -1;
        }

        done += n;
    }

    return (int32_t)done;
}

int32_t tud_msc_scsi_cb(uint8_t lun,
                        uint8_t const scsi_cmd[16],
                        void *buffer,
                        uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;

    /* SYNCHRONIZE CACHE(10): writes are synchronous. */
    if (scsi_cmd[0] == 0x35)
        return 0;

    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST,
                      0x20, 0x00);
    return -1;
}

void tud_msc_inquiry_cb(uint8_t lun,
                        uint8_t vendor_id[8],
                        uint8_t product_id[16],
                        uint8_t product_rev[4])
{
    (void) lun;

    memcpy(vendor_id, "nfiop  ", 8);
    memcpy(product_id, "MSC config mode ", 16);
    memcpy(product_rev, "1.0 ", 4);
}

void start_msc_config_mode(void)
{
    tusb_init();

    while (1) {
        tud_task();
    }
}
