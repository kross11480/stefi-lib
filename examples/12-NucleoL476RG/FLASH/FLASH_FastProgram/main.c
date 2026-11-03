/* FLASH_FastProgram (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * Mass-erase bank 2 (pages 256..511, 0x08080000) while running from
 * bank 1, then fast-program it row by row (32 double words = 256 bytes;
 * the routine runs from RAM with interrupts off), read back and check.
 *   LD2 on: data correct, blinking 1 s: wrong data,
 *   blinking 100 ms / 2 s: erase or program failed */
#include "l4_example.h"
#include "libstefi/flash.h"

#define START (flash_page_addr(256))
#define END   (flash_page_addr(512))

static uint64_t row[FLASH_ROW_SIZE / 8u];   /* source in RAM */

static uint64_t pattern(uint32_t a) { return ((uint64_t) ~a << 32) | a; }

_Noreturn static void error_flash(void)
{
    for (;;) { led_on(LED2); systick_delay_ms(100); led_off(LED2); systick_delay_ms(2000); }
}

void main(void)
{
    board_init();
    if (!flash_mass_erase_bank(2)) error_flash();
    for (uint32_t a = START; a < END; a += FLASH_ROW_SIZE) {
        for (uint32_t i = 0; i < FLASH_ROW_SIZE / 8u; i++) row[i] = pattern(a + 8u * i);
        if (!flash_fast_program_row(a, row)) error_flash();
    }
    for (uint32_t a = START; a < END; a += 8u) if (*(volatile const uint64_t *) a != pattern(a)) example_error();
    example_success();
}
