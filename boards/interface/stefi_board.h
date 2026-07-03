#pragma once

/*
 * Board interface — the contract between the component layer and a concrete
 * board implementation.
 *
 * Components compile against this header only; they never see a concrete
 * board. A concrete board (a subdirectory of boards/, or an application's
 * own board.c) provides:
 *   - definitions of the config tables declared extern below — only those
 *     for devices it actually has. Linking a component against a board that
 *     lacks its table fails with an undefined reference at link time, which
 *     is the intended capability check;
 *   - enumerator names for its device instances (LED0_RED, BUTTON_S0, ...,
 *     including the NUM_LEDS / NUM_BUTTONS counts) in its own board.h;
 *   - board_init().
 * The executable chooses the board implementation at link time by linking
 * stefi::board (the bundled board selected via STEFI_BOARD) or its own
 * board target.
 */

#include "libstefi/gpio.h"
#include "libstefi/timer.h"
#include "libstefi/peripheral.h"

#include <stdint.h>

/* Device IDs are indices into the config tables below. The concrete board's
 * header names them; instance names and counts differ per board. */
typedef uint32_t stefi_led_t;
typedef uint32_t stefi_button_t;

/* Components keep per-button state in a fixed-size table; a board must not
 * declare more buttons than this (each board.c _Static_asserts it). */
#define STEFI_MAX_BUTTONS 8

/* Reserved timers: the same contract on every board. */
typedef enum {
    DEBOUNCE_TIMER,
    NUM_RESERVED_TIMER,
} stefi_timer_t;

/* The motor and line-sensor components are written for a differential-drive
 * robot: exactly one left and one right instance of each. */
typedef enum {
    MOTOR_L = 0,
    MOTOR_R,
    NUM_MOTORS
} stefi_motor_t;

typedef enum {
    LINE_SENSOR_L = 0,
    LINE_SENSOR_R,
    NUM_LINE_SENSORS
} stefi_line_sensor_t;

typedef peripheral_i2c_t i2c_id_t;
typedef peripheral_adc_t adc_id_t;

typedef struct {
    gpio_id_t portpin;
    sig_t off_state;
    tim_id_t timer_id;
    uint32_t channel;
    afr_t af_mode;
} led_config_t;

//Button Confguration
typedef struct {
    gpio_id_t portpin;
    pupdr_t pull;
} button_config_t;

typedef struct {
    tim_id_t timer;
    uint32_t prescaler;
    uint32_t period;
} hardware_timer_config_t;

/* STSPIN240 dual brushed-DC driver: one PWM (speed) + one DIR (phase) per
 * motor. EN/FAULT and STBY/RESET are strapped on-board, not MCU-controlled. */
typedef struct {
    gpio_id_t pwm_portpin;
    tim_id_t timer_id;
    uint32_t channel;
    afr_t af_mode;
    gpio_id_t dir_portpin;
} motor_config_t;

/* Reflectance sensor module: 3V3, GND, digital signal out. */
typedef struct {
    gpio_id_t portpin;
} line_sensor_config_t;

/* LSM6DSO 6-axis IMU on I2C. */
typedef struct {
    i2c_id_t i2c_id;
    uint8_t i2c_addr;
    gpio_id_t scl;
    gpio_id_t sda;
    gpio_id_t int1_portpin;
} lsm6dso_config_t;

typedef struct {
    gpio_id_t portpin;
    tim_id_t timer_id;
    uint32_t timer_channel;
    adc_id_t *adc_id;
    uint32_t adc_channel;
} freepin_config_t;

/* Config tables implemented by the concrete board (sizes are the board's
 * NUM_* counts; boards without a device omit its table). */
extern led_config_t leds[];
extern button_config_t buttons[];
extern hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER];
extern motor_config_t motors[NUM_MOTORS];
extern line_sensor_config_t line_sensors[NUM_LINE_SENSORS];
extern lsm6dso_config_t imu;
extern freepin_config_t freepin[];

void board_init();
