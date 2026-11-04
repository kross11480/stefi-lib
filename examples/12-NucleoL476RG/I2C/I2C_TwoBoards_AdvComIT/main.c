/*
 * I2C_TwoBoards_AdvComIT: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, I2C3 PC0/PC1, 7-bit address 0x1F (HAL form 0x3E), interrupt mode.
 * The master sends a request (WRITE 0x34 / READ 0x12), a 2-byte length,
 * then the data; the slave acts on the request. Build variants _master / _slave.
 * LED2 on: both buffers exchanged correctly; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/i2c.h>
#include <libstefi/systick.h>

#include <stddef.h>
#include <string.h>

#define I2C_ADDRESS       0x1F
#define I2C_TIMING        0x00D00E28
#define MASTER_REQ_READ   0x12
#define MASTER_REQ_WRITE  0x34

static const uint8_t tx_buffer[] = " ****I2C_TwoBoards advanced communication based on IT****  ****I2C_TwoBoards advanced communication based on IT**** ";
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
/* one interrupt-driven transfer, retried while the other side NACKs */
static void master_send(const uint8_t *tx, uint16_t txn, uint8_t *rx, uint16_t rxn) {
    i2c_status_t st;
    do {
        i2c_master_transfer_it(3, I2C_ADDRESS, tx, txn, rx, rxn, NULL, NULL);
        while (i2c_busy(3));
        st = i2c_status(3);
    } while (st == I2C_NACK);
    if (st != I2C_OK) led_error_forever();
}
#endif

#if !defined(MASTER_BOARD)
static void slave_receive(uint8_t *buf, uint16_t n) {
    i2c_slave_receive_it(3, buf, n, NULL, NULL);
    while (i2c_busy(3));
    if (i2c_status(3) != I2C_OK) led_error_forever();
}
#endif

#if !defined(MASTER_BOARD)
static void slave_transmit(const uint8_t *buf, uint16_t n) {
    i2c_slave_transmit_it(3, buf, n, NULL, NULL);
    while (i2c_busy(3));
    if (i2c_status(3) != I2C_OK) led_error_forever();
}
#endif

void main() {
    board_init();
    const i2c_bus_config_t bus = {
        .bus = 3, .scl = C0, .sda = C1, .af = AF4, .clock = I2C_CLK_PCLK,
        .timing = I2C_TIMING, .own_address = I2C_ADDRESS,
    };
    i2c_bus_init(&bus);

#if defined(MASTER_BOARD)
    const uint16_t num = BUFFERSIZE;
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    //write: request, length, data
    uint8_t req = MASTER_REQ_WRITE;
    master_send(&req, 1, NULL, 0);
    master_send((const uint8_t *) &num, 2, NULL, 0);
    master_send(tx_buffer, BUFFERSIZE, NULL, 0);
    //read: request, length, data
    req = MASTER_REQ_READ;
    systick_delay_ms(10);
    master_send(&req, 1, NULL, 0);
    master_send((const uint8_t *) &num, 2, NULL, 0);
    master_send(NULL, 0, rx_buffer, BUFFERSIZE);
#else
    for (int round = 0; round < 2; round++) {
        uint8_t req;
        uint16_t num;
        slave_receive(&req, 1);
        slave_receive((uint8_t *) &num, 2);
        if (num != BUFFERSIZE) led_error_forever();
        if (req == MASTER_REQ_WRITE) slave_receive(rx_buffer, num);
        else if (req == MASTER_REQ_READ) slave_transmit(tx_buffer, num);
        else led_error_forever();
    }
#endif

    if (memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
