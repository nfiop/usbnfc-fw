/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "tusb.h"

#include "platform/platform.h"
#include "common/types.h"

// Select this before tusb_init() on core 1.
usb_mode_t g_usb_mode = USB_MODE_MSC;

enum {
    ITF_MSC       = 0,
    ITF_CDC       = 0,
    ITF_CDC_DATA  = 1,
    ITF_VENDOR    = 0,
    ITF_TOTAL     = 2
};

#define EP_CDC_NOTIF  0x81
#define EP_CDC_OUT    0x02
#define EP_CDC_IN     0x82

#define CDC_CONFIG_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define VENDOR_CONFIG_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

#define MSC_CONFIG_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

#define NAND_VENDOR_SPECIFIC_EP_OUT        0x01
#define NAND_VENDOR_SPECIFIC_EP_IN         0x81
#define NAND_VENDOR_SPECIFIC_USB_EP_SIZE   64

static const uint8_t cdc_config[] = {
    TUD_CONFIG_DESCRIPTOR(
        1, ITF_TOTAL, 0, CDC_CONFIG_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100
    ),
    TUD_CDC_DESCRIPTOR(
        ITF_CDC, 4, EP_CDC_NOTIF, 8,
        EP_CDC_OUT, EP_CDC_IN, 64
    )
};

static const uint8_t msc_config[] = {
    TUD_CONFIG_DESCRIPTOR(
        1, ITF_TOTAL, 0, MSC_CONFIG_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100
    ),
    TUD_MSC_DESCRIPTOR(
        ITF_MSC, 4, 0x01, 0x81, 64
    )
};

static const uint8_t vendor_config[] = {
    TUD_CONFIG_DESCRIPTOR(
        1,                  // configuration number
        1,                  // interface count
        0,                  // configuration string
        VENDOR_CONFIG_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
        100                 // max power, in mA
    ),

    TUD_VENDOR_DESCRIPTOR(
        0,                  // interface number
        4,                  // interface string index
        NAND_VENDOR_SPECIFIC_EP_OUT,
        NAND_VENDOR_SPECIFIC_EP_IN,
        NAND_VENDOR_SPECIFIC_USB_EP_SIZE
    ),
};

static const tusb_desc_device_t msc_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x08,
    .bDeviceSubClass    = 0x06,
    .bDeviceProtocol    = 0x50,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

static const tusb_desc_device_t cdc_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x02,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

static tusb_desc_device_t const raw_nfc_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0x01, // Raw NAND interface
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

static tusb_desc_device_t const spi_nfc_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0x02, // SPI interface
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    if (g_usb_mode ==  USB_MODE_MSC)
        return (uint8_t const*)&msc_device_desc;
    if (g_usb_mode ==  USB_MODE_CDC)
        return (uint8_t const *)&cdc_device_desc;
    if (g_usb_mode ==  USB_MODE_VENDOR_SPECIFIC_RAW_NFC)
        (uint8_t const *)&raw_nfc_device_desc;
    if (g_usb_mode ==  USB_MODE_VENDOR_SPECIFIC_SPI_NFC)
        (uint8_t const *)&spi_nfc_device_desc;

    ASSERT_NOT_REACHED("Invalid usb_mode");
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    if (index != 0)
        return NULL;

    if (g_usb_mode == USB_MODE_CDC)
        return cdc_config;
    if (g_usb_mode == USB_MODE_MSC)
        return msc_config;
    return vendor_config;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index,
                                         uint16_t langid)
{
    (void)langid;
    unsigned n;
    static uint16_t desc[32];
    const char *str;

    switch (index) {
    case 0:
        /* Language is US English */
        desc[1] = 0x0409;
        desc[0] = (TUSB_DESC_STRING << 8) | 4;
        return desc;
    case 1: 
        str = "NAND flash I/O platform"; 
        break;
    case 2:
        if (g_usb_mode ==  USB_MODE_CDC)
            str = "USB NAND flash adapter (UART)";
        else if (g_usb_mode ==  USB_MODE_MSC)
            str =  "USB NAND flash adapter (MSC)";
        else
            str = "USB NAND flash adapter"; 
        break;
    case 3: 
        str = "000001"; 
        break;
    case 4: 
        if (g_usb_mode ==  USB_MODE_CDC)
            str = "CDC Interface";
        else if (g_usb_mode ==  USB_MODE_MSC)
            str = "Flash storage";
        else
            str = "Vendor Interface";
        break;
    default:
        return NULL;
    }

    n = 0;
    while (str[n] && n < 31) {
        desc[n + 1] = (uint8_t)str[n];
        ++n;
    }

    desc[0] = (TUSB_DESC_STRING << 8) | (2 * n + 2);
    return desc;
}
