/*System Initialization*/
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Initialize Clock to 80 MHz (L4/G4: PLL from HSI16; F7: 200 MHz) */
void system_init(void);

/**
 * @brief STM32G4: run SYSCLK = HCLK = PCLK1/2 at @p hz from HSI16 via the PLL
 * (M = 4, N = hz / 2 MHz, R = 2), with the matching flash wait states and,
 * above 150 MHz, the regulator's range-1 boost mode (RM0440 §6.1.5).
 * Also usable to restore the clock after waking from Stop (which falls back
 * to HSI16).
 * @param hz  even number of MHz, 16..170 MHz (e.g. 170000000, 150000000)
 * @return false on other MCUs or an unsupported frequency (clock unchanged)
 */
bool system_init_g4(uint32_t hz);

/* ---------------------------- STM32L4 ---------------------------- */
typedef enum { SYSTEM_CLK_MSI = 0, SYSTEM_CLK_HSI16 = 1 } system_clk_src_t;

/**
 * @brief STM32L4: SYSCLK = HCLK = PCLKx = @p hz from the PLL, fed by MSI
 * (4 MHz range) or HSI16, VCO input 4 MHz (M = 1 / 4), N = hz / 2 MHz, R = 2.
 * Safe to call while the PLL is the system clock (switches away first).
 * @param hz  even number of MHz, 16..80 MHz
 */
bool system_init_l4(system_clk_src_t src, uint32_t hz);

/**
 * @brief STM32L4: SYSCLK = MSI directly (PLL off), for low-power work.
 * @param range  MSI range 0..11: 100k 200k 400k 800k 1M 2M 4M 8M 16M 24M 32M 48M
 */
bool system_init_l4_msi(uint8_t range);

/** Source currently feeding the PLL (L4). */
system_clk_src_t system_pll_source(void);

/* Microcontroller clock output on PA8 (AF0): source and divider. */
typedef enum {
    MCO_OFF = 0, MCO_SYSCLK = 1, MCO_MSI = 2, MCO_HSI16 = 3, MCO_HSE = 4,
    MCO_PLL = 5, MCO_LSI = 6, MCO_LSE = 7,
} mco_source_t;
/** @param div_log2  0..4: divide by 1, 2, 4, 8, 16 */
void system_mco(mco_source_t src, uint8_t div_log2);

/** L4: measure the LSI with TIM16 (CH1 internally remapped to the LSI),
 *  against the current core clock. Takes a few ms. */
uint32_t system_measure_lsi_hz(void);

/** Current core clock in Hz, as last set by the system_init* calls. */
uint32_t system_get_core_clock(void);

/* Reset cause, from RCC_CSR (L4/G4). Flags accumulate until cleared. */
typedef enum {
    RESET_CAUSE_PIN      = (1u << 26),
    RESET_CAUSE_BOR      = (1u << 27),
    RESET_CAUSE_SOFTWARE = (1u << 28),
    RESET_CAUSE_IWDG     = (1u << 29),
    RESET_CAUSE_WWDG     = (1u << 30),
    RESET_CAUSE_LOWPOWER = (1u << 31),
} reset_cause_t;

/** @return bit set of reset_cause_t flags since they were last cleared */
uint32_t system_reset_flags(void);
void system_clear_reset_flags(void);

/** Software reset (SCB AIRCR SYSRESETREQ). */
_Noreturn void system_reset(void);
