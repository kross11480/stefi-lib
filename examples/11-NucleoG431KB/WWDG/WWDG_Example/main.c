/*
 * WWDG_Example (STM32CubeG4, NUCLEO-G431KB) on stefi-lib. SYSCLK = PCLK1
 * = 150 MHz.
 *
 * WWDG: prescaler 128 (one tick = 4096 * 128 / 150 MHz = 3.495 ms),
 * counter 0x7F, window 0x50: a refresh is allowed once the counter is
 * below 0x50, i.e. (0x7F - 0x50 + 1) ticks = 167.8 ms after the last one;
 * at 0x3F the MCU resets. The main loop refreshes every 168.8 ms and
 * toggles LED2.
 * Putting/removing the jumper PA12 (Arduino D2) <-> GND simulates a
 * software failure: its interrupt writes to an invalid address, the
 * HardFault handler loops, the WWDG is no longer refreshed -> reset.
 * After a WWDG reset LED2 is on for 4 s. Run without a debugger attached
 * (a debugger halting on the fault keeps the counter frozen).
 */
#include "g4_example.h"
#include "libstefi/wwdg.h"

#define WWDG_PRESCALER_LOG2 7     /* /128 */
#define WWDG_WINDOW         0x50
#define WWDG_COUNTER        0x7F

static void on_jumper(void *aux)
{
    (void) aux;
    *(volatile uint32_t *) 0x60000000u = 0;   // no memory there: bus fault -> HardFault
}

void main(void)
{
    board_init();
    example_set_clock(150000000);

    if (system_reset_flags() & RESET_CAUSE_WWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    gpio_interrupt_register_handler(BOARD_JUMPER_PIN, on_jumper);
    gpio_enable_interrupt(BOARD_JUMPER_PIN, RISING_EDGE);

    uint32_t tick_us = wwdg_tick_us(system_get_core_clock(), WWDG_PRESCALER_LOG2);
    uint32_t delay_ms = (tick_us * (WWDG_COUNTER - WWDG_WINDOW + 1u) + 999u) / 1000u + 1u;

    wwdg_start(WWDG_PRESCALER_LOG2, WWDG_WINDOW, WWDG_COUNTER);
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(delay_ms);
        wwdg_refresh(WWDG_COUNTER);
    }
}
