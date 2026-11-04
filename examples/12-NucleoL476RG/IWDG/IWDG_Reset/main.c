/*
 * IWDG_Reset: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * IWDG set to 1 s from the measured LSI, refreshed every 990 ms (LED2 toggles).
 * B1: HardFault, no refresh -> reset. LED2 on 4 s after it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/iwdg.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

static void on_button(void *aux) {
    (void) aux;
    *(volatile uint32_t *) 0xA0003000u = 0xFF;   // no memory there: bus fault
}

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

    if (system_reset_flags() & RESET_CAUSE_IWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    uint32_t lsi_hz = system_measure_lsi_hz();
    iwdg_start(IWDG_PRESCALER_32, (uint16_t) (lsi_hz / 32u), IWDG_WINDOW_DISABLED);

    while(1) {
        led_toggle(LED2);
        systick_delay_ms(990);
        iwdg_refresh();
    }
}
