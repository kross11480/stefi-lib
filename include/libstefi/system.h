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
