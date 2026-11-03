/*
 * UART_TwoBoards_ComPolling (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 * Built twice: TRANSMITTER_BOARD (g4_uart_twoboards_polling_tx) and without
 * (g4_uart_twoboards_polling_rx).
 *
 * Two boards, USART1 9600 8N1, polling mode with timeouts: TX PA9 -> RX
 * PA10 of the other board (both ways), GND <-> GND.
 * Board 1 (transmitter): waits for the jumper PA12 (Arduino D2) <-> GND
 *   (LED2 blinking fast), sends the message (5 s timeout), receives it
 *   back (5 s timeout), compares.
 * Board 2 (receiver): receives (long timeout), sends it back, compares.
 *   LED2 on:                     message identical on both boards
 *   LED2 blinking (1 s period):  timeout, wrong data or line error
 */
#include <string.h>

#include "g4_example.h"
#include "libstefi/usart.h"

static const uint8_t tx_buffer[] =
    " **** UART_TwoBoards_ComPolling ****  **** UART_TwoBoards_ComPolling ****  **** UART_TwoBoards_ComPolling **** ";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

void main(void)
{
    board_init();

    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART1, A9, A10, &cfg);

#if defined(TRANSMITTER_BOARD)
    while (!example_jumper_set()) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    if (usart_write(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, 5000) != USART_OK) example_error();
    if (usart_read(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, 5000) != USART_OK) example_error();
#else
    if (usart_read(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, 0x1FFFFFF) != USART_OK) example_error();
    if (usart_write(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, 5000) != USART_OK) example_error();
#endif

    if (memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) example_error();
    example_success();
}
