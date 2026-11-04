/*
 * I2C_TwoBoards_ComPolling: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, I2C3 SCL PC0 / SDA PC1 (pull-ups), 10-bit address 0x30F, polling.
 * Build variants _master / _slave; the master starts on a B1 press.
 * LED2 on: received message == sent message; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/i2c.h>
#include <libstefi/systick.h>

#include <stddef.h>
#include <string.h>

#define I2C_ADDRESS 0x30F           //10-bit
#define I2C_TIMING  0x00D00E28      //for the 80 MHz PCLK

static const uint8_t tx_buffer[] = " ****I2C_TwoBoards communication (polling)****  ****I2C_TwoBoards communication (polling)**** ";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

#if defined(MASTER_BOARD)
/* LED2 blinks fast until B1 is pressed (and released) */
static void wait_for_button() {
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    while(button_is_pressed(BUTTON_USER));
}
#endif


void main() {
    board_init();
    const i2c_bus_config_t bus = {
        .bus = 3, .scl = C0, .sda = C1, .af = AF4, .clock = I2C_CLK_PCLK,
        .timing = I2C_TIMING, .own_address = I2C_ADDRESS, .addr10 = true,
    };
    i2c_bus_init(&bus);

#if defined(MASTER_BOARD)
    wait_for_button();
    //retry while the slave is not listening yet (address NACK)
    while (i2c_master_transfer(3, I2C_ADDRESS, tx_buffer, BUFFERSIZE, NULL, 0, 10000) == I2C_NACK);
    while (i2c_master_transfer(3, I2C_ADDRESS, NULL, 0, rx_buffer, BUFFERSIZE, 10000) == I2C_NACK);
#else
    if (i2c_slave_receive(3, rx_buffer, BUFFERSIZE, 10000) != I2C_OK) led_error_forever();
    if (i2c_slave_transmit(3, tx_buffer, BUFFERSIZE, 10000) != I2C_OK) led_error_forever();
#endif

    if (memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
