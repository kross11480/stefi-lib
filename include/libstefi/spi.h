#pragma once

#include <stdint.h>

#include "libstefi/peripheral.h"
#include "libstefi/gpio.h"

/**********************************************************************/
/* SPI Hardware Abstraction layer */
/**********************************************************************/

/* SPI initialization functions*/
void spi_init(const peripheral_spi_t spi_id);
void spi_gpio_init(gpio_id_t cs_pin, gpio_id_t sck_pin, gpio_id_t miso_pin, gpio_id_t mosi_pin);

/* SPI processing functions: SPI full duplex read, write*/
uint8_t spi_transfer(const peripheral_spi_t spi_id, uint8_t data);


/**********************************************************************/
/* Configurable SPI1 (STM32L4 / STM32G4): master or slave, any mode,  */
/* 8-bit frames, software NSS, full-duplex DMA transfers.             */
/**********************************************************************/
#include <stdbool.h>

typedef struct {
    bool master;
    uint8_t baud_div_log2;     /* master clock = PCLK / 2^n, n = 1..8 */
    uint8_t mode;              /* 0..3: CPOL = bit 1, CPHA = bit 0 */
    bool lsb_first;
} spi_config_t;

typedef void (*spi_callback_t)(void *aux, bool error);

/** Clock on, format set, peripheral enabled (software NSS: always selected). */
void spi_configure(const peripheral_spi_t spi_id, const spi_config_t *cfg);
/** SCK/MISO/MOSI on AF5, no hardware NSS. */
void spi_gpio_init_pins(gpio_id_t sck_pin, gpio_id_t miso_pin, gpio_id_t mosi_pin);

/**
 * @brief Full-duplex DMA transfer of @p len bytes (DMA1 ch2 = RX, ch3 = TX,
 * as in ST's examples; routed by DMAMUX on G4 / CSELR on L4). Non-blocking.
 * As a slave the transfer completes when the master has clocked all bytes.
 * @param done  called from the RX DMA interrupt (after BSY clears, as master)
 */
void spi_transfer_dma(const peripheral_spi_t spi_id, const void *tx, void *rx, uint16_t len,
                      spi_callback_t done, void *aux);
bool spi_busy(const peripheral_spi_t spi_id);
