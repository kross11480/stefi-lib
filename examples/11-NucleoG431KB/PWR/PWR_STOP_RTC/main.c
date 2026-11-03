/*
 * PWR_STOP0_RTC / PWR_STOP1_RTC (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 * Built twice: STOP_MODE=0 (g4_pwr_stop0_rtc) and STOP_MODE=1 (g4_pwr_stop1_rtc).
 *
 * LED2 toggles for 5 s (Run mode), then the RTC wakeup timer (LSI / 16,
 * 0xFFFF ticks: ~33 s) is armed and the MCU enters Stop 0 / Stop 1 (LED2
 * off) — measure IDD on JP4 now. The RTC wakeup event (EXTI line 20)
 * resumes execution; the clock is restored to 170 MHz and the cycle repeats.
 * Not usable under a debugger (the core is not clocked in Stop mode).
 */
#include "g4_example.h"
#include "libstefi/pwr.h"
#include "libstefi/rtc.h"

#ifndef STOP_MODE
#define STOP_MODE 1
#endif

void main(void)
{
    board_init();
    rtc_init_lsi();

    for (;;) {
        for (uint32_t t = 0; t < 5000; t += 100) {
            led_toggle(LED2);
            systick_delay_ms(100);
        }
        led_off(LED2);

        rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
#if STOP_MODE == 0
        pwr_enter_stop0();
#else
        pwr_enter_stop1();
#endif
        // woken by the RTC: running on HSI16 now
        rtc_wakeup_stop();
        example_set_clock(BOARD_SYSCLK);
    }
}
