/*
 * FLASH_WriteProtection: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * Build variants _enable / _disable / _program: set or clear the write
 * protection of pages 32..63 (option bytes, reset), then try to program them.
 * LED2 on: written; blinking 1 s: refused (protected); 100 ms / 2 s: error.
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

static bool area_protected(void) {
    uint32_t start, end;
    flash_get_write_protection(FLASH_WRP_AREA_A, &start, &end);
    return start <= USER_FIRST_PAGE && end >= USER_LAST_PAGE && start <= end;
}

void main() {
    board_init();

#if defined(WRITE_PROTECTION_ENABLE)
    if (!area_protected()) {
        flash_set_write_protection(FLASH_WRP_AREA_A, USER_FIRST_PAGE, USER_LAST_PAGE);
        led_flash_forever();      // only reached if the option change failed
    }
#elif defined(WRITE_PROTECTION_DISABLE)
    if (area_protected()) {
        flash_set_write_protection(FLASH_WRP_AREA_A, 0x7Fu, 0u);   // start > end: off
        led_flash_forever();
    }
#endif

    bool protected = area_protected();
    uint32_t bad_page;
    bool ok = flash_erase_pages(USER_FIRST_PAGE, USER_LAST_PAGE - USER_FIRST_PAGE + 1u, &bad_page);
    const uint64_t pattern = DATA_64;
    for (uint32_t addr = USER_START; ok && addr < USER_END; addr += 8u) {
        ok = flash_write(addr, &pattern, 8u);
    }
    if (!ok) {
        if (protected) led_error_forever();   // refused, as it should be
        led_flash_forever();            // failed although unprotected
    }

    for (uint32_t addr = USER_START; addr < USER_END; addr += 4u) {
        if (*(volatile const uint32_t *) addr != DATA_32) led_error_forever();
    }
    led_success_forever();
}
