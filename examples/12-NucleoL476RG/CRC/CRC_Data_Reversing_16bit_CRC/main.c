/*
 * CRC_Data_Reversing_16bit_CRC: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * 16-bit CRC-CCITT with input/output bit reversal, init 0x5ABE; ST's test
 * vectors. LED2 on: correct; blinking: error.
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

static const uint8_t data[9]       = {0x4D, 0x3C, 0x2B, 0x1A, 0xBE, 0x71, 0xC9, 0x8A, 0x5E};
static const uint8_t data_check[9] = {0x58, 0xD4, 0x3C, 0xB2, 0x51, 0x93, 0x8E, 0x7D, 0x7A};
#define EXPECTED 0x5043u

void main() {
    board_init();

    crc_config_t cfg = {.polynomial = 0x1021, .width = 16, .init = 0x5ABE,
                        .rev_in = CRC_REV_IN_WORD, .rev_out = true};
    crc_configure(&cfg);
    if (crc_feed_bytes(data, sizeof data) != EXPECTED) led_error_forever();

    cfg.rev_in = CRC_REV_IN_NONE;
    cfg.rev_out = false;
    crc_configure(&cfg);
    uint32_t want = 0;
    for (int i = 0; i < 16; i++) if (EXPECTED & (1u << i)) want |= 1u << (15 - i);
    if (crc_feed_bytes(data_check, sizeof data_check) != want) led_error_forever();
    led_success_forever();
}
