/*
 * I2C_WakeUpFromStop / I2C_WakeUpFromStop2: port of ST's examples (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, I2C3 PC0/PC1, 7-bit address 0x65 (HAL form 0xCA), I2C kernel
 * clock HSI16. The slave waits in Stop 1 (STOP_MODE=1) / Stop 2 (STOP_MODE=2);
 * the master's address match wakes it. Build variants _master / _slave.
 * LED2 on: message exchanged; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/i2c.h>
#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <stddef.h>
#include <string.h>

#define I2C_ADDRESS 0x65
#define I2C_TIMING  0x0020098E      //for the 16 MHz HSI kernel clock

static const uint8_t tx_buffer[] = " ****I2C_TwoBoards communication wake up from stop mode based on IT****  ****I2C_TwoBoards communication wake up from stop mode based on IT**** ";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

#if !defined(MASTER_BOARD)
static void wait_done() {
    while (i2c_busy(3)) {
#if STOP_MODE == 2
        pwr_enter_stop2();
#else
        pwr_enter_stop1();
#endif
    }
    if (i2c_status(3) != I2C_OK) led_error_forever();
}
#endif

void main() {
    board_init();
    const i2c_bus_config_t bus = {
        .bus = 3, .scl = C0, .sda = C1, .af = AF4, .clock = I2C_CLK_HSI16,
        .timing = I2C_TIMING, .own_address = I2C_ADDRESS,
    };
    i2c_bus_init(&bus);

#if defined(MASTER_BOARD)
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    i2c_status_t st;
    do {
        i2c_master_transfer_it(3, I2C_ADDRESS, tx_buffer, BUFFERSIZE, NULL, 0, NULL, NULL);
        while (i2c_busy(3));
        st = i2c_status(3);
    } while (st == I2C_NACK);
    systick_delay_ms(10);
    do {
        i2c_master_transfer_it(3, I2C_ADDRESS, NULL, 0, rx_buffer, BUFFERSIZE, NULL, NULL);
        while (i2c_busy(3));
        st = i2c_status(3);
    } while (st == I2C_NACK);
    if (st != I2C_OK) led_error_forever();
#else
    i2c_enable_wakeup(3);
    i2c_slave_receive_it(3, rx_buffer, BUFFERSIZE, NULL, NULL);
    wait_done();                        //sleeps in Stop until the master addresses us
    i2c_slave_transmit_it(3, tx_buffer, BUFFERSIZE, NULL, NULL);
    wait_done();
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
#endif

    if (memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
