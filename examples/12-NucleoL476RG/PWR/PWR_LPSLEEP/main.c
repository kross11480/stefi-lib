/*
 * PWR_LPSLEEP: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 toggles 5 s at 32 MHz, then low-power sleep at 2 MHz until B1.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

static void on_button(void *aux) { (void) aux; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);
    while(1) {
        system_init_l4_msi(10);
        systick_init(32000000u);
        systick_start();
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        system_init_l4_msi(5);               // 2 MHz
        pwr_enter_lp_sleep();                // until B1
        pwr_lprun_exit();
    }
}
