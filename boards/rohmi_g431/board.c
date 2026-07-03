#include "board.h"

#include "button.h"
#include "led.h"

#include <stddef.h>
#include <libstefi/i2c.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

/*
 * Board-specific configuration with default peripheral mapping
 * RobOhmobil V01 — NUCLEO-G431KB
 *
 * Note (timer budget):
 *  Systick - used for delay functions
 *  Timer 1  - buzzer/LED1 tone PWM (CH3 on PA10, AF6), optional
 *  Timer 2  - motor PWM: CH1 = left (PA0), CH2 = right (PA1)
 *  Timer 3  - free (expansion pin PB4 = CH1, AF2)
 *  Timer 4  - ultrasound echo input capture (CH1 on PB6, AF2)
 *  Timer 6  - used for Button debouncing
 */

led_config_t leds[NUM_LEDS] = {
    [LED1] = {A10, LOW, TIMER1, 3, AF6},   // shared with buzzer
    [LED_NUCLEO] = {B8, LOW},              // D13 position; DIR MOTOR R uses PB3
};

//not populated: default mapping for external buttons (to GND) on free pins
button_config_t buttons[NUM_BUTTONS] = {
    [BUTTON_USER0] = {B4, PULL_UP},
    [BUTTON_USER1] = {B5, PULL_UP},
};

_Static_assert(NUM_BUTTONS <= STEFI_MAX_BUTTONS,
        "button.c sizes its per-button state by STEFI_MAX_BUTTONS");

hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER] = {
    [DEBOUNCE_TIMER] = {TIMER6, 4000, 10},
};

motor_config_t motors[NUM_MOTORS] = {
    //            PWM  timer   ch  AF   DIR
    [MOTOR_L] = {A0, TIMER2, 1, AF1, F0},  // STSPIN240 PWMA / PHA
    [MOTOR_R] = {A1, TIMER2, 2, AF1, B3},  // STSPIN240 PWMB / PHB ("PB3 verwenden")
};

line_sensor_config_t line_sensors[NUM_LINE_SENSORS] = {
    [LINE_SENSOR_L] = {B7},
    [LINE_SENSOR_R] = {B0},                // also ADC1 channel 15
};

ultrasound_config_t ultrasound = {
    .trigger_portpin = A15,
    .echo_portpin = B6,
    .echo_timer_id = TIMER4,
    .echo_channel = 1,
    .echo_af_mode = AF2,
};

buzzer_config_t buzzer = {
    .portpin = A10,                        // shared with LED1
    .timer_id = TIMER1,
    .channel = 3,
    .af_mode = AF6,
};

lsm6dso_config_t imu = {
    .i2c_id = 2,                           // 1-based HAL id (i2c_* use id-1 indexing); I2C2
    .i2c_addr = 0x6B,                      // SA0 high (confirmed via WHO_AM_I=0x6C)
    .scl = A9,
    .sda = A8,
    .int1_portpin = A12,                   // INT2 not routed to the MCU
};

freepin_config_t freepin[NUM_GPIO_PINS] = {
    /* expansion socket pins (timer/ADC routes per DS12589 AF table) */
    {F1, TIMER_INVALID, 0, PERIPHERAL_ADC2, 10}, //AN
    {A11, TIMER_INVALID, 0, NULL, 0},            //RST
    {A4, TIMER3, 2, PERIPHERAL_ADC2, 17},        //CS    (SPI1)
    {A5, TIMER2, 1, PERIPHERAL_ADC2, 13},        //SCK   (SPI1)
    {A6, TIMER3, 1, PERIPHERAL_ADC2, 3},         //MISO  (SPI1)
    {A7, TIMER17, 1, PERIPHERAL_ADC2, 4},        //MOSI  (SPI1)
    {B4, TIMER3, 1, NULL, 0},                    //default Button USER0
    {B5, TIMER3, 2, NULL, 0},                    //default Button USER1
    {A2, TIMER2, 3, PERIPHERAL_ADC1, 3},         //TX, Reserved UART2 (printf!)
    {A3, TIMER2, 4, PERIPHERAL_ADC1, 4},         //RX, Reserved UART2 (printf!)
    {A9, TIMER2, 3, NULL, 0},                    //SCL, Reserved I2C2 (IMU)
    {A8, TIMER1, 1, NULL, 0},                    //SDA, Reserved I2C2 (IMU)
};

void board_init() {
    //UART (USART2/PA2/PA3 — ST-Link VCP)
    uart_configure();

    //initialize all leds, default, output not PWM
    for(uint32_t i = 0; i < NUM_LEDS; i++) {
        led_init(i, OUTPUT);
    }

    //initialize all buttons (default mapping on free pins, see buttons[])
    for(uint32_t i = 0; i < NUM_BUTTONS; i++) {
        button_init(i);
    }

    //intialize default timers
    for(uint32_t i = 0; i < NUM_RESERVED_TIMER; i++) {
        timer_init(hardware_timers[i].timer);
        timer_set_period(hardware_timers[i].timer, hardware_timers[i].prescaler, hardware_timers[i].period);
    }

    //motor driver: direction pins low, PWM channels at 0% duty
    for(uint32_t i = 0; i < NUM_MOTORS; i++) {
        uint16_t port = gpio_get_port_from_portpin(motors[i].dir_portpin);
        peripheral_gpio_enable(port);
        gpio_write(motors[i].dir_portpin, LOW);
        gpio_set_mode(motors[i].dir_portpin, MODER_OUTPUT);

        port = gpio_get_port_from_portpin(motors[i].pwm_portpin);
        peripheral_gpio_enable(port);
        gpio_set_mode(motors[i].pwm_portpin, MODER_AF);
        gpio_set_alternate_function(motors[i].pwm_portpin, motors[i].af_mode);
    }
    //both motors share TIMER2: 16 MHz/(16*100) = 10 kHz PWM, duty in percent
    timer_init(motors[MOTOR_L].timer_id);
    timer_set_period(motors[MOTOR_L].timer_id, 16, 100);
    for(uint32_t i = 0; i < NUM_MOTORS; i++) {
        timer_set_mode_pwm(motors[i].timer_id, motors[i].channel);
        timer_cc_enable(motors[i].timer_id, motors[i].channel, false);
        //OC polarity is active low (see timer_set_cc_io): compare == period
        //means 0% duty (motor off); speed s% -> compare = 100 - s
        timer_set_compare(motors[i].timer_id, motors[i].channel, 100);
    }
    timer_start(motors[MOTOR_L].timer_id);

    //line-following sensors: digital inputs
    for(uint32_t i = 0; i < NUM_LINE_SENSORS; i++) {
        uint16_t port = gpio_get_port_from_portpin(line_sensors[i].portpin);
        peripheral_gpio_enable(port);
        gpio_set_mode(line_sensors[i].portpin, MODER_INPUT);
    }

    //ultrasound: trigger idle-low output; echo input (capture set up by component)
    gpio_write(ultrasound.trigger_portpin, LOW);
    gpio_set_mode(ultrasound.trigger_portpin, MODER_OUTPUT);
    gpio_set_mode(ultrasound.echo_portpin, MODER_INPUT);

    //IMU interrupt pin
    gpio_set_mode(imu.int1_portpin, MODER_INPUT);

    //initialize systick timer
    systick_init(BOARD_SYSCLK);
    systick_start();

    //I2C2 bus for the IMU; IMU register setup is the component's job
    i2c_gpio_init(imu.scl, imu.sda);
}
