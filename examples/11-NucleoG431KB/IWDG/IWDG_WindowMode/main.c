/*
 * IWDG_WindowMode: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * IWDG ~762 ms timeout, window ~400 ms, refreshed every 450 ms (LED2 toggles).
 * Jumper PA12-GND: refresh every 200 ms, too early -> reset. LED2 on 4 s after it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/iwdg.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

static void set_core_clock(uint32_t hz) {
    system_init_g4(hz);
    systick_init(hz);
    systick_start();
}

#define IWDG_RELOAD 1561u   /* 762 ms */
#define IWDG_WINDOW 800u    /* 400 ms */

static volatile uint32_t refresh_period_ms = 450;

static void on_jumper(void *aux) {
    (void) aux;
    refresh_period_ms = 200;
}

void main() {
    board_init();
    set_core_clock(150000000);

    if (system_reset_flags() & RESET_CAUSE_IWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    button_interrupt_init(BUTTON_D2, on_jumper);

    iwdg_start(IWDG_PRESCALER_16, IWDG_RELOAD, IWDG_WINDOW);

    while(1) {
        led_toggle(LED2);
        systick_delay_ms(refresh_period_ms);
        iwdg_refresh();
    }
}
