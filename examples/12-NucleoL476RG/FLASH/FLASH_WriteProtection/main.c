/* FLASH_WriteProtection (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * Built three times: _enable (WRITE_PROTECTION_ENABLE), _disable
 * (WRITE_PROTECTION_DISABLE), _program (FLASH_PAGE_PROGRAM only).
 *
 * Enable/disable write protection (bank 1 area A) on the user pages via
 * the option bytes (applied by a reset), then erase/program/check them:
 *   LD2 on:                       data written (expected: unprotected)
 *   LD2 blinking (1 s period):    write refused (expected: protected)
 *   LD2 blinking (100 ms / 2 s):  failed on an unprotected area / option error
 * Difference: user pages 64..68 (ST: 24..28) so they never hold code. */
#include "l4_example.h"
#include "libstefi/flash.h"

#define FIRST_PAGE 64u
#define LAST_PAGE  68u
#define DATA_32    0x12345678u
#define DATA_64    0x1234567812345678ull

_Noreturn static void error_flash(void)
{
    for (;;) { led_on(LED2); systick_delay_ms(100); led_off(LED2); systick_delay_ms(2000); }
}

static bool area_protected(void)
{
    uint32_t s, e;
    flash_get_write_protection(FLASH_WRP_AREA_A, &s, &e);
    return s <= e && s <= FIRST_PAGE && e >= LAST_PAGE;
}

void main(void)
{
    board_init();
#if defined(WRITE_PROTECTION_ENABLE)
    if (!area_protected()) { flash_set_write_protection(FLASH_WRP_AREA_A, FIRST_PAGE, LAST_PAGE); error_flash(); }
#elif defined(WRITE_PROTECTION_DISABLE)
    if (area_protected()) { flash_set_write_protection(FLASH_WRP_AREA_A, 0xFF, 0); error_flash(); }
#endif
    bool prot = area_protected();
    uint32_t bad;
    bool ok = flash_erase_pages(FIRST_PAGE, LAST_PAGE - FIRST_PAGE + 1u, &bad);
    const uint64_t v = DATA_64;
    const uint32_t start = flash_page_addr(FIRST_PAGE), end = flash_page_addr(LAST_PAGE + 1u);
    for (uint32_t a = start; ok && a < end; a += 8u) ok = flash_write(a, &v, 8u);
    if (!ok) { if (prot) example_error(); error_flash(); }
    for (uint32_t a = start; a < end; a += 4u) if (*(volatile const uint32_t *) a != DATA_32) example_error();
    example_success();
}
