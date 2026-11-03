/*
 * FLASH_WriteProtection (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * Built three times, like ST's compile switches:
 *   g4_flash_wrp_enable   WRITE_PROTECTION_ENABLE:  protect the user area
 *   g4_flash_wrp_disable  WRITE_PROTECTION_DISABLE: remove the protection
 *   g4_flash_wrp_program  FLASH_PAGE_PROGRAM only:  just erase/program/check
 * Changing the option bytes needs a reset (the driver launches it); after
 * it, the program finds the protection as requested and goes on to the
 * erase/program/check of the user area:
 *   LED2 on:                        data correct (expected when unprotected)
 *   LED2 blinking (1 s period):     area protected, data not written
 *                                   (expected after WRITE_PROTECTION_ENABLE)
 *   LED2 blinking (100 ms / 2 s):   operation failed on an unprotected area,
 *                                   or the option bytes could not be changed
 *
 * Difference to ST's example: user area = pages 32..63 (ST: 4..63), as the
 * stefi-lib image is larger than 8 KB. Area A is used for the protection.
 */
#include "g4_example.h"
#include "libstefi/flash.h"

#define USER_FIRST_PAGE 32u
#define USER_LAST_PAGE  63u
#define USER_START      flash_page_addr(USER_FIRST_PAGE)
#define USER_END        (flash_page_addr(USER_LAST_PAGE) + FLASH_PAGE_SIZE)
#define DATA_32         0x12345678u
#define DATA_64         0x1234567812345678ull

static bool area_protected(void)
{
    uint32_t start, end;
    flash_get_write_protection(FLASH_WRP_AREA_A, &start, &end);
    return start <= USER_FIRST_PAGE && end >= USER_LAST_PAGE && start <= end;
}

void main(void)
{
    board_init();

#if defined(WRITE_PROTECTION_ENABLE)
    if (!area_protected()) {
        flash_set_write_protection(FLASH_WRP_AREA_A, USER_FIRST_PAGE, USER_LAST_PAGE);
        example_error_flash();      // only reached if the option change failed
    }
#elif defined(WRITE_PROTECTION_DISABLE)
    if (area_protected()) {
        flash_set_write_protection(FLASH_WRP_AREA_A, 0x7Fu, 0u);   // start > end: off
        example_error_flash();
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
        if (protected) example_error();   // refused, as it should be
        example_error_flash();            // failed although unprotected
    }

    for (uint32_t addr = USER_START; addr < USER_END; addr += 4u) {
        if (*(volatile const uint32_t *) addr != DATA_32) example_error();
    }
    example_success();
}
