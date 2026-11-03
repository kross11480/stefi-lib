/* PWR_STOP1_RTC / PWR_STOP2_RTC (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * Built twice: STOP_MODE=1 and STOP_MODE=2.
 *
 * LD2 toggles for 5 s, then the RTC wakeup timer (LSI / 16, 0xFFFF ticks:
 * ~33 s) is armed and the MCU enters Stop 1 / Stop 2 (LD2 off). The RTC
 * wakeup (EXTI line 20) resumes execution; clock restored, cycle repeats. */
#include "l4_example.h"
#include "libstefi/pwr.h"
#include "libstefi/rtc.h"

void main(void)
{
    board_init();
    rtc_init_lsi();
    for (;;) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
#if STOP_MODE == 2
        pwr_enter_stop2();
#else
        pwr_enter_stop1();
#endif
        rtc_wakeup_stop();
        example_restore_clock();
    }
}
