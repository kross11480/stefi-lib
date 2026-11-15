/*
 * FLASH_FastProgram: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Mass-erase bank 2 while running from bank 1, fast-program it row by row,
 * verify. LED2 on: ok; blinking 1 s: wrong data; 100 ms / 2 s: flash error.
 */

#include "board.h"
#include "led.h"

#include <libstefi/flash.h>
#include <libstefi/systick.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* LED2 blinking 100 ms on / 2 s off: a flash operation failed */
static void led_flash_forever() {
    while(1) {
        led_on(LED2);
        systick_delay_ms(100);
        led_off(LED2);
        systick_delay_ms(2000);
    }
}

/* LED2 on: passed */
static void led_success_forever() {
    led_on(LED2);
    while(1);
}

#define START (flash_page_addr(256))
#define END   (flash_page_addr(512))

static uint64_t row[FLASH_ROW_SIZE / 8u];   /* source in RAM */

static uint64_t pattern(uint32_t a) { return ((uint64_t) ~a << 32) | a; }

void main() {
    board_init();
    if (!flash_mass_erase_bank(2)) led_flash_forever();
    for (uint32_t a = START; a < END; a += FLASH_ROW_SIZE) {
        for (uint32_t i = 0; i < FLASH_ROW_SIZE / 8u; i++) row[i] = pattern(a + 8u * i);
        if (!flash_fast_program_row(a, row)) led_flash_forever();
    }
    for (uint32_t a = START; a < END; a += 8u) if (*(volatile const uint64_t *) a != pattern(a)) led_error_forever();
    led_success_forever();
}
