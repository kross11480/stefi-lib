/*
 * SPI_FullDuplex_ComDMA: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * Two boards, SPI1 PA5/PA6/PA7 full duplex by DMA; build variants master / slave.
 * Master starts when PA12 is jumpered to GND. LED2 on: data ok; blinking: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/gpio.h>
#include <libstefi/spi.h>
#include <libstefi/systick.h>

#include <stddef.h>
#include <string.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* LED2 on: passed */
static void led_success_forever() {
    led_on(LED2);
    while(1);
}

static const uint8_t tx_buffer[] =
    "****SPI - Two Boards communication based on DMA **** SPI Message ******** SPI Message ******** SPI Message ****";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

static volatile enum { WAIT, COMPLETE, ERROR } state = WAIT;

static void on_done(void *aux, bool error) {
    (void) aux;
    state = error ? ERROR : COMPLETE;
}

void main() {
    board_init();

    spi_gpio_init_pins(A5, A6, A7);
#if defined(SPI_MASTER_BOARD)
    const spi_config_t cfg = {.master = true, .baud_div_log2 = 5, .mode = 0};
    spi_configure(PERIPHERAL_SPI1, &cfg);
    while (!button_is_pressed(BUTTON_D2)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
#else
    const spi_config_t cfg = {.master = false, .mode = 0};
    spi_configure(PERIPHERAL_SPI1, &cfg);
#endif

    spi_transfer_dma(PERIPHERAL_SPI1, tx_buffer, rx_buffer, BUFFERSIZE, on_done, NULL);
    while (state == WAIT);

    if (state == ERROR || memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) led_error_forever();
    led_success_forever();
}
