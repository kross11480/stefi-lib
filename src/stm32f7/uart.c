/**********************************************************************/
/* STM32F7 UART: USART3 on PD8/PD9 = ST-Link VCP of the NUCLEO-144    */
/* Same API as src/uart.c; the F7 USART register layout is the L4 one.*/
/**********************************************************************/
#include "libstefi/uart.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/dma_stream.h"
#include "libstefi/util.h"
#include "../internal/uart_internal.h"
#include "internal/rcc_f7.h"

#define USART3 ((UART_typeDef *) 0x40004800)

static const gpio_id_t port_uart_tx = D8;
static const gpio_id_t port_uart_rx = D9;
static UART_typeDef *const uart = USART3;

void uart_configure(){
    peripheral_uart_enable(PERIPHERAL_UART3);
    peripheral_gpio_enable(PERIPHERAL_GPIOD);
    gpio_set_alternate_function(port_uart_tx, AF7);
    gpio_set_alternate_function(port_uart_rx, AF7);
    gpio_set_mode(port_uart_tx, MODER_AF);
    gpio_set_mode(port_uart_rx, MODER_AF);

    /* Kernel clock from HSI (16 MHz, on after reset) instead of PCLK1, so
     * the baud rate does not change when system_init() switches to 200 MHz. */
    RCC_F7->CR |= BIT(RCC_CR_HSION);
    while (!(RCC_F7->CR & BIT(RCC_CR_HSIRDY)));
    RCC_F7->DCKCFGR2 = (RCC_F7->DCKCFGR2 & ~(0x3u << RCC_DCKCFGR2_USART3SEL_SHIFT))
                     | (0x2u << RCC_DCKCFGR2_USART3SEL_SHIFT);

    uart->BRR = (16000000 + 115200/2) / 115200;
    uart->CR1 |= (1 << 0) | (1 << 2) | (1 << 3); //uart, receive, Transmit Enable

    while (!(uart->ISR & BIT(21)) || !(uart->ISR & BIT(22)));   // TEACK, REACK
}

void uart_poll_out(const char *buf, uint32_t len)
{
    uint32_t i = 0;
    while ((uart->ISR & BIT(7)) && (i < len))
    {
        uart->TDR  = buf[i++];
        while (!(uart->ISR & BIT(6))); //Check Transmission Complete
    }
}

/* USART3_TX: DMA1 stream 3, channel 4 (RM0410 table 27). Blocking. */
void uart_dma_out(const char *buf, uint32_t len)
{
    uart->CR3 |= BIT(7);            // DMAT
    dma_mem_to_periph(3, 4, &uart->TDR, buf, len);
    while (!(uart->ISR & BIT(6)));  // TC
    uart->CR3 &= ~BIT(7);
}

void uart_putchar(char ch) {
    uart_poll_out(&ch, 1);
}

char uart_getchar() {
    return uart_poll_in();
}

unsigned char uart_poll_in()
{
    while ((uart->ISR & (1 << 5)) != (1 << 5));
    return (unsigned char) (uart->RDR & 0xFF);
}
