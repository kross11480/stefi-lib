#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Internal-flash erase/program for the STM32G431 (single bank, 2 KB pages).
 *
 * Flash is memory-mapped for reads: read back simply by dereferencing the
 * address (e.g. flash_page_addr(page)). Writes require the page to be erased
 * first, and are programmed a double word (8 bytes) at a time, so addr and len
 * passed to flash_write() must both be 8-byte aligned.
 *
 * Unlock/lock are handled internally by erase/write. Erasing or programming a
 * page that holds executing code will hang/fault — keep app data pages clear of
 * the .text region (the map uses the last page; see map_storage in the app).
 */

#define FLASH_PAGE_SIZE 2048u
#define FLASH_NUM_PAGES 64u             /* G431KB: 128 KB / 2 KB (see flash_num_pages()) */
#define FLASH_MEM_BASE  0x08000000u

/* Absolute address of a page (page 0..FLASH_NUM_PAGES-1). */
static inline uint32_t flash_page_addr(uint32_t page) {
    return FLASH_MEM_BASE + page * FLASH_PAGE_SIZE;
}

/* Pages of this MCU: 64 on the G431KB, 512 (2 banks x 256) on the L476RG.
 * Pages are numbered across banks: L4 page 256 = bank 2 page 0 = 0x08080000. */
uint32_t flash_num_pages(void);

/* Erase one page. Returns false on a bad page index or a flash error. */
bool flash_erase_page(uint32_t page);

/*
 * Program len bytes from data to flash address addr. addr and len must be
 * multiples of 8 (double-word programming). The target must already be erased.
 * Returns false on misalignment or a flash error.
 */
bool flash_write(uint32_t addr, const void *data, uint32_t len);


/* Erase pages first .. first + count - 1. On failure *bad_page (if given)
 * names the page that failed. */
bool flash_erase_pages(uint32_t first, uint32_t count, uint32_t *bad_page);

/*
 * Mass erase of the whole bank. Runs from RAM with interrupts off; erases
 * the running program too — only useful for code that lives in RAM
 * entirely, or right before reflashing. Returns after completion.
 */
bool flash_mass_erase(void);
/* L4 dual bank: erase one bank (1 or 2) — bank 2 can be erased while
 * running from bank 1. */
bool flash_mass_erase_bank(uint8_t bank);

/*
 * Fast programming (RM0440 §3.3.7): one row = 32 double words = 256 bytes,
 * written back to back at up to ~4x the double-word rate. addr must be
 * 256-byte aligned and erased. Runs from RAM with interrupts disabled
 * (the flash cannot be read meanwhile); data must not be in flash.
 */
#define FLASH_ROW_SIZE 256u
bool flash_fast_program_row(uint32_t addr, const uint64_t *data);

/*
 * Write protection (option bytes WRP1AR/WRP1BR), page granularity.
 * Area A or B protects pages start..end; start > end = area disabled.
 * flash_set_write_protection() programs the option bytes and launches
 * them — which RESETS the MCU (it does not return on success).
 */
typedef enum {
    FLASH_WRP_AREA_A = 0, FLASH_WRP_AREA_B = 1,     /* bank 1 */
    FLASH_WRP_AREA_2A = 2, FLASH_WRP_AREA_2B = 3,   /* L4 bank 2; pages relative to the bank */
} flash_wrp_area_t;
void flash_get_write_protection(flash_wrp_area_t area, uint32_t *start, uint32_t *end);
bool flash_set_write_protection(flash_wrp_area_t area, uint32_t start, uint32_t end);
/** Page currently protected by area A or B. */
bool flash_page_is_protected(uint32_t page);

/*
 * L4 dual-bank boot (option bit BFB2): when set, the MCU boots from bank 2
 * if it holds a valid image (bank 2 is then mapped at 0x08000000).
 * flash_set_dual_boot() programs the option and resets the MCU.
 */
bool flash_dual_boot_enabled(void);
bool flash_set_dual_boot(bool boot_from_bank2);
/** Bank currently mapped at 0x08000000 (1 or 2). */
uint8_t flash_boot_bank(void);
