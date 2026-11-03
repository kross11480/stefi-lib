/*
 * UART_HyperTerminal_DMA (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * USART2 (ST-Link VCP, PA2/PA3), 9600 baud, 7 data bits + odd parity,
 * 1 stop bit — set the terminal to 9600 7O1. By DMA (DMA1 ch1 TX, ch2 RX):
 *   1. a start message is sent,
 *   2. 10 characters typed in the terminal are received,
 *   3. they are sent back, followed by an end message.
 *   LED2 on:                     all transfers done
 *   LED2 blinking (1 s period):  framing/parity error on reception
 */
#include "g4_example.h"
#include "libstefi/usart.h"

#define RXBUFFERSIZE 10u

static const char start_message[] =
    "\n\r ****UART-Hyperterminal communication based on DMA****\n\r Enter 10 characters using keyboard :\n\r";
static const char end_message[] = "\n\r Example Finished\n\r";
static uint8_t rx_buffer[RXBUFFERSIZE];

static void wait_tx(void) { while (usart_tx_busy(PERIPHERAL_UART2)); }

void main(void)
{
    board_init();

    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_ODD, .stop_bits = 1};
    usart_init(PERIPHERAL_UART2, A2, A3, &cfg);

    usart_write_dma(PERIPHERAL_UART2, start_message, sizeof start_message - 1u, NULL, NULL);
    usart_read_dma(PERIPHERAL_UART2, rx_buffer, RXBUFFERSIZE, NULL, NULL);
    wait_tx();
    while (usart_rx_busy(PERIPHERAL_UART2));

    usart_write_dma(PERIPHERAL_UART2, rx_buffer, RXBUFFERSIZE, NULL, NULL);
    wait_tx();
    usart_write_dma(PERIPHERAL_UART2, end_message, sizeof end_message - 1u, NULL, NULL);
    wait_tx();

    if (usart_errors(PERIPHERAL_UART2) & 0x3u) example_error();   // PE / FE
    example_success();
}
