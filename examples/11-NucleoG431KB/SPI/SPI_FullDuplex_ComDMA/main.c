/*
 * SPI_FullDuplex_ComDMA_Master / _Slave (STM32CubeG4, NUCLEO-G431KB) on
 * stefi-lib. Built twice: SPI_MASTER_BOARD (g4_spi_fullduplex_dma_master)
 * and without (g4_spi_fullduplex_dma_slave).
 *
 * Two boards, SPI1: SCK PA5 <-> PA5, MISO PA6 <-> PA6, MOSI PA7 <-> PA7,
 * GND <-> GND. Mode 0, 8 bit, MSB first, software NSS, master clock
 * PCLK / 32 = 5.3 MHz. Both send the same message and receive the other's,
 * full duplex, by DMA (DMA1 ch2 RX, ch3 TX).
 * Master: waits (LED2 blinking 100 ms) until the jumper PA12 (Arduino D2)
 * <-> GND is set, then starts the transfer. Start the slave first.
 *   LED2 on:                     received buffer == sent buffer
 *   LED2 blinking (1 s period):  transfer error or wrong data
 */
#include <string.h>

#include "g4_example.h"
#include "libstefi/spi.h"

static const uint8_t tx_buffer[] =
    "****SPI - Two Boards communication based on DMA **** SPI Message ******** SPI Message ******** SPI Message ****";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

static volatile enum { WAIT, COMPLETE, ERROR } state = WAIT;

static void on_done(void *aux, bool error)
{
    (void) aux;
    state = error ? ERROR : COMPLETE;
}

void main(void)
{
    board_init();

    spi_gpio_init_pins(A5, A6, A7);
#if defined(SPI_MASTER_BOARD)
    const spi_config_t cfg = {.master = true, .baud_div_log2 = 5, .mode = 0};
    spi_configure(PERIPHERAL_SPI1, &cfg);
    while (!example_jumper_set()) {
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

    if (state == ERROR || memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) example_error();
    example_success();
}
