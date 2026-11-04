/*
 * UART_TwoBoards_ComPolling: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, USART1 TX PA9 -> RX PA10 both ways (+ GND), 9600 8N1, Polling.
 * Build variants _tx (starts on a B1 press) / _rx.
 * LED2 on: message ok on both boards; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/systick.h>
#include <libstefi/usart.h>

#include <stddef.h>
#include <string.h>

static const uint8_t tx_buffer[] = " **** UART_TwoBoards_ComPolling ****  **** UART_TwoBoards_ComPolling ****  **** UART_TwoBoards_ComPolling **** ";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

void main() {
    board_init();
    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART1, A9, A10, &cfg);

#if defined(TRANSMITTER_BOARD)
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    if (usart_write(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, 5000) != USART_OK) led_error_forever();
    if (usart_read(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, 5000) != USART_OK) led_error_forever();
#else
    if (usart_read(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, 0x1FFFFFF) != USART_OK) led_error_forever();
    if (usart_write(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, 5000) != USART_OK) led_error_forever();
#endif

    if (usart_errors(PERIPHERAL_UART1) || memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) {
        led_error_forever();
    }
    led_on(LED2);
    while(1);
}
