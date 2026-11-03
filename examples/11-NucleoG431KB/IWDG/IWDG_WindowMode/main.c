/*
 * IWDG_WindowMode (STM32CubeG4, NUCLEO-G431KB) on stefi-lib. SYSCLK 150 MHz.
 *
 * IWDG: LSI / 16, reload 1561 (~762 ms timeout), window 800 (~400 ms):
 * a refresh is only accepted in the last ~400 ms before the timeout.
 * The main loop refreshes every 450 ms (inside the window) and toggles
 * LED2. Putting/removing the jumper PA12 (Arduino D2) <-> GND raises an
 * EXTI interrupt that shortens the refresh period to 200 ms: the refresh
 * then comes too early (outside the window) and the IWDG resets the MCU.
 * After an IWDG reset LED2 is on for 4 s.
 * Timeouts depend on the LSI, which varies from part to part.
 */
#include "g4_example.h"
#include "libstefi/iwdg.h"

#define IWDG_RELOAD 1561u   /* 762 ms */
#define IWDG_WINDOW 800u    /* 400 ms */

static volatile uint32_t refresh_period_ms = 450;

static void on_jumper(void *aux)
{
    (void) aux;
    refresh_period_ms = 200;
}

void main(void)
{
    board_init();
    example_set_clock(150000000);

    if (system_reset_flags() & RESET_CAUSE_IWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    gpio_interrupt_register_handler(BOARD_JUMPER_PIN, on_jumper);
    gpio_enable_interrupt(BOARD_JUMPER_PIN, BOTH_EDGES);   // put or remove

    iwdg_start(IWDG_PRESCALER_16, IWDG_RELOAD, IWDG_WINDOW);

    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(refresh_period_ms);
        iwdg_refresh();
    }
}
