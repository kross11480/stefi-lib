/*
 * UART_HyperTerminal_DMA: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * USART2 (VCP) 9600 7O1 by DMA: prints a prompt, receives 10 chars, echoes them.
 */

#include "board.h"
#include "led.h"

#include <libstefi/systick.h>
#include <libstefi/usart.h>

#include <stddef.h>

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

#define RXBUFFERSIZE 10u

static const char start_message[] =
    "\n\r ****UART-Hyperterminal communication based on DMA****\n\r Enter 10 characters using keyboard :\n\r";
static const char end_message[] = "\n\r Example Finished\n\r";
static uint8_t rx_buffer[RXBUFFERSIZE];

static void wait_tx(void) { while (usart_tx_busy(PERIPHERAL_UART2)); }

void main() {
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

    if (usart_errors(PERIPHERAL_UART2) & 0x3u) led_error_forever();   // PE / FE
    led_success_forever();
}
