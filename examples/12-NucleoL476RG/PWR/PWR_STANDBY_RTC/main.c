/* PWR_STANDBY_RTC (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * LD2 toggles for 5 s, then the RTC wakeup timer (LSI / 16, 0xFFFF ticks:
 * ~33 s) is armed and the MCU enters Standby (LD2 off). The RTC wakeup or
 * the reset button restarts the program from reset; the cycle repeats. */
#include "l4_example.h"
#include "libstefi/pwr.h"
#include "libstefi/rtc.h"

void main(void)
{
    board_init();
    if (pwr_woke_from_standby()) pwr_clear_standby_flag();
    rtc_init_lsi();
    rtc_wakeup_stop();
    for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
    led_off(LED2);
    rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
    pwr_enter_standby();
}
