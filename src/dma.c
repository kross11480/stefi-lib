/**********************************************************************/
/* DMA  (Direct Memory Access)  Hardware Abstraction layer  — L4/G4   */
/* Implements libstefi/dma.h and libstefi/dma_channel.h.              */
/**********************************************************************/
#include "libstefi/dma.h"
#include "libstefi/dma_channel.h"
#include "libstefi/peripheral.h"
#include "libstefi/interrupts.h"
#include "libstefi/util.h"
#include "internal/dma_internal.h"
#include "internal/mcu_id.h"

/*
 * DMA_CCRx bit positions (RM0440 / RM0351):
 *   EN      [0]    channel enable — set LAST; this starts the transfer
 *   TCIE    [1]    transfer-complete interrupt enable
 *   HTIE    [2]    half-transfer interrupt enable
 *   TEIE    [3]    transfer-error interrupt enable
 *   DIR     [4]    0 = read from peripheral addr (CPAR) -> memory (CMAR)
 *   CIRC    [5]    circular mode
 *   PINC    [6]    increment the peripheral (CPAR) pointer after each item
 *   MINC    [7]    increment the memory   (CMAR) pointer after each item
 *   PSIZE   [9:8], MSIZE [11:10], PL [13:12]
 *   MEM2MEM [14]   memory-to-memory mode (runs without a peripheral request)
 */
#define CCR_EN        0u
#define CCR_TCIE      1u
#define CCR_HTIE      2u
#define CCR_TEIE      3u
#define CCR_DIR       4u
#define CCR_CIRC      5u
#define CCR_PINC      6u
#define CCR_MINC      7u
#define CCR_PSIZE     8u
#define CCR_MSIZE    10u
#define CCR_PL       12u
#define CCR_MEM2MEM  14u

/* Only DMA1 channels 1..7 have wired NVIC vectors (see startup.c). */
#define DMA1_NUM_CHANNELS 7u

static inline DMA_t *ctrl_of(peripheral_dma_t dma)
{
    return (dma == PERIPHERAL_DMA2) ? DMA2 : DMA1;
}

static inline uint32_t base_of(peripheral_dma_t dma)
{
    return (dma == PERIPHERAL_DMA2) ? DMA2_BASE : DMA1_BASE;
}

/* Each channel owns 4 bits in ISR/IFCR at (channel-1)*4: GIF/TCIF/HTIF/TEIF. */
static inline uint32_t channel_flag_shift(uint8_t channel)
{
    return (uint32_t)(channel - 1u) * 4u;
}

/*
 * Request routing — the L4/G4 seam.
 *
 * A peripheral transfer only advances when the peripheral asserts its DMA
 * request, so the channel must be told *which* request drives it:
 *   L4 (L476): DMA_CSELR (offset 0xA8 in the DMA block) holds a 4-bit selector
 *              per channel. USART2_TX = request 2 on DMA1 channel 7.
 *   G4 (G431): DMAMUX1 instead — DMAMUX1_CHANNEL(mux)->CCR = request_id, with
 *              mux channels 0..n-1 feeding DMA1 ch1..n, n..2n-1 feeding DMA2.
 * Which one applies is read from the device ID (internal/mcu_id.h).
 */
#define DMA_CSELR(base) (*(volatile uint32_t *)((base) + 0xA8u))

static void select_request(peripheral_dma_t dma, uint8_t channel, uint8_t request)
{
    if (mcu_is_g4()) {
        uint32_t mux = (uint32_t) (channel - 1u)
                     + ((dma == PERIPHERAL_DMA2) ? mcu_dma_channels() : 0u);
        DMAMUX1_CHANNEL(mux)->CCR = request;
    } else {
        uint32_t base = base_of(dma);
        uint32_t shift = (uint32_t)(channel - 1u) * 4u;   /* 4 bits per channel */
        DMA_CSELR(base) = (DMA_CSELR(base) & ~(0xFu << shift))
                        | ((uint32_t) (request & 0xFu) << shift);
    }
}

/*************************** dma_channel.h ***************************/
static struct {
    dma_channel_callback_t fn;
    void *aux;
} channel_handlers[DMA1_NUM_CHANNELS];

void dma_channel_disable(peripheral_dma_t dma, uint8_t channel)
{
    DMA_CHANNEL(ctrl_of(dma), channel)->CCR &= ~BIT(CCR_EN);
}

void dma_channel_enable(peripheral_dma_t dma, uint8_t channel)
{
    DMA_CHANNEL(ctrl_of(dma), channel)->CCR |= BIT(CCR_EN);
}

bool dma_channel_is_enabled(peripheral_dma_t dma, uint8_t channel)
{
    return (DMA_CHANNEL(ctrl_of(dma), channel)->CCR & BIT(CCR_EN)) != 0;
}

uint16_t dma_channel_get_items(peripheral_dma_t dma, uint8_t channel)
{
    return (uint16_t) DMA_CHANNEL(ctrl_of(dma), channel)->CNDTR;
}

nvic_source_t dma_channel_irq(peripheral_dma_t dma, uint8_t channel)
{
    assert(dma == PERIPHERAL_DMA1 && channel >= 1 && channel <= DMA1_NUM_CHANNELS);
    return (nvic_source_t) (INTERRUPT_SOURCE_DMA1_CH1 + (channel - 1));
}

void dma_channel_register_handler(peripheral_dma_t dma, uint8_t channel,
                                  dma_channel_callback_t fn, void *aux)
{
    assert(dma == PERIPHERAL_DMA1 && channel >= 1 && channel <= DMA1_NUM_CHANNELS);
    channel_handlers[channel - 1u].aux = aux;
    channel_handlers[channel - 1u].fn = fn;
}

void dma_channel_configure(const dma_channel_config_t *cfg, volatile const void *periph,
                           void *mem, uint16_t items)
{
    assert(cfg->channel >= 1 && cfg->channel <= 8);
    peripheral_dma_enable(cfg->dma);

    DMA_t *ctrl = ctrl_of(cfg->dma);
    DMA_Channel_t *ch = DMA_CHANNEL(ctrl, cfg->channel);
    ch->CCR &= ~BIT(CCR_EN);                    // CPAR/CMAR/CNDTR need EN = 0
    if (cfg->dir != DMA_DIR_MEM_TO_MEM) {
        select_request(cfg->dma, cfg->channel, cfg->request);
    }

    // DIR=1 means "read memory (CMAR), write peripheral (CPAR)". For M2M the
    // source sits in CPAR (DIR=0), as in dma_mem_to_mem().
    ch->CPAR  = (uint32_t) periph;
    ch->CMAR  = (uint32_t) mem;
    ch->CNDTR = items;

    uint32_t ccr = ((uint32_t) cfg->periph_width << CCR_PSIZE)
                 | ((uint32_t) cfg->mem_width << CCR_MSIZE)
                 | ((uint32_t) cfg->priority << CCR_PL);
    if (cfg->dir == DMA_DIR_MEM_TO_PERIPH) ccr |= BIT(CCR_DIR);
    if (cfg->dir == DMA_DIR_MEM_TO_MEM)    ccr |= BIT(CCR_MEM2MEM);
    if (cfg->periph_inc) ccr |= BIT(CCR_PINC);
    if (cfg->mem_inc)    ccr |= BIT(CCR_MINC);
    if (cfg->circular)   ccr |= BIT(CCR_CIRC);
    if (cfg->irq_tc)     ccr |= BIT(CCR_TCIE);
    if (cfg->irq_ht)     ccr |= BIT(CCR_HTIE);
    if (cfg->irq_te)     ccr |= BIT(CCR_TEIE);

    ctrl->IFCR = 0xFu << channel_flag_shift(cfg->channel);   // stale flags
    ch->CCR = ccr;

    if ((cfg->irq_tc || cfg->irq_ht || cfg->irq_te) && cfg->dma == PERIPHERAL_DMA1) {
        interrupts_enable_source(dma_channel_irq(cfg->dma, cfg->channel));
    }
}

/***************************** dma.h *****************************/
void dma_mem_to_mem(peripheral_dma_t dma, uint8_t channel,
                    void *dst, const void *src, uint32_t len)
{
    const dma_channel_config_t cfg = {
        .dma = dma, .channel = channel, .dir = DMA_DIR_MEM_TO_MEM,
        .periph_inc = true, .mem_inc = true,            // 8-bit items
    };
    dma_channel_configure(&cfg, src, dst, (uint16_t) len);

    DMA_t *ctrl = ctrl_of(dma);
    uint32_t shift = channel_flag_shift(channel);
    dma_channel_enable(dma, channel);                    // GO

    // Wait for transfer-complete (TCIF = bit shift+1).
    while (!(ctrl->ISR & (1u << (shift + 1u))));

    ctrl->IFCR = 0xFu << shift;          // clear flags
    dma_channel_disable(dma, channel);   // release the channel
}

void dma_mem_to_periph(uint8_t channel, uint8_t request,
                       volatile void *periph_reg, const void *src, uint32_t len)
{
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = channel, .request = request,
        .dir = DMA_DIR_MEM_TO_PERIPH, .mem_inc = true,  // 8-bit items
    };
    dma_channel_configure(&cfg, periph_reg, (void *) src, (uint16_t) len);

    uint32_t shift = channel_flag_shift(channel);
    dma_channel_enable(PERIPHERAL_DMA1, channel);        // GO

    // Bounded wait for transfer-complete: a mis-routed request would otherwise
    // hang forever (the peripheral never asserts its request, CNDTR never drains).
    volatile uint32_t guard = 0;
    while (!(DMA1->ISR & (1u << (shift + 1u))) && ++guard < 5000000u) { }

    DMA1->IFCR = 0xFu << shift;
    dma_channel_disable(PERIPHERAL_DMA1, channel);
}

/****************************** Interrupts ******************************/
/*
 * Legacy one-shot M2M callbacks (dma_register_handler / dma_mem_to_mem_irq)
 * and dma_channel_register_handler() share the DMA1 vectors; a channel uses
 * one or the other.
 */
static struct {
    callbackfn_t callback;
    void *aux_data;
} dma1_handlers[DMA1_NUM_CHANNELS];

void dma_register_handler(uint8_t channel, callbackfn_t fn, void *aux)
{
    assert(channel >= 1 && channel <= DMA1_NUM_CHANNELS);
    dma1_handlers[channel - 1u].callback = fn;
    dma1_handlers[channel - 1u].aux_data = aux;
}

void dma_mem_to_mem_irq(uint8_t channel,
                        void *dst, const void *src, uint32_t len)
{
    assert(channel >= 1 && channel <= DMA1_NUM_CHANNELS);
    channel_handlers[channel - 1u].fn = NULL;   // this channel: legacy callback
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = channel, .dir = DMA_DIR_MEM_TO_MEM,
        .periph_inc = true, .mem_inc = true, .irq_tc = true, .irq_te = true,
    };
    dma_channel_configure(&cfg, src, dst, (uint16_t) len);
    dma_channel_enable(PERIPHERAL_DMA1, channel);        // GO
}

static inline void handle_dma1_irq(uint8_t channel)
{
    uint32_t shift = channel_flag_shift(channel);
    uint32_t flags = (DMA1->ISR >> shift) & 0xFu;
    if (!flags) return;
    DMA1->IFCR = flags << shift;

    dma_channel_callback_t cfn = channel_handlers[channel - 1u].fn;
    if (cfn) {
        cfn(channel_handlers[channel - 1u].aux, flags);
        return;
    }
    // Legacy M2M one-shot: transfer-complete or -error ends it.
    if (flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE)) {
        dma_channel_disable(PERIPHERAL_DMA1, channel);
        callbackfn_t fn = dma1_handlers[channel - 1u].callback;
        if (fn) {                        // guard: never call a NULL handler
            fn(dma1_handlers[channel - 1u].aux_data);
        }
    }
}

void DMA1_Channel1_IRQHandler(void) { handle_dma1_irq(1); }
void DMA1_Channel2_IRQHandler(void) { handle_dma1_irq(2); }
void DMA1_Channel3_IRQHandler(void) { handle_dma1_irq(3); }
void DMA1_Channel4_IRQHandler(void) { handle_dma1_irq(4); }
void DMA1_Channel5_IRQHandler(void) { handle_dma1_irq(5); }
void DMA1_Channel6_IRQHandler(void) { handle_dma1_irq(6); }
void DMA1_Channel7_IRQHandler(void) { handle_dma1_irq(7); }
