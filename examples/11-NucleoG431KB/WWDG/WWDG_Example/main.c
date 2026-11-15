/*
 * WWDG_Example: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * WWDG (prescaler 128, window 0x50) refreshed every ~169 ms (LED2 toggles).
 * Jumper PA12-GND: HardFault, no refresh -> reset. LED2 on 4 s after it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/wwdg.h>

static void set_core_clock(uint32_t hz) {
    system_init_g4(hz);
    systick_init(hz);
    systick_start();
}

#define WWDG_PRESCALER_LOG2 7     /* /128 */
#define WWDG_WINDOW         0x50
#define WWDG_COUNTER        0x7F

static void on_jumper(void *aux) {
    (void) aux;
    *(volatile uint32_t *) 0x60000000u = 0;   // no memory there: bus fault -> HardFault
}

void main() {
    board_init();
    set_core_clock(150000000);

    if (system_reset_flags() & RESET_CAUSE_WWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    button_interrupt_init(BUTTON_D2, on_jumper);

    uint32_t tick_us = wwdg_tick_us(system_get_core_clock(), WWDG_PRESCALER_LOG2);
    uint32_t delay_ms = (tick_us * (WWDG_COUNTER - WWDG_WINDOW + 1u) + 999u) / 1000u + 1u;

    wwdg_start(WWDG_PRESCALER_LOG2, WWDG_WINDOW, WWDG_COUNTER);
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(delay_ms);
        wwdg_refresh(WWDG_COUNTER);
    }
}
