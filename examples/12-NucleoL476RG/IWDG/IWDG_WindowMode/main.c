/*
 * IWDG_WindowMode: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * IWDG ~762 ms, window ~400 ms, refreshed every 450 ms (LED2 toggles).
 * B1: refresh every 200 ms, too early -> reset. LED2 on 4 s after it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/iwdg.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

static volatile uint32_t refresh_period_ms = 450;

static void on_button(void *aux) { (void) aux; refresh_period_ms = 200; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

    if (system_reset_flags() & RESET_CAUSE_IWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    iwdg_start(IWDG_PRESCALER_16, (32000u * 762u) / 16000u, (32000u * 400u) / 16000u);
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(refresh_period_ms);
        iwdg_refresh();
    }
}
