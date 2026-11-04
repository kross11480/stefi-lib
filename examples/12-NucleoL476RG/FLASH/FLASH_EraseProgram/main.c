/*
 * FLASH_EraseProgram: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Erase pages 32..255, program, verify.
 * LED2 on: ok; blinking 1 s: wrong data; 100 ms / 2 s: flash error.
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

#define FIRST_PAGE 32u
#define LAST_PAGE  255u
#define DATA_32    0x12345678u
#define DATA_64    0x1234567812345678ull

void main() {
    board_init();
    uint32_t bad;
    if (!flash_erase_pages(FIRST_PAGE, LAST_PAGE - FIRST_PAGE + 1u, &bad)) led_flash_forever();
    const uint64_t v = DATA_64;
    const uint32_t start = flash_page_addr(FIRST_PAGE), end = flash_page_addr(LAST_PAGE + 1u);
    for (uint32_t a = start; a < end; a += 8u) if (!flash_write(a, &v, 8u)) led_flash_forever();
    for (uint32_t a = start; a < end; a += 4u) if (*(volatile const uint32_t *) a != DATA_32) led_error_forever();
    led_success_forever();
}
