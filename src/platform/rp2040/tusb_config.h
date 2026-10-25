
#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// Common configuration
//--------------------------------------------------------------------

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_MCU OPT_MCU_RP2040
#define CFG_TUSB_OS OPT_OS_PICO
#define CFG_TUSB_DEBUG 0

// Native RP2040 USB controller, device mode
#define CFG_TUD_ENABLED 1
#define CFG_TUH_ENABLED 0

#define CFG_TUD_MSC_EP_BUFSIZE 512

#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

// USB memory alignment
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

//--------------------------------------------------------------------
// Device configuration
//--------------------------------------------------------------------

// Control endpoint
#define CFG_TUD_ENDPOINT0_SIZE 64

//--------------------------------------------------------------------
// USB class drivers
//--------------------------------------------------------------------

// Enable both at build time.
// Descriptors select which personality is exposed at boot.

#define CFG_TUD_CDC 1
#define CFG_TUD_VENDOR 1
#define CFG_TUD_MSC 1

// Explicitly disable unused classes
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_AUDIO 0
#define CFG_TUD_VIDEO 0
#define CFG_TUD_BTH 0
#define CFG_TUD_ECM_RNDIS 0
#define CFG_TUD_NCM 0
#define CFG_TUD_DFU_RUNTIME 0

//--------------------------------------------------------------------
// CDC ACM configuration
//--------------------------------------------------------------------

// Software FIFOs
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256

// Bulk endpoint transfer buffers (full-speed USB)
#define CFG_TUD_CDC_RX_EPSIZE 64
#define CFG_TUD_CDC_TX_EPSIZE 64

// CDC notification endpoint support
#define CFG_TUD_CDC_NOTIFY 1

//--------------------------------------------------------------------
// Vendor-specific interface configuration
//--------------------------------------------------------------------

// Software FIFOs
#define CFG_TUD_VENDOR_RX_BUFSIZE 512
#define CFG_TUD_VENDOR_TX_BUFSIZE 512

// Bulk endpoint transfer buffers
#define CFG_TUD_VENDOR_RX_EPSIZE 64
#define CFG_TUD_VENDOR_TX_EPSIZE 64

#ifdef __cplusplus
}
#endif

#endif // TUSB_CONFIG_H_
