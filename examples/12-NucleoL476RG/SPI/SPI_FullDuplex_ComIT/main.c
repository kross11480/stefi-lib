/*
 * SPI_FullDuplex_ComIT: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, SPI1 SCK PB3 / MISO PB4 / MOSI PB5 (+ GND), full duplex, Interrupt.
 * Build variants _master / _slave (start the slave first); the master
 * starts on a B1 press. LED2 on: data ok; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/spi.h>
#include <libstefi/systick.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static const uint8_t tx_buffer[] = "****SPI - Two Boards communication based on Interrupt **** SPI Message ******** SPI Message ******** SPI Message ****";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

static volatile bool done, failed;

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

void transfer_done_cb(void *aux, bool error) {
    failed = error;
    done = true;
}

void main() {
    board_init();
    spi_gpio_init_pins(B3, B4, B5);
#if defined(MASTER_BOARD)
    const spi_config_t cfg = {.master = true, .baud_div_log2 = 8, .mode = 0};
    spi_configure(PERIPHERAL_SPI1, &cfg);
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
#else
    const spi_config_t cfg = {.master = false, .mode = 0};
    spi_configure(PERIPHERAL_SPI1, &cfg);
#endif

    spi_transfer_it(PERIPHERAL_SPI1, tx_buffer, rx_buffer, BUFFERSIZE, transfer_done_cb, NULL);
    while (!done);

    if (failed || memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
