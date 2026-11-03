#pragma once

#include "stefi_board.h"

/*
 * NUCLEO-F767ZI (NUCLEO-144) with the USB-oscilloscope front-end
 * (kross11480/usb-oscilloscope, KiCad "Entwurf1"):
 *
 *   LTC1420 12-bit 10 MSPS ADC, parallel out  D11..D0 -> PF14..PF3, OF -> PF15
 *   ADC sample clock                           TIM1_CH1 (AF1) on PE9
 *   LT1714 comparator trigger output           PF2  (EXTI2)
 *   LTC1450 12-bit DAC (ADC reference)         D11..D7 <- PE14..PE10,
 *                                              D6..D0  <- PE8..PE2,
 *                                              /CLR <- PE0, /LD <- PE1
 *                                              (open drain, 5 V pull-ups)
 *   USB device (CN13, OTG_FS)                  PA11/PA12
 *   ST-Link virtual COM port (printf)          USART3, PD8/PD9
 */

/* board_init() switches to 200 MHz (needed for the 48 MHz USB clock and the
 * 100 MHz timer kernel the ADC clock is derived from). */
#define BOARD_SYSCLK      (200000000U)
#define BOARD_TIMER_CLK   (100000000U)   /* APB1/APB2 timer kernel clock */

enum {
    LED_GREEN = 0,  // LD1, PB0
    LED_BLUE,       // LD2, PB7
    LED_RED,        // LD3, PB14
    NUM_LEDS
};

enum {
    BUTTON_USER = 0, // B1, PC13 (active high, external pull-down)
    NUM_BUTTONS
};

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
