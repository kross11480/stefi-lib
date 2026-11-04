/*
 * PWR_LPRUN: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 toggles 5 s at 32 MHz, then low-power run at 100 kHz until B1.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

static void run_clock(void) {
    system_init_l4_msi(10);                  // 32 MHz
    systick_init(32000000u);
    systick_start();
}

void main() {
    board_init();
    run_clock();
    while(1) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        system_init_l4_msi(0);               // 100 kHz
        pwr_lprun_enter();
        while(!button_is_pressed(BUTTON_USER));   //low-power run until B1 (polled:
        while(button_is_pressed(BUTTON_USER));    //the debounce timer is too slow at 100 kHz)
        pwr_lprun_exit();
        run_clock();
    }
}
