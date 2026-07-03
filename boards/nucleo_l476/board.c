#include "board.h"

#include "button.h"
#include "led.h"
#include "ssd1306.h"

#include <stddef.h>
#include <libstefi/i2c.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

/*
 * Board-specific configuration with default peripheral mapping
 *
 * Note:
 *  Systick - used for delay functions
 *  Timer 3  - used for fading the green, blue LED.
 *  Timer 4  -
 *  Timer 5  -
 *  Timer 6  - used for Button debouncing.
 */


button_config_t buttons[NUM_BUTTONS] = {
    [BUTTON_S0] = {B0, PULL_UP},
    [BUTTON_S1] = {B1, PULL_UP},
    [BUTTON_S2] = {B2, PULL_UP},
    [BUTTON_S3] = {B3, PULL_UP},
    [BUTTON_NUCLEO] = {C13, NONE},
};

led_config_t leds[NUM_LEDS] = {
    [LED0_RED] = {C4, HIGH},
    [LED1_YELLOW] = {C5, HIGH},
    [LED2_GREEN] = {C6, HIGH, TIMER3, 1, AF2},
    [LED3_BLUE] = {C7, HIGH, TIMER3, 2, AF2},
    [LED_NUCLEO]= {A5, LOW},
};

_Static_assert(NUM_BUTTONS <= STEFI_MAX_BUTTONS,
        "button.c sizes its per-button state by STEFI_MAX_BUTTONS");

hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER] = {
    [DEBOUNCE_TIMER] = {TIMER6, 4000, 10},
};

fm25cl64_config_t fram ={
    .spi_id = PERIPHERAL_SPI1,
    .cs_portpin = A4,
    .sck_portpin = A5,
    .miso_portpin = A6,
    .mosi_portpin = A7,
};

ssd1306_config_t ssd1306 = {
    .i2c_id = 2,
    .i2c_addr = 0x3C,
    .scl = B10,
    .sda = B11,
    .display_switch_pin = B5,
};

/*
 * Pins genuinely free for applications: the header pins NOT already claimed
 * by an on-board peripheral. Removed vs. the raw header catalog because they
 * are used elsewhere in this file:
 *   A2/A3            USART2 (printf / ST-Link VCP)
 *   A4/A5/A6/A7      FRAM on SPI1 (A5 is also LED_NUCLEO)
 *   B0/B1/B2/B3,C13  buttons S0..S3 + NUCLEO button
 *   B5/B10/B11       EMB display (power switch / I2C2 SCL / I2C2 SDA)
 *   C4/C5/C6/C7      on-board LEDs
 */
freepin_config_t freepin[NUM_GPIO_PINS] = {
    /* Arduino-Connector CN9, right */
    {A10,   TIMER_INVALID,  0, NULL, 0},
    {B4, TIMER3,  1, NULL, 0},
    {A8, TIMER_INVALID,  0, NULL, 0},
    /* Arduino-Connector CN5, right */
    {A9, TIMER_INVALID,  0, NULL, 0},
    {B6, TIMER4,  1, NULL, 0},
    {B9, TIMER4,  4, NULL, 0},
    {B8, TIMER4,  3, NULL, 0},
    /* Arduino-Connector CN8, left */
    {A0, TIMER2,  1, PERIPHERAL_ADC1, 5},
    {A1, TIMER2,  2, PERIPHERAL_ADC1, 6},
    {C1, TIMER_INVALID,  0, PERIPHERAL_ADC1, 2},
    {C0, TIMER_INVALID,  0, PERIPHERAL_ADC1, 1},

    /*ST-Morphoconnectors,CN7 on outside, inside same as arduino, only port A,B,C */
    {C10,   TIMER_INVALID,  0, NULL, 0}, //UART4/USART3-capable
    {C12,   TIMER_INVALID,  0, NULL, 0}, //UART5-capable
    {A13,   TIMER_INVALID,  0, NULL, 0}, //SWDIO — using this disables SWD debug
    {A14, TIMER_INVALID,  0, NULL, 0},   //SWCLK — using this disables SWD debug
    {A15, TIMER_INVALID,  0, NULL, 0},
    {B7, TIMER_INVALID,  0, NULL, 0},
    {C14, TIMER_INVALID,  0, NULL, 0},   //OSC32_IN  — free only if no 32 kHz crystal
    {C15, TIMER_INVALID,  0, NULL, 0},   //OSC32_OUT — free only if no 32 kHz crystal
    {C2, TIMER_INVALID,  0, PERIPHERAL_ADC1, 3},
    {C3, TIMER_INVALID,  0, PERIPHERAL_ADC1, 4},

    /*ST-Morphoconnectors,CN10 on outside, inside same as arduino, only port A,B,C */
    {C8,   TIMER_INVALID,  0, NULL, 0},
    {A12, TIMER_INVALID,  0, NULL, 0},
    {A11, TIMER_INVALID,  0, NULL, 0},
    {B12, TIMER_INVALID,  0, NULL, 0},
    {B15, TIMER_INVALID,  0, NULL, 0},
    {B14, TIMER_INVALID,  0, NULL, 0},
    {B13, TIMER_INVALID,  0, NULL, 0},
    /*CN7,10 inside not covered by ARDUINO headers*/
    {C11,   TIMER_INVALID,  0, NULL, 0}, //UART4/USART3-capable
    {C9,   TIMER_INVALID,  0, NULL, 0},
};

/* Free expansion pins on CON20AP (J1), indexed by connector signal name —
 * see stefi_con20ap_t in board.h. ADC0/1 carry their TIM2/ADC1 routes; the
 * INT/OC/ECLK group is plain GPIO (efiCAN silkscreen names). */
freepin_config_t con20ap[NUM_CON20AP_PINS] = {
    [CON20AP_ADC0] = {A0,  TIMER2, 1, PERIPHERAL_ADC1, 5},
    [CON20AP_ADC1] = {A1,  TIMER2, 2, PERIPHERAL_ADC1, 6},
    [CON20AP_ECLK] = {C8,  TIMER_INVALID, 0, NULL, 0},
    [CON20AP_OC2]  = {C9,  TIMER_INVALID, 0, NULL, 0},
    [CON20AP_INT0] = {C10, TIMER_INVALID, 0, NULL, 0},
    [CON20AP_OC1B] = {C11, TIMER_INVALID, 0, NULL, 0},
};

void board_init() {
    //UART
    uart_configure();

    //initialize all leds, default, output not PWM
    for(uint32_t i = 0; i < NUM_LEDS; i++) {
        led_init(i, OUTPUT);
    }

    //initialize all leds, default, output not PWM
    for(uint32_t i = 0; i < NUM_BUTTONS; i++) {
        button_init(i);
    }

    //intialize default timers
    for(uint32_t i = 0; i < NUM_RESERVED_TIMER; i++) {
        timer_init(hardware_timers[i].timer);
        timer_set_period(hardware_timers[i].timer, hardware_timers[i].prescaler, hardware_timers[i].period);
    }

    //initialize systick timer
    systick_init(BOARD_SYSCLK);
    systick_start();

    //initialze display
    i2c_gpio_init(ssd1306.scl, ssd1306.sda);
    gpio_write(ssd1306.display_switch_pin, HIGH);
    gpio_set_mode(ssd1306.display_switch_pin, MODER_OUTPUT);
    gpio_write(ssd1306.display_switch_pin, LOW); //Display On
    ssd1306_init(ssd1306.i2c_id, ssd1306.i2c_addr);
}