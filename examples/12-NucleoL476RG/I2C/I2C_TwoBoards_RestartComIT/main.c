/*
 * I2C_TwoBoards_RestartComIT: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, I2C3 SCL PC0 / SDA PC1 (pull-ups), 10-bit address 0x30F, repeated start, interrupt.
 * Build variants _master / _slave; the master starts on a B1 press.
 * LED2 on: received message == sent message; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/i2c.h>
#include <libstefi/systick.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define I2C_ADDRESS 0x30F           //10-bit
#define I2C_TIMING  0x00D00E28      //for the 80 MHz PCLK

static const uint8_t tx_buffer[] = " ****I2C_TwoBoards communication (repeated start, interrupt)****  ****I2C_TwoBoards communication (repeated start, interrupt)**** ";
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

#if !defined(MASTER_BOARD)
static volatile bool transfer_done;

void on_address(void *aux, bool master_reads, uint8_t **buf, uint16_t *len) {
    *buf = master_reads ? (uint8_t *) tx_buffer : rx_buffer;
    *len = BUFFERSIZE;
}

void on_complete(void *aux, bool master_reads, uint16_t count) {
    if (master_reads) transfer_done = true;      //the read ends the transaction
}

static const i2c_listen_ops_t listen_ops = {on_address, on_complete};
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
    //one transaction: write the message, repeated start, read it back
    i2c_status_t st;
    do {
        i2c_master_transfer_it(3, I2C_ADDRESS, tx_buffer, BUFFERSIZE, rx_buffer, BUFFERSIZE, NULL, NULL);
        while (i2c_busy(3));
        st = i2c_status(3);
    } while (st == I2C_NACK);
    if (st != I2C_OK) led_error_forever();
#else
    //listen mode: the direction of each address match picks the buffer
    i2c_slave_listen(3, &listen_ops, NULL);
    while (!transfer_done);
    i2c_slave_listen_stop(3);
#endif

    if (memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
