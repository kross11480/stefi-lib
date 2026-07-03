#pragma once

#include "stefi_board.h"

/*
 * RobOhmobil ("Rohmi") mainboard — NUCLEO-G431KB
 * Pin mapping from Schematic_RobOhmobil_V01 (Rohmi_Extended_Mainboard).
 *
 * NOTE: DIR MOTOR L is PF0 (and expansion pin PF1) — requires port F support
 * in libstefi (gpio_id_t: F0 = 0x500, F1; peripheral: PERIPHERAL_GPIOF).
 */

/* Boot clock: the G431 resets onto the 16 MHz HSI16. Passed to
 * systick_init(); update the SysTick (and UART/I2C) setup if the
 * clock is reconfigured. */
#define BOARD_SYSCLK (16000000U)

/* Device instances — indices into the interface config tables
 * (stefi_led_t / stefi_button_t are typedef'd in stefi_board.h;
 * MOTOR_L/R and LINE_SENSOR_L/R come from the interface too). */
enum {
    LED1 = 0,       // PA10 — shared net with the buzzer!
    LED_NUCLEO,     // LD2 on the G431KB (PB8)
    NUM_LEDS
};

/* No button is populated on the Rohmi board or the NUCLEO-G431KB. The
 * default mapping points at free expansion pins (PB4/PB5) so the button
 * component works out of the box — wire external buttons to GND there. */
enum {
    BUTTON_USER0 = 0,
    BUTTON_USER1,
    NUM_BUTTONS
};

/* HC-SR04-style ranger on J6. Echo is level-shifted 5V->3V3 on-board and
 * routes to TIM4 CH1 for input-capture pulse measurement. */
typedef struct {
    gpio_id_t trigger_portpin;
    gpio_id_t echo_portpin;
    tim_id_t echo_timer_id;
    uint32_t echo_channel;
    afr_t echo_af_mode;
} ultrasound_config_t;

/* Buzzer shares PA10 with LED1: any beep also lights the LED.
 * Tone generation: TIM1 CH3 (AF6) PWM, or plain GPIO toggling. */
typedef struct {
    gpio_id_t portpin;
    tim_id_t timer_id;
    uint32_t channel;
    afr_t af_mode;
} buzzer_config_t;

/* Pins routed to the (currently unused) expansion socket; free for apps. */
#define NUM_GPIO_PINS 12

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
extern ultrasound_config_t ultrasound;
extern buzzer_config_t buzzer;
extern freepin_config_t freepin[NUM_GPIO_PINS];
