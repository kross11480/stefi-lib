/**********************************************************************/
/* USART1/USART2 — STM32L4 / STM32G4 (same USART v2 register layout)  */
/**********************************************************************/
#include "libstefi/usart.h"
#include "libstefi/dma_channel.h"
#include "libstefi/interrupts.h"
#include "libstefi/systick.h"
#include "libstefi/util.h"
#include "internal/uart_internal.h"
#include "internal/rcc_internal.h"
#include "internal/mcu_id.h"

#include <stddef.h>

#define CR1_UE     BIT(0)
#define CR1_RE     BIT(2)
#define CR1_TE     BIT(3)
#define CR1_RXNEIE BIT(5)
#define CR1_TCIE   BIT(6)
#define CR1_TXEIE  BIT(7)
#define CR1_PS     BIT(9)
#define CR1_PCE    BIT(10)
#define CR1_M0     BIT(12)
#define CR1_M1     (1u << 28)
#define CR3_EIE    BIT(0)
#define CR3_DMAR   BIT(6)
#define CR3_DMAT   BIT(7)
#define ISR_ERR    0xFu        /* PE FE NE ORE */
#define ISR_RXNE   BIT(5)
#define ISR_TC     BIT(6)
#define ISR_TXE    BIT(7)

#define NUM_USART 2u

typedef struct {
    const uint8_t *tx; uint32_t tx_len, tx_pos;
    uint8_t *rx; uint32_t rx_len, rx_pos;
    volatile bool tx_busy, rx_busy;
    usart_callback_t tx_done, rx_done;
    void *tx_aux, *rx_aux;
    volatile uint32_t errors;
    uint8_t data_mask;
} usart_state_t;

static usart_state_t st[NUM_USART];

static UART_typeDef *const regs[NUM_USART] = {USART1, USART2};
static const nvic_source_t irqs[NUM_USART] = {(nvic_source_t) 37, (nvic_source_t) 38};

/*
 * DMA routing. G4: DMAMUX, any channel — the channels ST's examples use
 * (TX ch1, RX ch2). L4: fixed request table (RM0351 table 41): USART1
 * TX ch4 / RX ch5, USART2 TX ch7 / RX ch6, all CxS = 2.
 */
static void dma_route(peripheral_uart_t id, bool tx, uint8_t *ch, uint8_t *req)
{
    if (mcu_is_g4()) {
        *ch = tx ? 1u : 2u;
        *req = (uint8_t) (24u + 2u * id + (tx ? 1u : 0u));   // USART1_RX 24 .. USART2_TX 27
    } else {
        static const uint8_t l4_ch[2][2] = {{5, 4}, {6, 7}};   // [id][tx]
        *ch = l4_ch[id][tx ? 1 : 0];
        *req = 2u;
    }
}

void usart_init(peripheral_uart_t id, gpio_id_t tx, gpio_id_t rx, const usart_config_t *cfg)
{
    assert(id < NUM_USART);
    UART_typeDef *u = regs[id];
    peripheral_uart_enable(id);
    peripheral_gpio_enable(gpio_get_port_from_portpin(tx));
    peripheral_gpio_enable(gpio_get_port_from_portpin(rx));
    gpio_set_alternate_function(tx, AF7);
    gpio_set_alternate_function(rx, AF7);
    gpio_set_mode(tx, MODER_AF);
    gpio_set_mode(rx, MODER_AF);
    gpio_set_pupd(rx, PULL_UP);       // idle-high line when unconnected

    // kernel clock HSI16: CCIPR USARTxSEL (2 bits at 2*id) = 0b10
    RCC->CR |= BIT(8);
    while (!(RCC->CR & BIT(10)));
    RCC->CCIPR = (RCC->CCIPR & ~(0x3u << (2u * id))) | (0x2u << (2u * id));

    u->CR1 = 0;
    uint32_t cr1 = 0;
    if (cfg->word_bits == 9) cr1 |= CR1_M0;
    if (cfg->word_bits == 7) cr1 |= CR1_M1;
    if (cfg->parity != USART_PARITY_NONE) cr1 |= CR1_PCE;
    if (cfg->parity == USART_PARITY_ODD) cr1 |= CR1_PS;
    u->CR2 = (cfg->stop_bits == 2) ? (0x2u << 12) : 0u;
    u->CR3 = 0;
    u->BRR = (16000000u + cfg->baud / 2u) / cfg->baud;
    u->ICR = 0xFFFFFFFFu;

    // data bits actually carried (parity bit masked off on reception)
    uint8_t data_bits = cfg->word_bits - (cfg->parity != USART_PARITY_NONE ? 1u : 0u);
    st[id] = (usart_state_t) {.data_mask = (uint8_t) ((1u << (data_bits > 8 ? 8 : data_bits)) - 1u)};

    u->CR1 = cr1 | CR1_TE | CR1_RE | CR1_UE;
    while (!(u->ISR & BIT(21)) || !(u->ISR & BIT(22)));   // TEACK, REACK
}

static bool timed_out(uint32_t start, uint32_t timeout_ms)
{
    return (systick_get_ms() - start) >= timeout_ms;
}

usart_status_t usart_write(peripheral_uart_t id, const void *buf, uint32_t len, uint32_t timeout_ms)
{
    UART_typeDef *u = regs[id];
    const uint8_t *p = buf;
    uint32_t t0 = systick_get_ms();
    for (uint32_t i = 0; i < len; i++) {
        while (!(u->ISR & ISR_TXE)) if (timed_out(t0, timeout_ms)) return USART_TIMEOUT;
        u->TDR = p[i];
    }
    while (!(u->ISR & ISR_TC)) if (timed_out(t0, timeout_ms)) return USART_TIMEOUT;
    return USART_OK;
}

usart_status_t usart_read(peripheral_uart_t id, void *buf, uint32_t len, uint32_t timeout_ms)
{
    UART_typeDef *u = regs[id];
    uint8_t *p = buf;
    uint32_t t0 = systick_get_ms();
    for (uint32_t i = 0; i < len; i++) {
        while (!(u->ISR & ISR_RXNE)) {
            if (u->ISR & ISR_ERR) { st[id].errors |= u->ISR & ISR_ERR; u->ICR = ISR_ERR; return USART_ERROR; }
            if (timed_out(t0, timeout_ms)) return USART_TIMEOUT;
        }
        p[i] = (uint8_t) u->RDR & st[id].data_mask;
    }
    return USART_OK;
}

void usart_write_it(peripheral_uart_t id, const void *buf, uint32_t len, usart_callback_t done, void *aux)
{
    usart_state_t *s = &st[id];
    s->tx = buf; s->tx_len = len; s->tx_pos = 0;
    s->tx_done = done; s->tx_aux = aux; s->tx_busy = true;
    interrupts_enable_source(irqs[id]);
    regs[id]->CR1 |= CR1_TXEIE;
}

void usart_read_it(peripheral_uart_t id, void *buf, uint32_t len, usart_callback_t done, void *aux)
{
    usart_state_t *s = &st[id];
    s->rx = buf; s->rx_len = len; s->rx_pos = 0;
    s->rx_done = done; s->rx_aux = aux; s->rx_busy = true;
    regs[id]->ICR = ISR_ERR;
    interrupts_enable_source(irqs[id]);
    regs[id]->CR3 |= CR3_EIE;
    regs[id]->CR1 |= CR1_RXNEIE;
}

static void handle_irq(peripheral_uart_t id)
{
    UART_typeDef *u = regs[id];
    usart_state_t *s = &st[id];
    uint32_t isr = u->ISR;
    uint32_t cr1 = u->CR1;

    if (isr & ISR_ERR) {
        s->errors |= isr & ISR_ERR;
        u->ICR = ISR_ERR;
    }
    if ((cr1 & CR1_RXNEIE) && (isr & ISR_RXNE)) {
        uint8_t c = (uint8_t) u->RDR & s->data_mask;
        if (s->rx_pos < s->rx_len) s->rx[s->rx_pos++] = c;
        if (s->rx_pos >= s->rx_len) {
            u->CR1 &= ~CR1_RXNEIE;
            u->CR3 &= ~CR3_EIE;
            s->rx_busy = false;
            if (s->rx_done) s->rx_done(s->rx_aux);
        }
    }
    if ((cr1 & CR1_TXEIE) && (isr & ISR_TXE)) {
        if (s->tx_pos < s->tx_len) {
            u->TDR = s->tx[s->tx_pos++];
        } else {
            u->CR1 = (u->CR1 & ~CR1_TXEIE) | CR1_TCIE;   // last byte: wait until shifted out
        }
    }
    if ((cr1 & CR1_TCIE) && (isr & ISR_TC)) {
        u->CR1 &= ~CR1_TCIE;
        u->ICR = ISR_TC;
        s->tx_busy = false;
        if (s->tx_done) s->tx_done(s->tx_aux);
    }
}

void USART1_IRQHandler(void) { handle_irq(PERIPHERAL_UART1); }
void USART2_IRQHandler(void) { handle_irq(PERIPHERAL_UART2); }

/* ------------------------------- DMA ------------------------------- */
static void on_tx_dma(void *aux, uint32_t flags)
{
    peripheral_uart_t id = (peripheral_uart_t) (uintptr_t) aux;
    if (!(flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE))) return;
    uint8_t ch, req;
    dma_route(id, true, &ch, &req);
    dma_channel_disable(PERIPHERAL_DMA1, ch);
    regs[id]->CR3 &= ~CR3_DMAT;
    // the DMA is done when the last byte is in TDR: finish on TC
    interrupts_enable_source(irqs[id]);
    regs[id]->CR1 |= CR1_TCIE;
}

static void on_rx_dma(void *aux, uint32_t flags)
{
    peripheral_uart_t id = (peripheral_uart_t) (uintptr_t) aux;
    if (!(flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE))) return;
    uint8_t ch, req;
    dma_route(id, false, &ch, &req);
    dma_channel_disable(PERIPHERAL_DMA1, ch);
    regs[id]->CR3 &= ~CR3_DMAR;
    usart_state_t *s = &st[id];
    if (s->data_mask != 0xFFu) {               // strip parity bits in place
        for (uint32_t i = 0; i < s->rx_len; i++) s->rx[i] &= s->data_mask;
    }
    s->rx_busy = false;
    if (s->rx_done) s->rx_done(s->rx_aux);
}

void usart_write_dma(peripheral_uart_t id, const void *buf, uint16_t len, usart_callback_t done, void *aux)
{
    usart_state_t *s = &st[id];
    s->tx_done = done; s->tx_aux = aux; s->tx_busy = true;
    uint8_t ch, req;
    dma_route(id, true, &ch, &req);
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = ch, .request = req,
        .dir = DMA_DIR_MEM_TO_PERIPH, .mem_inc = true,
        .priority = DMA_PRIORITY_LOW, .irq_tc = true, .irq_te = true,
    };
    dma_channel_register_handler(PERIPHERAL_DMA1, ch, on_tx_dma, (void *) (uintptr_t) id);
    dma_channel_configure(&cfg, &regs[id]->TDR, (void *) buf, len);
    regs[id]->ICR = ISR_TC;
    regs[id]->CR3 |= CR3_DMAT;
    dma_channel_enable(PERIPHERAL_DMA1, ch);
}

void usart_read_dma(peripheral_uart_t id, void *buf, uint16_t len, usart_callback_t done, void *aux)
{
    usart_state_t *s = &st[id];
    s->rx = buf; s->rx_len = len;
    s->rx_done = done; s->rx_aux = aux; s->rx_busy = true;
    uint8_t ch, req;
    dma_route(id, false, &ch, &req);
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = ch, .request = req,
        .dir = DMA_DIR_PERIPH_TO_MEM, .mem_inc = true,
        .priority = DMA_PRIORITY_HIGH, .irq_tc = true, .irq_te = true,
    };
    dma_channel_register_handler(PERIPHERAL_DMA1, ch, on_rx_dma, (void *) (uintptr_t) id);
    dma_channel_configure(&cfg, &regs[id]->RDR, buf, len);
    regs[id]->ICR = ISR_ERR;
    dma_channel_enable(PERIPHERAL_DMA1, ch);
    regs[id]->CR3 |= CR3_DMAR;
}

bool usart_tx_busy(peripheral_uart_t id) { return st[id].tx_busy; }
bool usart_rx_busy(peripheral_uart_t id) { return st[id].rx_busy; }
uint32_t usart_errors(peripheral_uart_t id) { return st[id].errors; }
