#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* PWR low-power modes (STM32L4 / STM32G4)                            */
/*                                                                    */
/*   Stop 0/1: clocks stopped, RAM + registers kept; an EXTI event    */
/*     (e.g. the RTC wakeup timer, EXTI line 20) resumes execution    */
/*     after pwr_enter_*() — on HSI16, so restore the PLL afterwards. */
/*   Standby: everything off except the backup domain; a wakeup       */
/*     restarts the program from reset (pwr_woke_from_standby()).     */
/* SysTick is paused around Stop (its interrupt would otherwise end   */
/* a WFI straight away).                                              */
/**********************************************************************/

void pwr_enter_stop0(void);
void pwr_enter_stop1(void);
/** Does not return: the next wakeup is a reset. RTC wakeup works as the
 *  internal wakeup line (enabled here). */
_Noreturn void pwr_enter_standby(void);

/** Sleep: core clock stopped until any interrupt (peripherals run). */
void pwr_enter_sleep(void);
/** L4: Stop 2 (lowest Stop current; only LPTIM1, I2C3, LPUART, COMP, RTC wake). */
void pwr_enter_stop2(void);
/** L4/G4: Shutdown — like Standby without BOR and without SRAM2/backup of
 *  the regulator; wakeup pins or reset restart the program. */
_Noreturn void pwr_enter_shutdown(void);

/** Low-power run: regulator in low-power mode. SYSCLK must be <= 2 MHz
 *  first (e.g. system_init_l4_msi(5)). */
void pwr_lprun_enter(void);
/** Back to the main regulator (raise the clock afterwards). */
void pwr_lprun_exit(void);
/** Low-power sleep = low-power run + Sleep (wake by interrupt, still in LP run). */
void pwr_enter_lp_sleep(void);

/** Wakeup pin n (1..5; L476 WKUP2 = PC13) for Standby/Shutdown;
 *  falling: active on a falling edge / low level. Clears its flag. */
void pwr_enable_wakeup_pin(uint8_t n, bool falling);
void pwr_disable_wakeup_pin(uint8_t n);
void pwr_clear_wakeup_flags(void);

/** L4 regulator range: 1 (up to 80 MHz) or 2 (up to 26 MHz, lower current).
 *  Lower the clock before selecting range 2. */
void pwr_set_voltage_range(uint8_t range);
/** L4 Standby: keep SRAM2 content (PWR_CR3.RRS). */
void pwr_retain_sram2_in_standby(bool retain);

/** The last reset was a wakeup from Standby (PWR_SR1.SBF). */
bool pwr_woke_from_standby(void);
void pwr_clear_standby_flag(void);

/** Backup-domain write access (RTC, RCC_BDCR). */
void pwr_backup_access(bool enable);
