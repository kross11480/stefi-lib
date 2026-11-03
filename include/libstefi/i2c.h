#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "peripheral.h"
#include "libstefi/gpio.h"

/**********************************************************************/
/* I2C Hardware Abstraction layer */
/**********************************************************************/

/* I2C initialization functions*/
void i2c_init(peripheral_i2c_t i2c_id);
void i2c_gpio_init(gpio_id_t scl_pin, gpio_id_t sda_pin);

/* I2C processing functions: readfrom, writeto*/

/**
 * @brief Read (len) byte from an I2C slave device and store in buf.
 * @details This is a blocking call
 * Example: reading 14 bytes from MPU Sensor with I2C Address 0x68
 * @code
 * i2c_readfrom(I2C_ID, MPU_ADDR, buf, 14);
 * @endcode
 */
void i2c_readfrom(peripheral_i2c_t i2c_id, uint8_t address7b, uint8_t *buf, uint32_t len);

/**
 * @brief Write (len) data from buffer to an I2C slave device.
 * @param repeat If, true no stop bit sent => repeated start (needed for atomic write reg/ read data use case)
 * Example:
 * @code
 * i2c_writeto(I2C_ID, LM75_ADDR, &reg_addr, 1, true);
 * i2c_readfrom(I2C_ID, LM75_ADDR, rx_value, 2);
 * @endcode
 */
void i2c_writeto(peripheral_i2c_t i2c_id, uint8_t address7b, uint8_t *buf, uint32_t len, bool repeat);

/**
 * @brief Helper for register-based i2c device. Write (len) byte from buffer to a register (or memory) address of I2C slave device.
 * @details This is a blocking call
 * Example: writing 1 command byte to Register with address 0x6B MPU Sensor with I2C Address 0x68
 * @code
 * i2c_writeto_reg(I2C_ID, MPU_ADDR, 0x6B, &byte, 1);
 * @endcode
 * Example: streaming image bytes to write-only GDDRAM data Register of SSD1306 OLED
 * @code
 * i2c_writeto_reg(I2C_ID, SSD1306_I2C_ADDR, 0x40, buffer, buff_size);
 * @endcode
 */
void i2c_writeto_reg(peripheral_i2c_t i2c_id, uint8_t address7b, uint8_t reg, uint8_t *buf, uint32_t len);

/**********************************************************************/
/* I2C transfer engine (STM32L4 / STM32G4, I2C v2): master and slave, */
/* 7/10-bit addresses, polling / interrupt / DMA, write-then-read     */
/* with repeated start, slave listen mode, wakeup from Stop.          */
/*                                                                    */
/* Addresses are plain 7-bit (0x00..0x7F) or 10-bit values — not the  */
/* "8-bit" left-shifted form ST's HAL uses (HAL 0x3E == 7-bit 0x1F).  */
/* `bus` is the instance number 1..3.                                 */
/**********************************************************************/

typedef enum { I2C_CLK_PCLK = 0, I2C_CLK_SYSCLK = 1, I2C_CLK_HSI16 = 2 } i2c_kernel_clock_t;

typedef struct {
    uint8_t bus;                 /* 1, 2, 3 */
    gpio_id_t scl, sda;
    afr_t af;                    /* usually AF4 */
    i2c_kernel_clock_t clock;
    uint32_t timing;             /* TIMINGR value for that kernel clock (CubeMX / RM table) */
    uint16_t own_address;        /* slave address; 0 = none */
    bool addr10;                 /* own and target addresses are 10-bit */
} i2c_bus_config_t;

typedef enum {
    I2C_OK = 0,
    I2C_NACK,                    /* address or data not acknowledged */
    I2C_TIMEOUT,
    I2C_ERROR,                   /* bus error / arbitration lost / overrun */
    I2C_IN_PROGRESS,
} i2c_status_t;

typedef void (*i2c_done_t)(void *aux, i2c_status_t status);

void i2c_bus_init(const i2c_bus_config_t *cfg);

/* ---- master: write tx[0..txn) then (repeated start) read rx[0..rxn) ---- */
i2c_status_t i2c_master_transfer(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                                 uint8_t *rx, uint16_t rxn, uint32_t timeout_ms);
void i2c_master_transfer_it(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                            uint8_t *rx, uint16_t rxn, i2c_done_t done, void *aux);
/** As _it, data moved by DMA (L4: I2C1 ch6/7, I2C2 ch4/5, I2C3 ch2/3; G4: ch2/3). */
void i2c_master_transfer_dma(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                             uint8_t *rx, uint16_t rxn, i2c_done_t done, void *aux);

/* ---- slave: answer the next transfer addressed to us ---- */
/** Polling; waits (up to timeout) for the master. Returns I2C_ERROR if the
 *  master asks for the other direction. */
i2c_status_t i2c_slave_receive(uint8_t bus, uint8_t *buf, uint16_t n, uint32_t timeout_ms);
i2c_status_t i2c_slave_transmit(uint8_t bus, const uint8_t *buf, uint16_t n, uint32_t timeout_ms);
void i2c_slave_receive_it(uint8_t bus, uint8_t *buf, uint16_t n, i2c_done_t done, void *aux);
void i2c_slave_transmit_it(uint8_t bus, const uint8_t *buf, uint16_t n, i2c_done_t done, void *aux);
void i2c_slave_receive_dma(uint8_t bus, uint8_t *buf, uint16_t n, i2c_done_t done, void *aux);
void i2c_slave_transmit_dma(uint8_t bus, const uint8_t *buf, uint16_t n, i2c_done_t done, void *aux);

/**
 * @brief Slave listen mode: on every address match `on_address` is asked
 * for the buffer of that transfer (write = master writes to us); when the
 * transfer ends (STOP, or a repeated start) `on_complete` gets the byte
 * count. Both in interrupt context. Listening continues until disabled.
 */
typedef struct {
    void (*on_address)(void *aux, bool master_reads, uint8_t **buf, uint16_t *len);
    void (*on_complete)(void *aux, bool master_reads, uint16_t count);
} i2c_listen_ops_t;
void i2c_slave_listen(uint8_t bus, const i2c_listen_ops_t *ops, void *aux);
void i2c_slave_listen_stop(uint8_t bus);

bool i2c_busy(uint8_t bus);
i2c_status_t i2c_status(uint8_t bus);

/** Address match wakes the MCU from Stop (kernel clock must be HSI16;
 *  Stop 2: I2C3 only). Sets HSI16 as the Stop wakeup clock. */
void i2c_enable_wakeup(uint8_t bus);
