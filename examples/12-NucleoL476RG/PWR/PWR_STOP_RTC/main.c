/*
 * PWR_STOP_RTC: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Build variants STOP_MODE=1/2: LED2 toggles 5 s, then Stop 1/2 until the
 * RTC wakeup (~33 s).
 */

#include "board.h"
#include "led.h"

#include <libstefi/pwr.h>
#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <stddef.h>

/* back to 80 MHz, e.g. after Stop (which wakes up on MSI) */
static void restore_core_clock() {
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
}

void main() {
    board_init();
    rtc_init_lsi();
    while(1) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
#if STOP_MODE == 2
        pwr_enter_stop2();
#else
        pwr_enter_stop1();
#endif
        rtc_wakeup_stop();
        restore_core_clock();
    }
}
