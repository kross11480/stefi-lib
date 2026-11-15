/*
 * CRC_Example: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * CRC-32 of 114 words, checked against crc_sw(). LED2 on: correct; blinking: error.
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

#define BUFFER_SIZE 114u

static uint32_t data[BUFFER_SIZE];

void main() {
    board_init();
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) data[i] = 0x00001021u * (i + 1u) ^ (i << 20);

    const crc_config_t cfg = CRC_CONFIG_DEFAULT;
    crc_configure(&cfg);
    uint32_t crc = crc_feed_words(data, BUFFER_SIZE);
    if (crc != crc_sw(&cfg, cfg.init, data, BUFFER_SIZE, 32)) led_error_forever();
    led_success_forever();
}
