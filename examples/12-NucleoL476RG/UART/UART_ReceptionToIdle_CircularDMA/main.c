/*
 * UART_ReceptionToIdle_CircularDMA: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * USART2 (VCP) 115200 8N1: reception into a 20-byte circular DMA buffer; at
 * half/full buffer and whenever the line goes idle the new characters are
 * handed over and echoed back. Type in a terminal.
 */

#include "board.h"
#include "led.h"

#include <libstefi/usart.h>

#include <stddef.h>

#define RX_BUFFER_SIZE 20u

static const char start_message[] = "\r\nUSART Example : Enter characters to fill reception buffers.\r\n";
static uint8_t rx_buffer[RX_BUFFER_SIZE];

/* new data in rx_buffer up to pos: echo what came since the last call */
void rx_event_cb(void *aux, uint16_t pos) {
    static uint16_t old_pos;
    if (pos == old_pos) return;
    if (pos > old_pos) {
        usart_write(PERIPHERAL_UART2, &rx_buffer[old_pos], pos - old_pos, 100);
    } else {                                     //wrapped around
        usart_write(PERIPHERAL_UART2, &rx_buffer[old_pos], RX_BUFFER_SIZE - old_pos, 100);
        usart_write(PERIPHERAL_UART2, rx_buffer, pos, 100);
    }
    old_pos = (pos == RX_BUFFER_SIZE) ? 0 : pos;
    led_toggle(LED2);
}

void main() {
    board_init();
    const usart_config_t cfg = {.baud = 115200, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART2, A2, A3, &cfg);
    usart_write(PERIPHERAL_UART2, start_message, sizeof start_message - 1u, 1000);
    usart_read_to_idle_circular(PERIPHERAL_UART2, rx_buffer, RX_BUFFER_SIZE, rx_event_cb, NULL);
    while(1);
}
