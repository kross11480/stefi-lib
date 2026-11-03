/*
 * FLASH_EraseProgram (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * Erase the user area page by page, program it with a double-word
 * pattern, read it back and check it.
 *   LED2 on:                        data correct
 *   LED2 blinking (1 s period):     data wrong after programming
 *   LED2 blinking (100 ms / 2 s):   erase or program operation failed
 *
 * Difference to ST's example: the user area starts at page 32 (64 KB)
 * instead of page 4 — a stefi-lib image with printf is larger than 8 KB.
 */
#include "g4_example.h"
#include "libstefi/flash.h"

#define USER_FIRST_PAGE 32u
#define USER_LAST_PAGE  63u
#define USER_START      flash_page_addr(USER_FIRST_PAGE)
#define USER_END        (flash_page_addr(USER_LAST_PAGE) + FLASH_PAGE_SIZE)
#define DATA_32         0x12345678u
#define DATA_64         0x1234567812345678ull

void main(void)
{
    board_init();

    uint32_t bad_page;
    if (!flash_erase_pages(USER_FIRST_PAGE, USER_LAST_PAGE - USER_FIRST_PAGE + 1u, &bad_page)) {
        example_error_flash();
    }
    const uint64_t pattern = DATA_64;
    for (uint32_t addr = USER_START; addr < USER_END; addr += 8u) {
        if (!flash_write(addr, &pattern, 8u)) example_error_flash();
    }
    for (uint32_t addr = USER_START; addr < USER_END; addr += 4u) {
        if (*(volatile const uint32_t *) addr != DATA_32) example_error();
    }
    example_success();
}
