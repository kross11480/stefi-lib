/*
 * PWR_STOP_RTC: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * Build variants STOP_MODE=0/1: LED2 toggles 5 s, then Stop 0/1 until the
 * RTC wakeup (~33 s); clock restored, repeat.
 */

#include "board.h"
#include "led.h"

#include <libstefi/pwr.h>
#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <stddef.h>

static void set_core_clock(uint32_t hz) {
    system_init_g4(hz);
    systick_init(hz);
    systick_start();
}

#ifndef STOP_MODE
#define STOP_MODE 1
#endif

void main() {
    board_init();
    rtc_init_lsi();

    while(1) {
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
        set_core_clock(BOARD_SYSCLK);
    }
}
