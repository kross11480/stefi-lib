/* FLASH_EraseProgram (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * Erase the user area of bank 1 page by page, program it with a
 * double-word pattern, read back and check.
 *   LD2 on:                        data correct
 *   LD2 blinking (1 s period):     data wrong after programming
 *   LD2 blinking (100 ms / 2 s):   erase or program operation failed
 * Difference: user area = pages 32..255 (ST: 16..255) — a stefi image with
 * printf is larger than the 32 KB below page 16. */
#include "l4_example.h"
#include "libstefi/flash.h"

#define FIRST_PAGE 32u
#define LAST_PAGE  255u
#define DATA_32    0x12345678u
#define DATA_64    0x1234567812345678ull

_Noreturn static void error_flash(void)
{
    for (;;) { led_on(LED2); systick_delay_ms(100); led_off(LED2); systick_delay_ms(2000); }
}

void main(void)
{
    board_init();
    uint32_t bad;
    if (!flash_erase_pages(FIRST_PAGE, LAST_PAGE - FIRST_PAGE + 1u, &bad)) error_flash();
    const uint64_t v = DATA_64;
    const uint32_t start = flash_page_addr(FIRST_PAGE), end = flash_page_addr(LAST_PAGE + 1u);
    for (uint32_t a = start; a < end; a += 8u) if (!flash_write(a, &v, 8u)) error_flash();
    for (uint32_t a = start; a < end; a += 4u) if (*(volatile const uint32_t *) a != DATA_32) example_error();
    example_success();
}
