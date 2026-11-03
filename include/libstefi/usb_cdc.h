#pragma once

#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* USB CDC-ACM device ("virtual COM port") Hardware Abstraction layer */
/*                                                                    */
/* Bare-metal full-speed device on the OTG_FS core of the STM32F7 and */
/* STM32L4 (PA11 = D-, PA12 = D+; the user USB connector CN13 of the  */
/* NUCLEO-144; wired by hand on a NUCLEO-64). One CDC-ACM function:   */
/* EP1 OUT/IN bulk data (64 B), EP2 IN interrupt (notifications,      */
/* never sent). No vendor middleware.                                 */
/*                                                                    */
/* Call system_init() first (F7: provides the 48 MHz clock; L4: the   */
/* driver then starts MSI at 48 MHz, trimmed by the 32.768 kHz LSE).   */
/* Not on the G4: its USB device block is a different peripheral;     */
/* usb_cdc_init() returns without doing anything there.               */
/**********************************************************************/

typedef struct {
    uint16_t vid;
    uint16_t pid;
    const char *manufacturer;  /* ASCII, <= 30 chars (longer is cut) */
    const char *product;
    const char *serial;
} usb_cdc_config_t;

/** Called from the USB interrupt with the bytes of one OUT packet (<= 64). */
typedef void (*usb_cdc_rx_callback_t)(const uint8_t *data, uint32_t len, void *aux);
/** Called from the USB interrupt on SET_CONTROL_LINE_STATE (port open/close). */
typedef void (*usb_cdc_line_state_callback_t)(bool dtr, bool rts, void *aux);

/**
 * @brief Bring up the OTG_FS core as a CDC-ACM device and connect (D+ pull-up).
 * @param cfg  descriptor identity; NULL = defaults (0x0483:0x5740, the ID the
 *             ST "Virtual ComPort" uses, so existing host setups keep working)
 */
void usb_cdc_init(const usb_cdc_config_t *cfg);

void usb_cdc_register_rx_handler(usb_cdc_rx_callback_t fn, void *aux);
void usb_cdc_register_line_state_handler(usb_cdc_line_state_callback_t fn, void *aux);

/** Host has selected the configuration (enumeration finished). */
bool usb_cdc_is_configured(void);
/** Host has the port open (DTR asserted by the terminal / pyserial). */
bool usb_cdc_dtr(void);

/**
 * @brief Blocking write on the bulk IN endpoint.
 *
 * Splits into 64-byte packets, appends a zero-length packet when the data
 * ends on a packet boundary. Must not be called from interrupt context.
 *
 * @return number of bytes sent, or -1 if not configured / aborted (USB reset,
 *         cable pulled, or the host dropped DTR while the write was blocked)
 */
int32_t usb_cdc_write(const void *buf, uint32_t len);
