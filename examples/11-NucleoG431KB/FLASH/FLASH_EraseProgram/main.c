/*
 * FLASH_EraseProgram: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * Erase pages 32..63, program them double word by double word, verify.
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

#define USER_FIRST_PAGE 32u
#define USER_LAST_PAGE  63u
#define USER_START      flash_page_addr(USER_FIRST_PAGE)
#define USER_END        (flash_page_addr(USER_LAST_PAGE) + FLASH_PAGE_SIZE)
#define DATA_32         0x12345678u
#define DATA_64         0x1234567812345678ull

void main() {
    board_init();

    uint32_t bad_page;
    if (!flash_erase_pages(USER_FIRST_PAGE, USER_LAST_PAGE - USER_FIRST_PAGE + 1u, &bad_page)) {
        led_flash_forever();
    }
    const uint64_t pattern = DATA_64;
    for (uint32_t addr = USER_START; addr < USER_END; addr += 8u) {
        if (!flash_write(addr, &pattern, 8u)) led_flash_forever();
    }
    for (uint32_t addr = USER_START; addr < USER_END; addr += 4u) {
        if (*(volatile const uint32_t *) addr != DATA_32) led_error_forever();
    }
    led_success_forever();
}
