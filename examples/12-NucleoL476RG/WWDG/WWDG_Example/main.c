/*
 * WWDG_Example: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * WWDG (prescaler 8, window 0x50) refreshed every ~21 ms (LED2 toggles).
 * B1: HardFault, no refresh -> reset. LED2 on 4 s after it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/wwdg.h>

#define WWDG_PRESCALER_LOG2 3
#define WWDG_WINDOW         0x50
#define WWDG_COUNTER        0x7F

static void on_button(void *aux) {
    (void) aux;
    *(volatile uint32_t *) 0xA0003000u = 0xFF;   // bus fault -> HardFault loop
}

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

    if (system_reset_flags() & RESET_CAUSE_WWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    uint32_t tick_us = wwdg_tick_us(system_get_core_clock(), WWDG_PRESCALER_LOG2);
    uint32_t delay_ms = (tick_us * (WWDG_COUNTER - WWDG_WINDOW + 1u) + 999u) / 1000u + 1u;
    wwdg_start(WWDG_PRESCALER_LOG2, WWDG_WINDOW, WWDG_COUNTER);
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(delay_ms);
        wwdg_refresh(WWDG_COUNTER);
    }
}
