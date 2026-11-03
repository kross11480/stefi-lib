#pragma once

#include "stefi_board.h"

/*
 * NUCLEO-L476RG (NUCLEO-64) wired to the USB-oscilloscope front-end
 * (kross11480/usb-oscilloscope). The front-end PCB plugs onto a NUCLEO-144
 * F767 (ports E/F); the 64-pin L476RG has no ports E/F, so the signals are
 * re-routed to morpho-connector pins (CN7/CN10):
 *
 *   LTC1420 ADC  D0..D11 -> PC0..PC11, OF -> PC12    (one port: one DMA read)
 *   ADC sample clock      TIM1_CH1 (AF1) on PA8      (Arduino D7)
 *   LT1714 trigger out    PA0 (EXTI0)                (Arduino A0)
 *   LTC1450 DAC  D0..D11 <- PB0..PB11, /CLR <- PB12, /LD <- PB13
 *                         open drain, pull-ups on the front-end. With 5 V
 *                         pull-ups every DAC pin must be 5 V tolerant (FT) —
 *                         check DS10198 for your pin choice; pull-ups to 3.3 V
 *                         also satisfy the LTC1450's 2.4 V input high level.
 *   USB device            PA11 = D-, PA12 = D+ (CN10-14 / CN10-12) to a USB
 *                         connector, plus GND. VBUS is not sensed.
 *   ST-Link VCP (printf)  USART2, PA2/PA3
 *   LD2 green LED PA5, B1 user button PC13 (active low)
 *
 * Kept free: PA13/PA14 (SWD), PC14/PC15 (32.768 kHz LSE — USB clock trim).
 */

/* board_init() switches to 80 MHz (PLL from HSI16): timers then run at
 * 80 MHz, which is what the ADC clock is divided from. */
#define BOARD_SYSCLK      (80000000U)
#define BOARD_TIMER_CLK   (80000000U)

enum {
    LED_GREEN = 0,  // LD2, PA5
    NUM_LEDS
};

enum {
    BUTTON_USER = 0, // B1, PC13 (pressed = low, external pull-up)
    NUM_BUTTONS
};

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
