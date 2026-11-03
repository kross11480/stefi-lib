#include "libstefi/flash.h"
#include "libstefi/util.h"
#include "internal/flash_internal.h"
#include "internal/mcu_id.h"

#include <string.h>

static void flash_wait_idle(void) {
    while (FLASH_REG->SR & FLASH_SR_BSY) {
    }
}

static void flash_unlock(void) {
    if (FLASH_REG->CR & FLASH_CR_LOCK) {
        FLASH_REG->KEYR = FLASH_KEY1;
        FLASH_REG->KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void) {
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

/* Wait for the current operation, then report and clear EOP + error flags. */
static bool flash_finish(void) {
    flash_wait_idle();
    uint32_t sr = FLASH_REG->SR;
    FLASH_REG->SR = sr & (FLASH_SR_EOP | FLASH_SR_ALL_ERR); /* w1c */
    return (sr & FLASH_SR_ALL_ERR) == 0;
}

/* L476: 2 x 256 pages of 2 KB (dual bank, contiguous at 0x08000000/0x08080000) */
#define L4_PAGES_PER_BANK 256u

uint32_t flash_num_pages(void) {
    return mcu_is_g4() ? FLASH_NUM_PAGES : 2u * L4_PAGES_PER_BANK;
}

bool flash_erase_page(uint32_t page) {
    if (page >= flash_num_pages()) {
        return false;
    }
    uint32_t bank_bit = 0;
    if (!mcu_is_g4() && page >= L4_PAGES_PER_BANK) {
        page -= L4_PAGES_PER_BANK;
        bank_bit = FLASH_CR_BKER;
    }

    flash_unlock();
    flash_wait_idle();
    FLASH_REG->SR = FLASH_SR_EOP | FLASH_SR_ALL_ERR; /* clear stale flags */

    uint32_t cr = FLASH_REG->CR & ~(FLASH_CR_PNB_MSK | FLASH_CR_BKER);
    cr |= FLASH_CR_PER | (page << FLASH_CR_PNB_POS) | bank_bit;
    FLASH_REG->CR = cr;
    FLASH_REG->CR |= FLASH_CR_STRT;

    bool ok = flash_finish();

    FLASH_REG->CR &= ~(FLASH_CR_PER | FLASH_CR_PNB_MSK | FLASH_CR_BKER);
    flash_lock();
    return ok;
}

bool flash_write(uint32_t addr, const void *data, uint32_t len) {
    if ((addr % 8u) != 0 || (len % 8u) != 0) {
        return false;
    }

    const uint8_t *src = (const uint8_t *) data;

    flash_unlock();
    flash_wait_idle();
    FLASH_REG->SR = FLASH_SR_EOP | FLASH_SR_ALL_ERR; /* clear stale flags */
    FLASH_REG->CR |= FLASH_CR_PG;

    bool ok = true;
    for (uint32_t i = 0; i < len && ok; i += 8u) {
        uint32_t w0, w1;
        memcpy(&w0, src + i, 4);
        memcpy(&w1, src + i + 4, 4);
        /* Both words of the double word must be written before BSY clears. */
        *(volatile uint32_t *) (addr + i) = w0;
        *(volatile uint32_t *) (addr + i + 4u) = w1;
        ok = flash_finish();
    }

    FLASH_REG->CR &= ~FLASH_CR_PG;
    flash_lock();
    return ok;
}


bool flash_erase_pages(uint32_t first, uint32_t count, uint32_t *bad_page) {
    for (uint32_t p = first; p < first + count; p++) {
        if (!flash_erase_page(p)) {
            if (bad_page) *bad_page = p;
            return false;
        }
    }
    return true;
}

/* RAM functions: placed in .data (copied to SRAM by the startup code) and
 * reached by a long call, since SRAM is out of BL range from flash. They
 * touch registers only — no calls back into flash. */
#define RAMFUNC __attribute__((section(".data.ramfunc"), noinline, long_call))

RAMFUNC static uint32_t ram_mass_erase(uint32_t mer) {
    flash_t *f = FLASH_REG;
    while (f->SR & FLASH_SR_BSY);
    f->SR = FLASH_SR_EOP | FLASH_SR_ALL_ERR;
    f->CR |= mer;
    f->CR |= FLASH_CR_STRT;
    while (f->SR & FLASH_SR_BSY);
    uint32_t sr = f->SR;
    f->CR &= ~mer;
    return sr;
}

RAMFUNC static uint32_t ram_fast_row(uint32_t addr, const uint64_t *data) {
    flash_t *f = FLASH_REG;
    while (f->SR & FLASH_SR_BSY);
    f->SR = FLASH_SR_EOP | FLASH_SR_ALL_ERR;
    f->CR |= FLASH_CR_FSTPG;
    volatile uint32_t *dst = (volatile uint32_t *) addr;
    const uint32_t *src = (const uint32_t *) data;
    for (uint32_t i = 0; i < FLASH_ROW_SIZE / 4u; i++) dst[i] = src[i];
    while (f->SR & FLASH_SR_BSY);
    uint32_t sr = f->SR;
    f->CR &= ~FLASH_CR_FSTPG;
    return sr;
}

static inline uint32_t irq_off(void) {
    uint32_t m;
    __asm volatile ("mrs %0, primask\n cpsid i" : "=r" (m) :: "memory");
    return m;
}
static inline void irq_restore(uint32_t m) {
    __asm volatile ("msr primask, %0" :: "r" (m) : "memory");
}

static bool mass_erase(uint32_t mer) {
    flash_unlock();
    uint32_t key = irq_off();
    uint32_t sr = ram_mass_erase(mer);
    irq_restore(key);
    FLASH_REG->SR = sr & (FLASH_SR_EOP | FLASH_SR_ALL_ERR);
    flash_lock();
    return (sr & FLASH_SR_ALL_ERR) == 0;
}

bool flash_mass_erase(void) {
    return mass_erase(mcu_is_g4() ? FLASH_CR_MER1 : (FLASH_CR_MER1 | FLASH_CR_MER2));
}

bool flash_mass_erase_bank(uint8_t bank) {
    if (mcu_is_g4()) return bank == 1 && mass_erase(FLASH_CR_MER1);
    return mass_erase(bank == 2 ? FLASH_CR_MER2 : FLASH_CR_MER1);
}

bool flash_fast_program_row(uint32_t addr, const uint64_t *data) {
    if ((addr % FLASH_ROW_SIZE) != 0) return false;
    flash_unlock();
    uint32_t key = irq_off();
    uint32_t sr = ram_fast_row(addr, data);
    irq_restore(key);
    FLASH_REG->SR = sr & (FLASH_SR_EOP | FLASH_SR_ALL_ERR);
    flash_lock();
    return (sr & FLASH_SR_ALL_ERR) == 0;
}

static volatile uint32_t *wrp_reg(flash_wrp_area_t area) {
    switch (area) {
        case FLASH_WRP_AREA_A: return &FLASH_REG->WRP1AR;
        case FLASH_WRP_AREA_B: return &FLASH_REG->WRP1BR;
        case FLASH_WRP_AREA_2A: return &FLASH_REG->WRP2AR;
        default: return &FLASH_REG->WRP2BR;
    }
}

static uint32_t wrp_mask(void) { return mcu_is_g4() ? 0x7Fu : 0xFFu; }

/* start/end are pages within the area's bank */
void flash_get_write_protection(flash_wrp_area_t area, uint32_t *start, uint32_t *end) {
    uint32_t r = *wrp_reg(area);
    *start = r & wrp_mask();
    *end = (r >> 16) & wrp_mask();
}

bool flash_page_is_protected(uint32_t page) {
    uint32_t bank = 0;
    if (!mcu_is_g4() && page >= L4_PAGES_PER_BANK) { bank = 1; page -= L4_PAGES_PER_BANK; }
    int areas = mcu_is_g4() ? 2 : 4;
    for (int a = 0; a < areas; a++) {
        if ((uint32_t) (a / 2) != bank) continue;
        uint32_t s0, e0;
        flash_get_write_protection((flash_wrp_area_t) a, &s0, &e0);
        if (s0 <= e0 && page >= s0 && page <= e0) return true;
    }
    return false;
}

static bool option_program(volatile uint32_t *reg, uint32_t clear, uint32_t set) {
    flash_unlock();
    if (FLASH_REG->CR & FLASH_CR_OPTLOCK) {
        FLASH_REG->OPTKEYR = FLASH_OPTKEY1;
        FLASH_REG->OPTKEYR = FLASH_OPTKEY2;
    }
    flash_wait_idle();
    FLASH_REG->SR = FLASH_SR_EOP | FLASH_SR_ALL_ERR;
    *reg = (*reg & ~clear) | set;
    FLASH_REG->CR |= FLASH_CR_OPTSTRT;
    if (!flash_finish()) {
        FLASH_REG->CR |= FLASH_CR_OPTLOCK;
        flash_lock();
        return false;
    }
    FLASH_REG->CR |= FLASH_CR_OBL_LAUNCH;   // reload option bytes: system reset
    for (;;);
}

bool flash_dual_boot_enabled(void) {
    return !mcu_is_g4() && (FLASH_REG->OPTR & FLASH_OPTR_BFB2);
}

bool flash_set_dual_boot(bool boot_from_bank2) {
    if (mcu_is_g4()) return false;
    return option_program(&FLASH_REG->OPTR, FLASH_OPTR_BFB2, boot_from_bank2 ? FLASH_OPTR_BFB2 : 0u);
}

uint8_t flash_boot_bank(void) {
    // SYSCFG_MEMRMP.FB_MODE [8]: bank 2 mapped at 0x08000000
    return (*(volatile uint32_t *) 0x40010000u & BIT(8)) ? 2u : 1u;
}

bool flash_set_write_protection(flash_wrp_area_t area, uint32_t start, uint32_t end) {
    uint32_t m = wrp_mask();
    return option_program(wrp_reg(area), (m << 16) | m, (start & m) | ((end & m) << 16));
}
