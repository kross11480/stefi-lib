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

/** Polling full-duplex transfer; false on timeout (ms, SysTick). */
bool spi_transfer_buffer(const peripheral_spi_t spi_id, const void *tx, void *rx, uint16_t len,
                         uint32_t timeout_ms);
/** Interrupt-driven full-duplex transfer (SPIx interrupt). Non-blocking. */
void spi_transfer_it(const peripheral_spi_t spi_id, const void *tx, void *rx, uint16_t len,
                     spi_callback_t done, void *aux);

/* ---- half duplex on one line (BIDIMODE): master MOSI <-> slave MISO ---- */
/** Configure like spi_configure(), 1-line bidirectional, transmit direction. */
void spi_configure_half_duplex(const peripheral_spi_t spi_id, const spi_config_t *cfg);
/** Direction of the shared line (only change while disabled). */
void spi_half_duplex_direction(const peripheral_spi_t spi_id, bool transmit);
void spi_enable(const peripheral_spi_t spi_id);
void spi_disable(const peripheral_spi_t spi_id);
/** Send one byte and wait until it is out (BSY clear). */
void spi_write_byte(const peripheral_spi_t spi_id, uint8_t val);
/**
 * @brief Receive one byte. As master in 1-line receive, enabling the SPI
 * starts the clock immediately; it is stopped again within the first byte
 * so exactly one byte is clocked (the RM0351 §40.4.13 procedure, as in ST's
 * example). The SPI is left disabled as master, enabled as slave.
 */
uint8_t spi_read_byte(const peripheral_spi_t spi_id, bool master);
