#pragma once
/* Host-test stub: shadows the real libstefi header (see tests/host/run.sh). */
#include <stdint.h>
#include <stdbool.h>
typedef void (*usb_cdc_rx_callback_t)(const uint8_t *data, uint32_t len, void *aux);
void usb_cdc_init(const void *cfg);
void usb_cdc_register_rx_handler(usb_cdc_rx_callback_t fn, void *aux);
int32_t usb_cdc_write(const void *buf, uint32_t len);
bool usb_cdc_dtr(void);
