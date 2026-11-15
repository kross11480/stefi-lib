/*
 * UART_TwoBoards_ComIT: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * Two boards, USART1 PA9/PA10 9600 8N1, interrupt mode; build variants _tx / _rx.
 * Transmitter starts on the PA12-GND jumper. LED2 on: message ok; blinking: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/systick.h>
#include <libstefi/usart.h>

#include <stddef.h>
#include <string.h>

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

static const uint8_t tx_buffer[] =
    " ****UART_TwoBoards_ComIT****  ****UART_TwoBoards_ComIT****  ****UART_TwoBoards_ComIT**** ";
#define BUFFERSIZE (sizeof tx_buffer - 1u)
static uint8_t rx_buffer[BUFFERSIZE];

void main() {
    board_init();

    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART1, A9, A10, &cfg);

#if defined(TRANSMITTER_BOARD)
    while (!button_is_pressed(BUTTON_D2)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    usart_write_it(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, NULL, NULL);
    while (usart_tx_busy(PERIPHERAL_UART1));
    usart_read_it(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, NULL, NULL);
    while (usart_rx_busy(PERIPHERAL_UART1));
#else
    usart_read_it(PERIPHERAL_UART1, rx_buffer, BUFFERSIZE, NULL, NULL);
    while (usart_rx_busy(PERIPHERAL_UART1)) {
        led_on(LED2);  systick_delay_ms(100);
        led_off(LED2); systick_delay_ms(100);
        led_on(LED2);  systick_delay_ms(100);
        led_off(LED2); systick_delay_ms(500);
    }
    usart_write_it(PERIPHERAL_UART1, tx_buffer, BUFFERSIZE, NULL, NULL);
    while (usart_tx_busy(PERIPHERAL_UART1));
#endif

    if (usart_errors(PERIPHERAL_UART1) || memcmp(tx_buffer, rx_buffer, BUFFERSIZE) != 0) led_error_forever();
    led_success_forever();
}
