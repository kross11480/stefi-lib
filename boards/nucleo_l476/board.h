#pragma once

#include "stefi_board.h"

/* Boot clock: the L476 resets onto the 4 MHz MSI. Passed to
 * systick_init(); update the SysTick (and UART/I2C) setup if the
 * clock is reconfigured (e.g. system_init() -> 80 MHz). */
#define BOARD_SYSCLK (4000000U)

/* Device instances — indices into the interface config tables
 * (stefi_led_t / stefi_button_t are typedef'd in stefi_board.h). */
enum {
    LED0_RED = 0,
    LED1_YELLOW,
    LED2_GREEN,
    LED3_BLUE,
    LED_NUCLEO,
    NUM_LEDS
};

enum {
    BUTTON_S0 = 0,
    BUTTON_S1,
    BUTTON_S2,
    BUTTON_S3,
    BUTTON_NUCLEO,
    NUM_BUTTONS
};

//FRAM Configuration: later freq, mode, ... (board-specific device)
typedef struct {
    peripheral_spi_t spi_id;
    gpio_id_t cs_portpin;
    gpio_id_t sck_portpin;
    gpio_id_t miso_portpin;
    gpio_id_t mosi_portpin;
} fm25cl64_config_t;

/* EMB SSD1306 OLED (board-specific device) */
typedef struct {
    i2c_id_t i2c_id;
    uint8_t i2c_addr;
    gpio_id_t scl;
    gpio_id_t sda;
    gpio_id_t display_switch_pin;
} ssd1306_config_t;

/* Header pins free for applications — i.e. not claimed by an on-board
 * peripheral (LEDs, buttons, FRAM, USART2/printf, EMB display). */
#define NUM_GPIO_PINS 30

extern led_config_t leds[NUM_LEDS];
extern button_config_t buttons[NUM_BUTTONS];
extern freepin_config_t freepin[NUM_GPIO_PINS];

/*
 * CON20AP (J1) expansion connector — only the pins broken out there that are
 * NOT already used by an on-board peripheral. The connector's other lines are
 * shared buses, so they are excluded here:
 *   SCK/MISO/MOSI/NSS  -> SPI1 / FRAM
 *   SCL/SDA            -> I2C2 / EMB display
 *   TXD/RXD (=PCS1/2)  -> USART2 / printf (PA2/PA3)
 *   TAS2 / INT2        -> buttons
 * These six pins also appear in freepin[]; CON20AP is just another physical
 * breakout of the same MCU pins (not a duplication bug).
 */
typedef enum {
    CON20AP_ADC0 = 0,   // PA0  — ADC1_IN5,  TIM2_CH1
    CON20AP_ADC1,       // PA1  — ADC1_IN6,  TIM2_CH2
    CON20AP_ECLK,       // PC8
    CON20AP_OC2,        // PC9
    CON20AP_INT0,       // PC10 — interrupt-capable
    CON20AP_OC1B,       // PC11
    NUM_CON20AP_PINS
} stefi_con20ap_t;

extern freepin_config_t con20ap[NUM_CON20AP_PINS];
