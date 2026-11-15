/*
 * UART_WakeUpFromStop: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, USART1 PA9/PA10 9600 8N1. Board _stop1 sleeps in Stop 1 four
 * times, woken in turn by: RXNE, start bit, 7-bit address 0x29, 4-bit
 * address 0x2; after each wakeup it reports back. Board _stimulus (start
 * with B1) sends the triggers and checks the reports.
 * LED2 on at the end: all four ok; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/usart.h>

#include <stdbool.h>
#include <string.h>

static const char *const reports[4] = {
    "RXNE wake-up successful",
    "Start bit detection wake-up successful",
    "7-bit address match wake-up successful",
    "4-bit address match wake-up successful",
};

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

#if defined(BOARD_IN_STOP1_MODE)
static void sleep_until(usart_wakeup_t event) {
    usart_enable_wakeup(PERIPHERAL_UART1, event);
    led_off(LED2);
    pwr_enter_stop1();
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);        //Stop wakes up on MSI
    systick_init(BOARD_SYSCLK);
    systick_start();
    usart_disable_wakeup(PERIPHERAL_UART1);
    systick_delay_ms(100);
}

void main() {
    board_init();
    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART1, A9, A10, &cfg);
    led_on(LED2);
    systick_delay_ms(2000);

    static const usart_wakeup_t events[4] = {USART_WAKEUP_RXNE, USART_WAKEUP_START_BIT,
                                             USART_WAKEUP_ADDRESS, USART_WAKEUP_ADDRESS};
    for (int i = 0; i < 4; i++) {
        if (i == 2) usart_wakeup_address(PERIPHERAL_UART1, 0x29, true);
        if (i == 3) usart_wakeup_address(PERIPHERAL_UART1, 0x2, false);
        sleep_until(events[i]);
        if (usart_write(PERIPHERAL_UART1, reports[i], strlen(reports[i]), 5000) != USART_OK) led_error_forever();
    }
    led_on(LED2);
    while(1);
}
#else
void main() {
    board_init();
    const usart_config_t cfg = {.baud = 9600, .word_bits = 8, .parity = USART_PARITY_NONE, .stop_bits = 1};
    usart_init(PERIPHERAL_UART1, A9, A10, &cfg);
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }

    static const uint8_t triggers[4] = {'R', 'S', 0xA9, 0x82};
    char rx[48];
    for (int i = 0; i < 4; i++) {
        led_on(LED2);
        size_t n = strlen(reports[i]);
        if (usart_write(PERIPHERAL_UART1, &triggers[i], 1, 5000) != USART_OK) led_error_forever();
        if (usart_read(PERIPHERAL_UART1, rx, n, 10000) != USART_OK) led_error_forever();
        led_off(LED2);
        if (memcmp(rx, reports[i], n) != 0) led_error_forever();
        systick_delay_ms(2000);
    }
    led_on(LED2);
    while(1);
}
#endif
