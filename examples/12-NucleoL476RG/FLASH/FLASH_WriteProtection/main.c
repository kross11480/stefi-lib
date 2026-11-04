/*
 * FLASH_WriteProtection: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Build variants _enable / _disable / _program: write protection of pages
 * 64..68 (option bytes, reset), then try to program them.
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

#define FIRST_PAGE 64u
#define LAST_PAGE  68u
#define DATA_32    0x12345678u
#define DATA_64    0x1234567812345678ull

static bool area_protected(void) {
    uint32_t s, e;
    flash_get_write_protection(FLASH_WRP_AREA_A, &s, &e);
    return s <= e && s <= FIRST_PAGE && e >= LAST_PAGE;
}

void main() {
    board_init();
#if defined(WRITE_PROTECTION_ENABLE)
    if (!area_protected()) { flash_set_write_protection(FLASH_WRP_AREA_A, FIRST_PAGE, LAST_PAGE); led_flash_forever(); }
#elif defined(WRITE_PROTECTION_DISABLE)
    if (area_protected()) { flash_set_write_protection(FLASH_WRP_AREA_A, 0xFF, 0); led_flash_forever(); }
#endif
    bool prot = area_protected();
    uint32_t bad;
    bool ok = flash_erase_pages(FIRST_PAGE, LAST_PAGE - FIRST_PAGE + 1u, &bad);
    const uint64_t v = DATA_64;
    const uint32_t start = flash_page_addr(FIRST_PAGE), end = flash_page_addr(LAST_PAGE + 1u);
    for (uint32_t a = start; ok && a < end; a += 8u) ok = flash_write(a, &v, 8u);
    if (!ok) { if (prot) led_error_forever(); led_flash_forever(); }
    for (uint32_t a = start; a < end; a += 4u) if (*(volatile const uint32_t *) a != DATA_32) led_error_forever();
    led_success_forever();
}
