#pragma once
/*
 * Shared bits of the NUCLEO-L476RG examples: clock restore, user button,
 * and the LED2 patterns ST's examples use to report their result.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board.h"
#include "led.h"
#include "libstefi/gpio.h"
#include "libstefi/system.h"
#include "libstefi/systick.h"
#include "libstefi/util.h"

/* Back to 80 MHz (e.g. after Stop, which returns on MSI/HSI). */
static inline void example_restore_clock(void)
{
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
}

/* "LED2 is slowly blinking (1 sec. period)": error. */
_Noreturn static inline void example_error(void)
{
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* "LED2 is ON": success. */
_Noreturn static inline void example_success(void)
{
    led_on(LED2);
    for (;;);
}

/* B1 (PC13) is pressed (active low). */
static inline bool example_button_pressed(void)
{
    return gpio_read(BOARD_BUTTON_PIN) == LOW;
}

/* Interrupt on the B1 press (falling edge), without debouncing. */
static inline void example_button_irq(callbackfn_t fn)
{
    gpio_interrupt_register_handler(BOARD_BUTTON_PIN, fn);
    gpio_enable_interrupt(BOARD_BUTTON_PIN, FALLING_EDGE);
}

/* Wait for a press, blinking LED2 fast meanwhile (as ST's two-board examples). */
static inline void example_wait_button(void)
{
    while (!example_button_pressed()) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    while (example_button_pressed());
}
