/*
 * CRC_Bytes_Stream_7bit_CRC: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * 7-bit CRC (poly 0x65) over byte streams, continued and restarted;
 * ST's test vectors. LED2 on: all correct; blinking: error.
 */

#include "board.h"
#include "led.h"

#include <libstefi/crc.h>
#include <libstefi/systick.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* LED2 on: passed */
static void led_success_forever() {
    led_on(LED2);
    while(1);
}

static const uint8_t test5[5]   = {0x12, 0x34, 0xBA, 0x71, 0xAD};
static const uint8_t test17[17] = {0x12, 0x34, 0xBA, 0x71, 0xAD, 0x11, 0x56, 0xDC, 0x88,
                                   0x1B, 0xEE, 0x4D, 0x82, 0x93, 0xA6, 0x7F, 0xC3};
static const uint8_t test1[1]   = {0x19};
static const uint8_t test2[2]   = {0xAB, 0xCD};

void main() {
    board_init();
    const crc_config_t cfg = {.polynomial = 0x65, .width = 7, .init = 0xFFFFFFFFu};
    crc_configure(&cfg);

    if (crc_feed_bytes(test5, sizeof test5) != 0x57) led_error_forever();
    if (crc_feed_bytes(test17, sizeof test17) != 0x6E) led_error_forever();
    if (crc_feed_bytes(test1, sizeof test1) != 0x4B) led_error_forever();
    crc_reset();
    if (crc_feed_bytes(test2, sizeof test2) != 0x26) led_error_forever();
    led_success_forever();
}
