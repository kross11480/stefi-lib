#include "board.h"

#include <libstefi/gpio.h>
#include <libstefi/peripheral.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

#include <stdint.h>
#include <stdio.h>

/*
 * UART4 loopback diagnostic for the RN4020 link (PA0=TX, PA1=RX).
 *
 * It transmits "PINGn" on UART4 once a second and prints whatever UART4
 * receives, over the VCP (USART2). Used to prove the MCU's UART4 RX path
 * independently of the RN4020:
 *
 *   1. Remove the RN4020's green (PA1) and yellow (PA0) wires.
 *   2. Jumper CON20AP ADC0 <-> ADC1 directly (PA0 <-> PA1).
 *   3. Flash this; the VCP should show "RX: PINGn" (MCU hears itself).
 *
 * RX: (nothing)  -> with the jumper in place means a UART4 RX problem;
 *                   without the jumper it's just the expected baseline.
 * RX: PINGn      -> UART4 TX+RX both work; the silent module is the fault.
 */

#define UART4_BASE   0x40004C00u
#define UART4_CR1    (*(volatile uint32_t *)(UART4_BASE + 0x00))
#define UART4_BRR    (*(volatile uint32_t *)(UART4_BASE + 0x0C))
#define UART4_ISR    (*(volatile uint32_t *)(UART4_BASE + 0x1C))
#define UART4_RDR    (*(volatile uint32_t *)(UART4_BASE + 0x24))
#define UART4_TDR    (*(volatile uint32_t *)(UART4_BASE + 0x28))

#define RCC_CR       (*(volatile uint32_t *)0x40021000u)
#define RCC_APB1ENR1 (*(volatile uint32_t *)0x40021058u)
#define RCC_CCIPR    (*(volatile uint32_t *)0x40021088u)

static void uart4_init(void) {
    RCC_CR |= (1u << 8);
    while (!(RCC_CR & (1u << 10))) {}
    RCC_CCIPR &= ~(0x3u << 6);
    RCC_CCIPR |=  (0x2u << 6);          // UART4SEL = HSI16
    RCC_APB1ENR1 |= (1u << 19);

    peripheral_gpio_enable(PERIPHERAL_GPIOA);
    gpio_set_alternate_function(A0, AF8);
    gpio_set_alternate_function(A1, AF8);
    gpio_set_mode(A0, MODER_AF);
    gpio_set_mode(A1, MODER_AF);

    UART4_BRR = (16000000u + 115200u / 2) / 115200u;
    UART4_CR1 = (1u << 0) | (1u << 2) | (1u << 3);
}

#define UART4_ICR    (*(volatile uint32_t *)(UART4_BASE + 0x20))

static void u4_putc(char c) { while (!(UART4_ISR & (1u << 7))) {} UART4_TDR = (uint8_t)c; }
static int  u4_getc(uint32_t ms) {
    uint32_t t0 = systick_get_ms();
    while (!(UART4_ISR & (1u << 5))) { if (systick_get_ms() - t0 >= ms) return -1; }
    return (int)(UART4_RDR & 0xFF);
}

void main(void) {
    board_init();
    uart4_init();
    UART4_ICR = 0xFFFFFFFFu;   // clear any pending error/status flags

    printf("\r\n=== UART4 single-byte loopback (PA0->PA1) ===\r\n");
    printf("jumper CON20AP ADC0<->ADC1; expect RX to echo TX\r\n");

    uint32_t n = 0;
    while (1) {
        char tx = (char)('A' + (n++ % 26));
        UART4_ICR = (1u << 3) | (1u << 2) | (1u << 1);  // clear ORE/NE/FE

        u4_putc(tx);                 // send one byte
        int r = u4_getc(50);         // read it straight back (no overrun)
        uint32_t isr = UART4_ISR & 0xFFFF;

        if (r < 0)
            printf("TX '%c' -> RX timeout      ISR=0x%04lX\r\n", tx, (unsigned long)isr);
        else
            printf("TX '%c' -> RX '%c' %-8s ISR=0x%04lX\r\n",
                   tx, (char)r, (r == tx) ? "OK" : "MISMATCH", (unsigned long)isr);

        systick_delay_ms(500);
    }
}
