#pragma once
#include <stdint.h>
#include "libstefi/interrupts.h"

/**********************************************************************/
/* RTC wakeup timer (STM32L4 / STM32G4), clocked from the LSI         */
/* (~32 kHz). Wakes the MCU from Sleep/Stop (EXTI line 20) and from   */
/* Standby (internal wakeup line).                                    */
/**********************************************************************/

typedef enum {
    RTC_WAKEUP_CLK_DIV16 = 0,   /* LSI/16 ~ 2 kHz: up to 32.7 s, 0.5 ms steps */
    RTC_WAKEUP_CLK_DIV8  = 1,
    RTC_WAKEUP_CLK_DIV4  = 2,
    RTC_WAKEUP_CLK_DIV2  = 3,
    RTC_WAKEUP_CLK_1HZ   = 4,   /* ck_spre (~1 Hz with the reset prescalers) */
} rtc_wakeup_clock_t;

/** LSI on, RTC clocked from it and enabled (backup domain reset if it was
 *  running from another source). */
void rtc_init_lsi(void);

/**
 * @brief Start the periodic wakeup timer: an event every (count + 1) ticks.
 * @param fn  optional callback from the RTC_WKUP interrupt (may be NULL:
 *            the interrupt still runs, ending a Stop/WFI)
 */
void rtc_wakeup_start(rtc_wakeup_clock_t clk, uint16_t count, callbackfn_t fn, void *aux);
void rtc_wakeup_stop(void);
