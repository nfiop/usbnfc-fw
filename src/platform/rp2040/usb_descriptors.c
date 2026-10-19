
#include "tusb.h"
#include <stdint.h>

// Select this before tusb_init() on core 1.
typedef enum {
    USB_MODE_CDC,
    USB_MODE_VENDOR
} usb_mode_t;

static usb_mode_t usb_mode = USB_MODE_CDC;

enum {
    ITF_CDC       = 0,
    ITF_CDC_DATA  = 1,
    ITF_VENDOR    = 0,
    ITF_TOTAL     = 2
};

#define EP_CDC_NOTIF  0x81
#define EP_CDC_OUT    0x02
#define EP_CDC_IN     0x82

#define EP_VENDOR_OUT 0x01
#define EP_VENDOR_IN  0x81

#define CDC_CONFIG_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define VENDOR_CONFIG_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

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

static const uint8_t vendor_config[] = {
    TUD_CONFIG_DESCRIPTOR(
        1, 1, 0, VENDOR_CONFIG_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100
    ),
    TUD_VENDOR_DESCRIPTOR(
        ITF_VENDOR, 4, EP_VENDOR_OUT, EP_VENDOR_IN, 64
    )
};

static const tusb_desc_device_t device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe, // Replace with your assigned VID
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&device_desc;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    if (index != 0)
        return NULL;

    return (usb_mode == USB_MODE_CDC)
        ? cdc_config
        : vendor_config;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index,
                                         uint16_t langid)
{
    (void)langid;

    static uint16_t desc[32];
    const char *str;

    switch (index) {
    case 0:
        desc[1] = 0x0409;
        desc[0] = (TUSB_DESC_STRING << 8) | 4;
        return desc;
    case 1: str = "NAND flash I/O platform"; break;
    case 2: str = "USB NAND flash adapter (UART)"; break;
    case 3: str = "000001"; break;
    case 4: str = (usb_mode == USB_MODE_CDC)
                    ? "CDC Interface" : "Vendor Interface";
            break;
    default:
        return NULL;
    }

    unsigned n = 0;
    while (str[n] && n < 31) {
        desc[n + 1] = (uint8_t)str[n];
        ++n;
    }

    desc[0] = (TUSB_DESC_STRING << 8) | (2 * n + 2);
    return desc;
}
