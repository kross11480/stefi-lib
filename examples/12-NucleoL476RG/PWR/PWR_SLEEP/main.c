/*
 * PWR_SLEEP: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 toggles 5 s, then Sleep until B1.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/systick.h>

static void on_button(void *aux) { (void) aux; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);
    while(1) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        pwr_enter_sleep();          // until B1
        systick_start();
    }
}
