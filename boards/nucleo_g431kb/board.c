#include "board.h"

#include "button.h"
#include "led.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

/*
 * NUCLEO-G431KB, as shipped (no shield).
 *
 * Note (timer budget):
 *  Systick  - delay functions (1 ms)
 *  Timer 6  - button debouncing (10 ms)
 *  Everything else is free for the application.
 */

led_config_t leds[NUM_LEDS] = {
    [LED2] = {B8, LOW},
};

button_config_t buttons[NUM_BUTTONS] = {
    [BUTTON_D2] = {A12, PULL_UP},
};

_Static_assert(NUM_BUTTONS <= STEFI_MAX_BUTTONS,
        "button.c sizes its per-button state by STEFI_MAX_BUTTONS");

hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER] = {
    // 170 MHz / 17000 = 10 kHz tick, 100 ticks = 10 ms
    [DEBOUNCE_TIMER] = {TIMER6, 17000, 100},
};

void board_init() {
    system_init_g4(BOARD_SYSCLK);

    //UART (USART2/PA2/PA3 — ST-Link VCP), HSI16-clocked: unaffected by the PLL
    uart_configure();

    for(uint32_t i = 0; i < NUM_LEDS; i++) {
        led_init(i, OUTPUT);
    }
    for(uint32_t i = 0; i < NUM_BUTTONS; i++) {
        button_init(i);
    }
    for(uint32_t i = 0; i < NUM_RESERVED_TIMER; i++) {
        timer_init(hardware_timers[i].timer);
        timer_set_period(hardware_timers[i].timer, hardware_timers[i].prescaler, hardware_timers[i].period);
    }

    systick_init(BOARD_SYSCLK);
    systick_start();
}
