/**********************************************************************/
/* STM32F7 DMA (stream controller) Hardware Abstraction layer         */
/* Implements libstefi/dma.h (same calls as on L4/G4) and the F7-only */
/* libstefi/dma_stream.h.                                             */
/**********************************************************************/
#include "libstefi/dma.h"
#include "libstefi/dma_stream.h"
#include "libstefi/peripheral.h"
#include "libstefi/interrupts.h"
#include "libstefi/util.h"
#include "internal/dma_f7.h"

#define NUM_STREAMS 8u

static inline DMA_F7_t *ctrl_of(peripheral_dma_t dma)
{
    return (dma == PERIPHERAL_DMA2) ? DMA2_F7 : DMA1_F7;
}

static inline DMA_Stream_F7_t *stream_of(peripheral_dma_t dma, uint8_t stream)
{
    return &ctrl_of(dma)->S[stream & 7u];
}

static uint32_t stream_read_flags(peripheral_dma_t dma, uint8_t stream)
{
    DMA_F7_t *c = ctrl_of(dma);
    uint32_t isr = (stream < 4u) ? c->LISR : c->HISR;
    return (isr >> dma_f7_flag_shift(stream)) & 0x3Du;
}

static void stream_clear_flags(peripheral_dma_t dma, uint8_t stream, uint32_t flags)
{
    DMA_F7_t *c = ctrl_of(dma);
    uint32_t v = (flags & 0x3Du) << dma_f7_flag_shift(stream);
    if (stream < 4u) c->LIFCR = v; else c->HIFCR = v;
}

nvic_source_t dma_stream_irq(peripheral_dma_t dma, uint8_t stream)
{
    if (dma == PERIPHERAL_DMA1) {
        return (stream == 7u) ? INTERRUPT_SOURCE_DMA1_STREAM7
                              : (nvic_source_t) (INTERRUPT_SOURCE_DMA1_STREAM0 + stream);
    }
    return (stream >= 5u) ? (nvic_source_t) (INTERRUPT_SOURCE_DMA2_STREAM5 + (stream - 5u))
                          : (nvic_source_t) (INTERRUPT_SOURCE_DMA2_STREAM0 + stream);
}

void dma_stream_disable(peripheral_dma_t dma, uint8_t stream)
{
    DMA_Stream_F7_t *s = stream_of(dma, stream);
    s->CR &= ~BIT(DMA_SxCR_EN);
    while (s->CR & BIT(DMA_SxCR_EN));   // EN reads 1 until the last beat is done
}

void dma_stream_enable(peripheral_dma_t dma, uint8_t stream)
{
    stream_of(dma, stream)->CR |= BIT(DMA_SxCR_EN);
}

bool dma_stream_is_enabled(peripheral_dma_t dma, uint8_t stream)
{
    return (stream_of(dma, stream)->CR & BIT(DMA_SxCR_EN)) != 0;
}

void dma_stream_set_items(peripheral_dma_t dma, uint8_t stream, uint16_t items)
{
    stream_of(dma, stream)->NDTR = items;
}

uint16_t dma_stream_get_items(peripheral_dma_t dma, uint8_t stream)
{
    return (uint16_t) stream_of(dma, stream)->NDTR;
}

void dma_stream_set_memory(peripheral_dma_t dma, uint8_t stream, uint8_t target, void *addr)
{
    DMA_Stream_F7_t *s = stream_of(dma, stream);
    if (target) s->M1AR = (uint32_t) addr; else s->M0AR = (uint32_t) addr;
}

uint8_t dma_stream_current_target(peripheral_dma_t dma, uint8_t stream)
{
    return (stream_of(dma, stream)->CR & BIT(DMA_SxCR_CT)) ? 1u : 0u;
}

void dma_stream_configure(const dma_stream_config_t *cfg, volatile const void *periph,
                          void *mem0, void *mem1, uint16_t items)
{
    assert(cfg->stream < NUM_STREAMS);
    assert(cfg->dir != DMA_DIR_MEM_TO_MEM || cfg->dma == PERIPHERAL_DMA2);
    assert(!cfg->double_buffer || mem1 != NULL);
    peripheral_dma_enable(cfg->dma);

    DMA_Stream_F7_t *s = stream_of(cfg->dma, cfg->stream);
    // RM0410 §8.3.18: disable, wait, clear the stream's flags, then program.
    dma_stream_disable(cfg->dma, cfg->stream);
    stream_clear_flags(cfg->dma, cfg->stream, 0x3Du);

    s->PAR  = (uint32_t) periph;
    s->M0AR = (uint32_t) mem0;
    s->M1AR = (uint32_t) mem1;
    s->NDTR = items;
    s->FCR  = 0;                         // direct mode (no FIFO)

    uint32_t cr = ((uint32_t) (cfg->channel & 7u) << DMA_SxCR_CHSEL)
                | ((uint32_t) cfg->dir << DMA_SxCR_DIR)
                | ((uint32_t) cfg->periph_width << DMA_SxCR_PSIZE)
                | ((uint32_t) cfg->mem_width << DMA_SxCR_MSIZE)
                | ((uint32_t) cfg->priority << DMA_SxCR_PL);
    if (cfg->periph_inc)    cr |= BIT(DMA_SxCR_PINC);
    if (cfg->mem_inc)       cr |= BIT(DMA_SxCR_MINC);
    if (cfg->circular)      cr |= BIT(DMA_SxCR_CIRC);
    if (cfg->double_buffer) cr |= BIT(DMA_SxCR_DBM) | BIT(DMA_SxCR_CIRC);
    if (cfg->irq_tc)        cr |= BIT(DMA_SxCR_TCIE);
    if (cfg->irq_ht)        cr |= BIT(DMA_SxCR_HTIE);
    if (cfg->irq_te)        cr |= BIT(DMA_SxCR_TEIE) | BIT(DMA_SxCR_DMEIE);
    s->CR = cr;

    if (cfg->irq_tc || cfg->irq_ht || cfg->irq_te) {
        interrupts_enable_source(dma_stream_irq(cfg->dma, cfg->stream));
    }
}

/****************************** Interrupts ******************************/
static struct {
    dma_stream_callback_t callback;
    void *aux_data;
} stream_handlers[PERIPHERAL_DMA_COUNT][NUM_STREAMS];

void dma_stream_register_handler(peripheral_dma_t dma, uint8_t stream,
                                 dma_stream_callback_t fn, void *aux)
{
    assert(stream < NUM_STREAMS);
    stream_handlers[dma][stream].callback = fn;
    stream_handlers[dma][stream].aux_data = aux;
}

static void handle_stream_irq(peripheral_dma_t dma, uint8_t stream)
{
    uint32_t flags = stream_read_flags(dma, stream);
    stream_clear_flags(dma, stream, flags);
    dma_stream_callback_t fn = stream_handlers[dma][stream].callback;
    if (fn) {
        fn(stream_handlers[dma][stream].aux_data, flags);
    }
}

void DMA1_Stream0_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 0); }
void DMA1_Stream1_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 1); }
void DMA1_Stream2_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 2); }
void DMA1_Stream3_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 3); }
void DMA1_Stream4_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 4); }
void DMA1_Stream5_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 5); }
void DMA1_Stream6_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 6); }
void DMA1_Stream7_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA1, 7); }
void DMA2_Stream0_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 0); }
void DMA2_Stream1_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 1); }
void DMA2_Stream2_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 2); }
void DMA2_Stream3_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 3); }
void DMA2_Stream4_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 4); }
void DMA2_Stream5_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 5); }
void DMA2_Stream6_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 6); }
void DMA2_Stream7_IRQHandler(void) { handle_stream_irq(PERIPHERAL_DMA2, 7); }

/********************** libstefi/dma.h on the F7 ************************/
/*
 * Same calls as on L4/G4, with F7 meaning:
 *   "channel" of dma_mem_to_mem*()  -> stream number 0..7 (DMA2 only:
 *                                      DMA1 cannot do memory-to-memory)
 *   dma_mem_to_periph(channel, request) -> DMA1 stream `channel`, request
 *                                      channel `request` (CHSEL)
 *   dma_register_handler(channel)    -> callback for DMA2 stream `channel`
 *                                      (the stream used by dma_mem_to_mem_irq)
 */
static void wait_tc_bounded(peripheral_dma_t dma, uint8_t stream)
{
    volatile uint32_t guard = 0;
    while (!(stream_read_flags(dma, stream) & (DMA_STREAM_FLAG_TC | DMA_STREAM_FLAG_TE))
           && ++guard < 50000000u) { }
    stream_clear_flags(dma, stream, 0x3Du);
    dma_stream_disable(dma, stream);
}

void dma_mem_to_mem(peripheral_dma_t dma, uint8_t channel,
                    void *dst, const void *src, uint32_t len)
{
    (void) dma;   // memory-to-memory is a DMA2 feature on the F7
    const dma_stream_config_t cfg = {
        .dma = PERIPHERAL_DMA2, .stream = channel, .channel = 0,
        .dir = DMA_DIR_MEM_TO_MEM, .periph_width = DMA_WIDTH_8, .mem_width = DMA_WIDTH_8,
        .periph_inc = true, .mem_inc = true,
    };
    dma_stream_configure(&cfg, src, dst, NULL, (uint16_t) len);
    dma_stream_enable(PERIPHERAL_DMA2, channel);
    wait_tc_bounded(PERIPHERAL_DMA2, channel);
}

void dma_mem_to_periph(uint8_t channel, uint8_t request,
                       volatile void *periph_reg, const void *src, uint32_t len)
{
    const dma_stream_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .stream = channel, .channel = request,
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_8, .mem_width = DMA_WIDTH_8,
        .mem_inc = true,
    };
    dma_stream_configure(&cfg, periph_reg, (void *) src, NULL, (uint16_t) len);
    dma_stream_enable(PERIPHERAL_DMA1, channel);
    wait_tc_bounded(PERIPHERAL_DMA1, channel);
}

static struct {
    callbackfn_t fn;
    void *aux;
} m2m_irq_handlers[NUM_STREAMS];

static void m2m_irq_done(void *aux, uint32_t flags)
{
    (void) flags;
    uint8_t stream = (uint8_t) (uintptr_t) aux;
    dma_stream_disable(PERIPHERAL_DMA2, stream);
    if (m2m_irq_handlers[stream].fn) {
        m2m_irq_handlers[stream].fn(m2m_irq_handlers[stream].aux);
    }
}

void dma_register_handler(uint8_t channel, callbackfn_t fn, void *aux)
{
    assert(channel < NUM_STREAMS);
    m2m_irq_handlers[channel].fn = fn;
    m2m_irq_handlers[channel].aux = aux;
}

void dma_mem_to_mem_irq(uint8_t channel, void *dst, const void *src, uint32_t len)
{
    assert(channel < NUM_STREAMS);
    const dma_stream_config_t cfg = {
        .dma = PERIPHERAL_DMA2, .stream = channel, .channel = 0,
        .dir = DMA_DIR_MEM_TO_MEM, .periph_width = DMA_WIDTH_8, .mem_width = DMA_WIDTH_8,
        .periph_inc = true, .mem_inc = true, .irq_tc = true, .irq_te = true,
    };
    dma_stream_register_handler(PERIPHERAL_DMA2, channel, m2m_irq_done,
                                (void *) (uintptr_t) channel);
    dma_stream_configure(&cfg, src, dst, NULL, (uint16_t) len);
    dma_stream_enable(PERIPHERAL_DMA2, channel);
}
