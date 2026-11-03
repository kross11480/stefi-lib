#pragma once
#include <stdbool.h>

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

/** The last reset was a wakeup from Standby (PWR_SR1.SBF). */
bool pwr_woke_from_standby(void);
void pwr_clear_standby_flag(void);

/** Backup-domain write access (RTC, RCC_BDCR). */
void pwr_backup_access(bool enable);
