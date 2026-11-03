#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"

/**********************************************************************/
/* USART1/USART2 driver (STM32L4 / STM32G4): any baud rate, parity,   */
/* polling, interrupt and DMA transfers.                              */
/*                                                                    */
/* Kernel clock is HSI16 (independent of SYSCLK), as for uart.h —     */
/* which keeps using USART2 for printf; do not drive USART2 with both */
/* APIs at the same time.                                             */
/*                                                                    */
/* word_bits counts the parity bit, as in ST's HAL: 8 + parity = 7    */
/* data bits + parity ("7O1" on the terminal), 9 + parity = 8 data.   */
/**********************************************************************/

typedef enum { USART_PARITY_NONE = 0, USART_PARITY_EVEN, USART_PARITY_ODD } usart_parity_t;

typedef struct {
    uint32_t baud;
    uint8_t word_bits;          /* 7, 8 or 9, including the parity bit */
    usart_parity_t parity;
    uint8_t stop_bits;          /* 1 or 2 */
} usart_config_t;

typedef void (*usart_callback_t)(void *aux);

typedef enum {
    USART_OK = 0,
    USART_TIMEOUT,
    USART_ERROR,                /* framing / parity / overrun / noise */
} usart_status_t;

/**
 * @brief Pins (AF7), clock and frame format; transmitter and receiver on.
 * @param id  PERIPHERAL_UART1 or PERIPHERAL_UART2
 */
void usart_init(peripheral_uart_t id, gpio_id_t tx, gpio_id_t rx, const usart_config_t *cfg);

/* ---- polling (timeout in ms, measured with SysTick: systick must run) ---- */
usart_status_t usart_write(peripheral_uart_t id, const void *buf, uint32_t len, uint32_t timeout_ms);
usart_status_t usart_read(peripheral_uart_t id, void *buf, uint32_t len, uint32_t timeout_ms);

/* ---- interrupt driven (non-blocking, callbacks in interrupt context) ---- */
void usart_write_it(peripheral_uart_t id, const void *buf, uint32_t len, usart_callback_t done, void *aux);
void usart_read_it(peripheral_uart_t id, void *buf, uint32_t len, usart_callback_t done, void *aux);

/* ---- DMA (non-blocking; DMA1 channels fixed per MCU, see usart.c) ---- */
void usart_write_dma(peripheral_uart_t id, const void *buf, uint16_t len, usart_callback_t done, void *aux);
void usart_read_dma(peripheral_uart_t id, void *buf, uint16_t len, usart_callback_t done, void *aux);

bool usart_tx_busy(peripheral_uart_t id);
bool usart_rx_busy(peripheral_uart_t id);
/** Error flags seen since init (PE [0], FE [1], NE [2], ORE [3]); 0 = none. */
uint32_t usart_errors(peripheral_uart_t id);
