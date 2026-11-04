/*
 * CRC_UserDefinedPolynomial: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * 8-bit CRC, user polynomial 0x9B, of the word 0x1234 -> 0xEF.
 * LED2 on: correct; blinking: error.
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

static const uint32_t data = 0x1234;

void main() {
    board_init();
    const crc_config_t cfg = {.polynomial = 0x9B, .width = 8, .init = 0xFFFFFFFFu};
    crc_configure(&cfg);
    if (crc_feed_words(&data, 1) != 0xEF) led_error_forever();
    led_success_forever();
}
