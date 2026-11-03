#pragma once
/*
 * Shared bits of the NUCLEO-G431KB examples: clock changes, and the LED2
 * patterns ST's examples use to report their result.
 */
#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "led.h"
#include "libstefi/gpio.h"
#include "libstefi/system.h"
#include "libstefi/systick.h"
#include "libstefi/util.h"

#include <stddef.h>

/* Some of ST's examples run at 150 MHz instead of board_init()'s 170. */
static inline void example_set_clock(uint32_t hz)
{
    system_init_g4(hz);
    systick_init(hz);
    systick_start();
}

/* "LED2 is blinking with a 1 s period": error / wrong result. */
_Noreturn static inline void example_error(void)
{
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* "LED2 is blinking (100 ms on, 2 s off)": peripheral operation failed. */
_Noreturn static inline void example_error_flash(void)
{
    for (;;) {
        led_on(LED2);
        systick_delay_ms(100);
        led_off(LED2);
        systick_delay_ms(2000);
    }
}

/* "LED2 is ON": success. */
_Noreturn static inline void example_success(void)
{
    led_on(LED2);
    for (;;);
}

/* The jumper between PA12 (Arduino D2) and GND is in place. */
static inline bool example_jumper_set(void)
{
    return gpio_read(BOARD_JUMPER_PIN) == LOW;
}
