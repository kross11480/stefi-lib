#pragma once

#include "stefi_board.h"

/*
 * Bare NUCLEO-L476RG (NUCLEO-64, MB1136) — the board ST's STM32CubeL4
 * examples are written for (Projects/NUCLEO-L476RG/Examples), ported in
 * examples/12-NucleoL476RG.
 *
 *   LD2 (green)          PA5, active high
 *   B1 user button       PC13, active low (external pull-up), also WKUP2,
 *                        RTC tamper 1 / timestamp input
 *   ST-Link VCP          USART2, PA2/PA3
 *   32.768 kHz LSE       PC14/PC15 (fitted on MB1136 rev C)
 *
 * For the STefi adapter board on a NUCLEO-L476RG, use nucleo_l476 instead.
 */

/* board_init() runs the core at 80 MHz (PLL from MSI), as ST's examples do. */
#define BOARD_SYSCLK      (80000000U)

enum {
    LED2 = 0,       // PA5
    NUM_LEDS
};

enum {
    BUTTON_USER = 0, // B1, PC13
    NUM_BUTTONS
};

#define BOARD_BUTTON_PIN  C13

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
