/*
 * FLASH_FastProgram (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * Erase the user area, then fast-program it row by row (32 double words =
 * 256 bytes per row; the flash routine runs from RAM with interrupts off),
 * read back and check.
 *   LED2 on:                        data correct
 *   LED2 blinking (1 s period):     data wrong after programming
 *   LED2 blinking (100 ms / 2 s):   erase or program operation failed
 *
 * Difference to ST's example: ST mass-erases the whole bank and runs the
 * complete example from SRAM. Here the program stays in flash, so the
 * user area (pages 32..63, upper 64 KB) is page-erased instead;
 * flash_mass_erase() exists for code that really runs from RAM.
 */
#include "g4_example.h"
#include "libstefi/flash.h"

#define USER_FIRST_PAGE 32u
#define USER_LAST_PAGE  63u
#define USER_START      flash_page_addr(USER_FIRST_PAGE)
#define USER_END        (flash_page_addr(USER_LAST_PAGE) + FLASH_PAGE_SIZE)

static uint64_t row[FLASH_ROW_SIZE / 8u];   /* source must be in RAM */

static uint64_t pattern(uint32_t addr)
{
    return ((uint64_t) ~addr << 32) | addr;  // address-dependent: catches row mix-ups
}

void main(void)
{
    board_init();

    uint32_t bad_page;
    if (!flash_erase_pages(USER_FIRST_PAGE, USER_LAST_PAGE - USER_FIRST_PAGE + 1u, &bad_page)) {
        example_error_flash();
    }
    for (uint32_t addr = USER_START; addr < USER_END; addr += FLASH_ROW_SIZE) {
        for (uint32_t i = 0; i < FLASH_ROW_SIZE / 8u; i++) row[i] = pattern(addr + 8u * i);
        if (!flash_fast_program_row(addr, row)) example_error_flash();
    }
    for (uint32_t addr = USER_START; addr < USER_END; addr += 8u) {
        if (*(volatile const uint64_t *) addr != pattern(addr)) example_error();
    }
    example_success();
}
