#pragma once

#include "stefi_board.h"

/*
 * Bare NUCLEO-G431KB (NUCLEO-32, MB1430) — the board ST's STM32CubeG4
 * examples are written for (Projects/NUCLEO-G431KB/Examples), ported in
 * examples/11-NucleoG431KB.
 *
 *   LD2 (green)          PB8, active high
 *   "jumper" input       PA12 (Arduino D2): ST's examples use a wire/jumper
 *                        to GND as the user input (the board has no user
 *                        button) — here it is BUTTON_D2, pull-up, active low
 *   ST-Link VCP          USART2, PA2/PA3
 *
 * For a NUCLEO-G431KB in the RobOhmobil, use rohmi_g431 instead.
 */

/* board_init() runs the core at 170 MHz (PLL from HSI16, boost mode), as
 * ST's examples do. Timers and APB buses run at the same clock. */
#define BOARD_SYSCLK      (170000000U)

enum {
    LED2 = 0,       // PB8
    NUM_LEDS
};

enum {
    BUTTON_D2 = 0,  // PA12 jumper to GND
    NUM_BUTTONS
};

/* The PA12 input as a plain pin (examples that poll it, or arm their own EXTI). */
#define BOARD_JUMPER_PIN  A12

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
