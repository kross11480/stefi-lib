/**********************************************************************/
/* I2C v2 transfer engine — STM32L4 (RM0351 §39) / STM32G4 (RM0440)   */
/*                                                                    */
/* One event-driven state machine per bus. Interrupt mode runs it     */
/* from the EV/ER interrupts, polling mode calls the same step        */
/* function in a loop, DMA mode lets the DMA move the data bytes.     */
/**********************************************************************/
#include "libstefi/i2c.h"
#include "libstefi/dma_channel.h"
#include "libstefi/interrupts.h"
#include "libstefi/systick.h"
#include "libstefi/util.h"
#include "internal/i2c_internal.h"
#include "internal/rcc_internal.h"
#include "internal/gpio_internal.h"
#include "internal/mcu_id.h"

#include <stddef.h>

#define CR1_PE      BIT(0)
#define CR1_TXIE    BIT(1)
#define CR1_RXIE    BIT(2)
#define CR1_ADDRIE  BIT(3)
#define CR1_NACKIE  BIT(4)
#define CR1_STOPIE  BIT(5)
#define CR1_TCIE    BIT(6)
#define CR1_ERRIE   BIT(7)
#define CR1_TXDMAEN BIT(14)
#define CR1_RXDMAEN BIT(15)
#define CR1_WUPEN   (1u << 18)
#define CR1_IRQS    (CR1_TXIE | CR1_RXIE | CR1_ADDRIE | CR1_NACKIE | CR1_STOPIE | CR1_TCIE | CR1_ERRIE)

#define CR2_RD_WRN  BIT(10)
#define CR2_ADD10   BIT(11)
#define CR2_START   BIT(13)
#define CR2_STOP    BIT(14)
#define CR2_NBYTES(n) ((uint32_t) (n) << 16)
#define CR2_RELOAD  (1u << 24)
#define CR2_AUTOEND (1u << 25)

#define ISR_TXE     BIT(0)
#define ISR_TXIS    BIT(1)
#define ISR_RXNE    BIT(2)
#define ISR_ADDR    BIT(3)
#define ISR_NACKF   BIT(4)
#define ISR_STOPF   BIT(5)
#define ISR_TC      BIT(6)
#define ISR_TCR     BIT(7)
#define ISR_ERRS    (BIT(8) | BIT(9) | BIT(10))   /* BERR ARLO OVR */
#define ISR_BUSY    BIT(15)
#define ISR_DIR     (1u << 16)

#define NUM_BUS 3u

typedef enum { ROLE_IDLE, ROLE_MASTER, ROLE_SLAVE_ONESHOT, ROLE_SLAVE_LISTEN } role_t;

typedef struct {
    role_t role;
    bool addr10, use_dma, irq_mode;
    uint16_t addr;
    const uint8_t *tx; uint16_t txn, txpos;
    uint8_t *rx; uint16_t rxn, rxpos;
    bool reading;                  /* master: in the read phase */
    bool slave_reads;              /* slave one-shot: master reads from us */
    bool slave_dir_read;           /* current slave transfer direction */
    volatile bool busy;
    volatile i2c_status_t status;
    i2c_done_t done; void *aux;
    const i2c_listen_ops_t *ops; void *ops_aux;
} bus_state_t;

static bus_state_t st[NUM_BUS];

static I2C_typeDef *const regs[NUM_BUS] = {I2C1, I2C2, I2C3};

static nvic_source_t ev_irq(uint8_t bus)
{
    static const uint8_t l4[3] = {31, 33, 72}, g4[3] = {31, 33, 92};
    return (nvic_source_t) (mcu_is_g4() ? g4[bus - 1u] : l4[bus - 1u]);
}

static void dma_route(uint8_t bus, bool tx, uint8_t *ch, uint8_t *req)
{
    if (mcu_is_g4()) {               // DMAMUX: I2C1_RX 16 .. I2C3_TX 21
        *ch = tx ? 2u : 3u;
        *req = (uint8_t) (16u + 2u * (bus - 1u) + (tx ? 1u : 0u));
    } else {                         // L4 CSELR = 3 on fixed channels
        static const uint8_t ch_tx[3] = {6, 4, 2}, ch_rx[3] = {7, 5, 3};
        *ch = tx ? ch_tx[bus - 1u] : ch_rx[bus - 1u];
        *req = 3u;
    }
}

void i2c_bus_init(const i2c_bus_config_t *cfg)
{
    uint8_t b = cfg->bus;
    I2C_typeDef *i2c = regs[b - 1u];
    peripheral_gpio_enable(gpio_get_port_from_portpin(cfg->scl));
    peripheral_gpio_enable(gpio_get_port_from_portpin(cfg->sda));
    const gpio_id_t pins[2] = {cfg->scl, cfg->sda};
    for (int i = 0; i < 2; i++) {
        gpio_set_output_type(pins[i], OPEN_DRAIN);
        gpio_set_pupd(pins[i], PULL_UP);
        gpio_set_output_speed(pins[i], VERY_HIGH_S);
        gpio_set_alternate_function(pins[i], cfg->af);
        gpio_set_mode(pins[i], MODER_AF);
    }
    // kernel clock: CCIPR I2C1SEL [13:12], I2C2SEL [15:14], I2C3SEL [17:16]
    uint32_t shift = 12u + 2u * (b - 1u);
    if (cfg->clock == I2C_CLK_HSI16) { RCC->CR |= BIT(8); while (!(RCC->CR & BIT(10))); }
    RCC->CCIPR = (RCC->CCIPR & ~(0x3u << shift)) | ((uint32_t) cfg->clock << shift);
    RCC->APB1ENR1 |= BIT(20 + b);                    // I2C1EN 21, I2C2EN 22, I2C3EN 23
    (void) RCC->APB1ENR1;

    i2c->CR1 = 0;
    i2c->TIMINGR = cfg->timing;
    i2c->OAR1 = 0;
    if (cfg->own_address) {
        i2c->OAR1 = cfg->addr10 ? ((cfg->own_address & 0x3FFu) | BIT(10))
                                : ((uint32_t) (cfg->own_address & 0x7Fu) << 1);
        i2c->OAR1 |= BIT(15);                        // OA1EN
    }
    i2c->CR2 = cfg->addr10 ? CR2_ADD10 : 0u;
    i2c->CR1 = CR1_PE;

    st[b - 1u] = (bus_state_t) {.addr10 = cfg->addr10, .status = I2C_OK};
    interrupts_enable_source(ev_irq(b));
    interrupts_enable_source((nvic_source_t) (ev_irq(b) + 1));   // ER = EV + 1
}

static uint32_t sadd(bus_state_t *s, uint16_t addr)
{
    return s->addr10 ? ((addr & 0x3FFu) | CR2_ADD10) : ((uint32_t) (addr & 0x7Fu) << 1);
}

/* program the next master phase (write or read) of up to 255 bytes */
static void master_phase(uint8_t bus, bool start)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    uint16_t left = s->reading ? (uint16_t) (s->rxn - s->rxpos) : (uint16_t) (s->txn - s->txpos);
    uint32_t cr2 = sadd(s, s->addr) | (s->reading ? CR2_RD_WRN : 0u);
    if (left > 255u) cr2 |= CR2_NBYTES(255) | CR2_RELOAD;
    else {
        cr2 |= CR2_NBYTES(left);
        bool last_phase = s->reading || s->rxn == 0;
        if (last_phase) cr2 |= CR2_AUTOEND;           // else: TC, then restart for the read
    }
    if (start) cr2 |= CR2_START;
    i2c->CR2 = cr2;
}

static void finish(uint8_t bus, i2c_status_t status)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    i2c->CR1 &= ~(CR1_TXIE | CR1_RXIE | CR1_TCIE | CR1_TXDMAEN | CR1_RXDMAEN);
    if (s->role != ROLE_SLAVE_LISTEN) i2c->CR1 &= ~CR1_IRQS;
    if (s->use_dma) {
        uint8_t ch, req;
        dma_route(bus, true, &ch, &req);  dma_channel_disable(PERIPHERAL_DMA1, ch);
        dma_route(bus, false, &ch, &req); dma_channel_disable(PERIPHERAL_DMA1, ch);
    }
    if (s->role != ROLE_SLAVE_LISTEN) s->role = ROLE_IDLE;
    s->status = status;
    s->busy = false;
    if (s->done) s->done(s->aux, status);
}

static void start_dma(uint8_t bus, bool tx, const uint8_t *buf, uint16_t n)
{
    I2C_typeDef *i2c = regs[bus - 1u];
    uint8_t ch, req;
    dma_route(bus, tx, &ch, &req);
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = ch, .request = req,
        .dir = tx ? DMA_DIR_MEM_TO_PERIPH : DMA_DIR_PERIPH_TO_MEM,
        .mem_inc = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_configure(&cfg, tx ? (volatile void *) &i2c->TXDR : (volatile void *) &i2c->RXDR,
                          (void *) buf, n);
    dma_channel_enable(PERIPHERAL_DMA1, ch);
    i2c->CR1 |= tx ? CR1_TXDMAEN : CR1_RXDMAEN;
}

/* one pass over the pending events; shared by interrupt and polling mode */
static void step(uint8_t bus)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    uint32_t isr = i2c->ISR;

    if (isr & ISR_ERRS) {
        i2c->ICR = ISR_ERRS;
        if (s->role != ROLE_IDLE) finish(bus, I2C_ERROR);
        return;
    }

    /* ---------------- master ---------------- */
    if (s->role == ROLE_MASTER) {
        if (isr & ISR_NACKF) {
            i2c->ICR = ISR_NACKF;
            s->status = I2C_NACK;                     // hardware sends STOP (AUTOEND) ...
            if (!(i2c->CR2 & CR2_AUTOEND)) i2c->CR2 |= CR2_STOP;   // ... or we do
        }
        if ((isr & ISR_TXIS) && !s->use_dma && s->txpos < s->txn) i2c->TXDR = s->tx[s->txpos++];
        if ((isr & ISR_RXNE) && !s->use_dma && s->rxpos < s->rxn) s->rx[s->rxpos++] = (uint8_t) i2c->RXDR;
        if (isr & ISR_TCR) {                          // 255-byte chunk done: reload
            if (s->use_dma) { if (s->reading) s->rxpos += 255; else s->txpos += 255; }
            master_phase(bus, false);
        }
        if (isr & ISR_TC) {                           // write phase done, read follows
            if (s->use_dma) s->txpos = s->txn;
            s->reading = true;
            if (s->use_dma) { i2c->CR1 &= ~CR1_TXDMAEN; start_dma(bus, false, s->rx, s->rxn); }
            master_phase(bus, true);                  // repeated start
        }
        if (isr & ISR_STOPF) {
            i2c->ICR = ISR_STOPF;
            i2c->CR2 = s->addr10 ? CR2_ADD10 : 0u;
            finish(bus, s->status == I2C_NACK ? I2C_NACK : I2C_OK);
        }
        return;
    }

    /* ---------------- slave ---------------- */
    if (s->role == ROLE_SLAVE_ONESHOT || s->role == ROLE_SLAVE_LISTEN) {
        if (isr & ISR_ADDR) {
            bool master_reads = (isr & ISR_DIR) != 0;
            if (s->role == ROLE_SLAVE_LISTEN) {
                // a repeated start ends the previous transfer
                if (s->slave_dir_read != master_reads && (s->rxpos || s->txpos) && s->ops->on_complete) {
                    s->ops->on_complete(s->ops_aux, s->slave_dir_read,
                                        s->slave_dir_read ? s->txpos : s->rxpos);
                }
                uint8_t *buf = NULL; uint16_t len = 0;
                if (s->ops->on_address) s->ops->on_address(s->ops_aux, master_reads, &buf, &len);
                if (master_reads) { s->tx = buf; s->txn = len; } else { s->rx = buf; s->rxn = len; }
                s->txpos = s->rxpos = 0;
            } else if (master_reads != s->slave_reads) {
                i2c->ICR = ISR_ADDR;
                finish(bus, I2C_ERROR);               // master wants the other direction
                return;
            }
            s->slave_dir_read = master_reads;
            if (master_reads) {
                i2c->ISR = ISR_TXE;                   // flush a stale TXDR byte
                if (s->use_dma) start_dma(bus, true, s->tx, s->txn);
                else i2c->CR1 |= CR1_TXIE;
            } else {
                if (s->use_dma) start_dma(bus, false, s->rx, s->rxn);
                else i2c->CR1 |= CR1_RXIE;
            }
            i2c->ICR = ISR_ADDR;                      // release the clock stretch
        }
        if ((isr & ISR_TXIS) && !s->use_dma) {
            i2c->TXDR = (s->txpos < s->txn) ? s->tx[s->txpos] : 0xFFu;
            s->txpos++;
        }
        if ((isr & ISR_RXNE) && !s->use_dma) {
            uint8_t c = (uint8_t) i2c->RXDR;
            if (s->rxpos < s->rxn) s->rx[s->rxpos] = c;
            s->rxpos++;
        }
        if (isr & ISR_NACKF) i2c->ICR = ISR_NACKF;    // master ends its read: normal
        if (isr & ISR_STOPF) {
            i2c->ICR = ISR_STOPF;
            i2c->ISR = ISR_TXE;
            i2c->CR1 &= ~(CR1_TXIE | CR1_RXIE);
            bool rd = s->slave_dir_read;
            uint16_t count;
            if (s->use_dma) {
                uint8_t ch, req;
                dma_route(bus, rd, &ch, &req);
                count = (uint16_t) ((rd ? s->txn : s->rxn) - dma_channel_get_items(PERIPHERAL_DMA1, ch));
                dma_channel_disable(PERIPHERAL_DMA1, ch);
                i2c->CR1 &= ~(CR1_TXDMAEN | CR1_RXDMAEN);
            } else {
                // a transmitter loads one byte more than the master takes
                count = rd ? (uint16_t) (s->txpos ? s->txpos - 1u : 0u) : s->rxpos;
            }
            if (s->role == ROLE_SLAVE_LISTEN) {
                if (s->ops->on_complete) s->ops->on_complete(s->ops_aux, rd, count);
                s->txpos = s->rxpos = 0;
            } else {
                uint16_t want = rd ? s->txn : s->rxn;
                finish(bus, count >= want ? I2C_OK : I2C_ERROR);
            }
        }
    }
}

static void irq(uint8_t bus)
{
    if (st[bus - 1u].irq_mode || st[bus - 1u].role == ROLE_SLAVE_LISTEN) step(bus);
    else regs[bus - 1u]->CR1 &= ~CR1_IRQS;           // polling owns the bus
}
void I2C1_EV_IRQHandler(void) { irq(1); }
void I2C1_ER_IRQHandler(void) { irq(1); }
void I2C2_EV_IRQHandler(void) { irq(2); }
void I2C2_ER_IRQHandler(void) { irq(2); }
void I2C3_EV_IRQHandler(void) { irq(3); }
void I2C3_ER_IRQHandler(void) { irq(3); }

/* ------------------------------ master API ------------------------------ */
static void master_begin(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                         uint8_t *rx, uint16_t rxn, bool irq_mode, bool dma, i2c_done_t done, void *aux)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    s->role = ROLE_MASTER; s->addr = addr;
    s->tx = tx; s->txn = txn; s->txpos = 0;
    s->rx = rx; s->rxn = rxn; s->rxpos = 0;
    s->reading = (txn == 0);
    s->use_dma = dma; s->irq_mode = irq_mode;
    s->done = done; s->aux = aux;
    s->status = I2C_IN_PROGRESS; s->busy = true;
    i2c->ICR = ISR_STOPF | ISR_NACKF | ISR_ERRS;
    if (dma) start_dma(bus, !s->reading, s->reading ? rx : tx, s->reading ? rxn : txn);
    if (irq_mode) {
        i2c->CR1 |= CR1_ERRIE | CR1_TCIE | CR1_STOPIE | CR1_NACKIE | (dma ? 0u : (CR1_TXIE | CR1_RXIE));
    }
    master_phase(bus, true);
}

i2c_status_t i2c_master_transfer(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                                 uint8_t *rx, uint16_t rxn, uint32_t timeout_ms)
{
    uint32_t t0 = systick_get_ms();
    while (regs[bus - 1u]->ISR & ISR_BUSY) {
        if (systick_get_ms() - t0 >= timeout_ms) return I2C_TIMEOUT;
    }
    master_begin(bus, addr, tx, txn, rx, rxn, false, false, NULL, NULL);
    while (st[bus - 1u].busy) {
        step(bus);
        if (systick_get_ms() - t0 >= timeout_ms) {
            regs[bus - 1u]->CR2 |= CR2_STOP;
            st[bus - 1u].role = ROLE_IDLE;
            st[bus - 1u].busy = false;
            return I2C_TIMEOUT;
        }
    }
    return st[bus - 1u].status;
}

void i2c_master_transfer_it(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                            uint8_t *rx, uint16_t rxn, i2c_done_t done, void *aux)
{
    master_begin(bus, addr, tx, txn, rx, rxn, true, false, done, aux);
}

void i2c_master_transfer_dma(uint8_t bus, uint16_t addr, const uint8_t *tx, uint16_t txn,
                             uint8_t *rx, uint16_t rxn, i2c_done_t done, void *aux)
{
    master_begin(bus, addr, tx, txn, rx, rxn, true, true, done, aux);
}

/* ------------------------------ slave API ------------------------------ */
static void slave_begin(uint8_t bus, bool master_reads, const uint8_t *tx, uint8_t *rx, uint16_t n,
                        bool irq_mode, bool dma, i2c_done_t done, void *aux)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    s->role = ROLE_SLAVE_ONESHOT;
    s->slave_reads = master_reads;
    s->tx = tx; s->txn = master_reads ? n : 0; s->txpos = 0;
    s->rx = rx; s->rxn = master_reads ? 0 : n; s->rxpos = 0;
    s->use_dma = dma; s->irq_mode = irq_mode;
    s->done = done; s->aux = aux;
    s->status = I2C_IN_PROGRESS; s->busy = true;
    i2c->ICR = ISR_STOPF | ISR_NACKF | ISR_ERRS | ISR_ADDR;
    if (irq_mode) i2c->CR1 |= CR1_ADDRIE | CR1_STOPIE | CR1_NACKIE | CR1_ERRIE;
}

static i2c_status_t slave_poll(uint8_t bus, uint32_t timeout_ms)
{
    uint32_t t0 = systick_get_ms();
    while (st[bus - 1u].busy) {
        step(bus);
        if (systick_get_ms() - t0 >= timeout_ms) {
            st[bus - 1u].role = ROLE_IDLE;
            st[bus - 1u].busy = false;
            return I2C_TIMEOUT;
        }
    }
    return st[bus - 1u].status;
}

i2c_status_t i2c_slave_receive(uint8_t bus, uint8_t *buf, uint16_t n, uint32_t timeout_ms)
{
    slave_begin(bus, false, NULL, buf, n, false, false, NULL, NULL);
    return slave_poll(bus, timeout_ms);
}

i2c_status_t i2c_slave_transmit(uint8_t bus, const uint8_t *buf, uint16_t n, uint32_t timeout_ms)
{
    slave_begin(bus, true, buf, NULL, n, false, false, NULL, NULL);
    return slave_poll(bus, timeout_ms);
}

void i2c_slave_receive_it(uint8_t bus, uint8_t *buf, uint16_t n, i2c_done_t done, void *aux)
{ slave_begin(bus, false, NULL, buf, n, true, false, done, aux); }
void i2c_slave_transmit_it(uint8_t bus, const uint8_t *buf, uint16_t n, i2c_done_t done, void *aux)
{ slave_begin(bus, true, buf, NULL, n, true, false, done, aux); }
void i2c_slave_receive_dma(uint8_t bus, uint8_t *buf, uint16_t n, i2c_done_t done, void *aux)
{ slave_begin(bus, false, NULL, buf, n, true, true, done, aux); }
void i2c_slave_transmit_dma(uint8_t bus, const uint8_t *buf, uint16_t n, i2c_done_t done, void *aux)
{ slave_begin(bus, true, buf, NULL, n, true, true, done, aux); }

void i2c_slave_listen(uint8_t bus, const i2c_listen_ops_t *ops, void *aux)
{
    bus_state_t *s = &st[bus - 1u];
    I2C_typeDef *i2c = regs[bus - 1u];
    s->role = ROLE_SLAVE_LISTEN;
    s->ops = ops; s->ops_aux = aux;
    s->use_dma = false; s->irq_mode = true;
    s->txpos = s->rxpos = 0;
    s->done = NULL;
    i2c->ICR = ISR_STOPF | ISR_NACKF | ISR_ERRS | ISR_ADDR;
    i2c->CR1 |= CR1_ADDRIE | CR1_STOPIE | CR1_NACKIE | CR1_ERRIE;
}

void i2c_slave_listen_stop(uint8_t bus)
{
    st[bus - 1u].role = ROLE_IDLE;
    regs[bus - 1u]->CR1 &= ~CR1_IRQS;
}

bool i2c_busy(uint8_t bus) { return st[bus - 1u].busy; }
i2c_status_t i2c_status(uint8_t bus) { return st[bus - 1u].status; }

void i2c_enable_wakeup(uint8_t bus)
{
    regs[bus - 1u]->CR1 |= CR1_WUPEN;
    RCC->CFGR |= BIT(15);                            // STOPWUCK: wake up on HSI16
    EXTI->IMR1 |= BIT(22 + bus);                     // L4: I2C1 23, I2C2 24, I2C3 25
}
