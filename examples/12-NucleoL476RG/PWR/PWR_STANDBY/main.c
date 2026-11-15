/*
 * PWR_STANDBY: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 toggles; B1 enters Standby, the next B1 press (WKUP2) restarts.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/systick.h>

static void on_button(void *aux) {
    (void) aux;
    while (button_is_pressed(BUTTON_USER));          // released, so the next press wakes
    pwr_enable_wakeup_pin(2, true);
    pwr_enter_standby();
}

void main() {
    board_init();
    if (pwr_woke_from_standby()) pwr_clear_standby_flag();
    pwr_disable_wakeup_pin(2);
    pwr_clear_wakeup_flags();
    button_interrupt_init(BUTTON_USER, on_button);
    while(1) { led_toggle(LED2); systick_delay_ms(200); }
}
