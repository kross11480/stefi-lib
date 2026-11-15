/*
 * PWR_STANDBY_RTC: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * LED2 toggles 5 s, then Standby; the RTC wakeup (~33 s) restarts the program.
 */

#include "board.h"
#include "led.h"

#include <libstefi/pwr.h>
#include <libstefi/rtc.h>
#include <libstefi/systick.h>

#include <stddef.h>

void main() {
    board_init();

    if (pwr_woke_from_standby()) {
        pwr_clear_standby_flag();
    }
    rtc_init_lsi();
    rtc_wakeup_stop();              // from the previous cycle

    for (uint32_t t = 0; t < 5000; t += 100) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);

    rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
    pwr_enter_standby();
}
