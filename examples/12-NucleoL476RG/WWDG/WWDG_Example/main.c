/* WWDG_Example (STM32CubeL4, NUCLEO-L476RG) on stefi-lib. PCLK1 = 80 MHz.
 *
 * WWDG: prescaler 8 (one tick = 4096 * 8 / 80 MHz = 409.6 us), counter
 * 0x7F, window 0x50: refresh allowed once the counter is below 0x50
 * ((0x7F - 0x50 + 1) ticks = 19.7 ms after the last refresh), reset at 0x3F.
 * The loop refreshes every ~21 ms and toggles LD2 (fast blinking).
 * B1 simulates a software failure (invalid address in its interrupt:
 * HardFault, no more refresh) -> WWDG reset; after it LD2 is on for 4 s.
 * Run without a debugger. */
#include "l4_example.h"
#include "libstefi/wwdg.h"

#define WWDG_PRESCALER_LOG2 3
#define WWDG_WINDOW         0x50
#define WWDG_COUNTER        0x7F

static void on_button(void *aux)
{
    (void) aux;
    *(volatile uint32_t *) 0xA0003000u = 0xFF;   // bus fault -> HardFault loop
}

void main(void)
{
    board_init();
    example_button_irq(on_button);

    if (system_reset_flags() & RESET_CAUSE_WWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    uint32_t tick_us = wwdg_tick_us(system_get_core_clock(), WWDG_PRESCALER_LOG2);
    uint32_t delay_ms = (tick_us * (WWDG_COUNTER - WWDG_WINDOW + 1u) + 999u) / 1000u + 1u;
    wwdg_start(WWDG_PRESCALER_LOG2, WWDG_WINDOW, WWDG_COUNTER);
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(delay_ms);
        wwdg_refresh(WWDG_COUNTER);
    }
}
