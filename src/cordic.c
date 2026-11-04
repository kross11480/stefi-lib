/**********************************************************************/
/* CORDIC (STM32G4) — RM0440 §17                                      */
/**********************************************************************/
#include "libstefi/cordic.h"
#include "libstefi/dma_channel.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"
#include "internal/cordic_internal.h"

#include <stddef.h>


#define CSR_DMAREN   BIT(17)
#define CSR_DMAWEN   BIT(18)
#define CSR_RRDY     (1u << 31)

/* DMAMUX request IDs (RM0440 table 91) */
#define DMAMUX_CORDIC_READ  112u
#define DMAMUX_CORDIC_WRITE 113u

static struct {
    volatile bool busy;
    uint8_t ch_write, ch_read;
    void (*done)(void);
} dma;

void cordic_configure(const cordic_config_t *cfg)
{
    RCC->AHB1ENR |= BIT(3);           // CORDICEN
    (void) RCC->AHB1ENR;
    CORDIC->CSR = ((uint32_t) cfg->function & 0xFu)
               | ((uint32_t) (cfg->precision & 0xFu) << 4)
               | ((uint32_t) (cfg->scale & 0x7u) << 8)
               | (cfg->two_results ? BIT(19) : 0u)
               | (cfg->two_args ? BIT(20) : 0u)
               | (cfg->q15 ? (BIT(21) | BIT(22)) : 0u);
}

int32_t cordic_calculate(int32_t arg)
{
    CORDIC->WDATA = (uint32_t) arg;    // writing the (last) argument starts it
    while (!(CORDIC->CSR & CSR_RRDY));
    return (int32_t) CORDIC->RDATA;
}

static void on_read_done(void *aux, uint32_t flags)
{
    (void) aux;
    if (!(flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE))) return;
    dma_channel_disable(PERIPHERAL_DMA1, dma.ch_read);
    dma_channel_disable(PERIPHERAL_DMA1, dma.ch_write);
    CORDIC->CSR &= ~(CSR_DMAREN | CSR_DMAWEN);
    dma.busy = false;
    if (dma.done) dma.done();
}

void cordic_calculate_dma(const int32_t *args, int32_t *results, uint16_t n,
                          uint8_t ch_write, uint8_t ch_read, void (*done)(void))
{
    dma.busy = true;
    dma.ch_write = ch_write;
    dma.ch_read = ch_read;
    dma.done = done;

    const dma_channel_config_t rd = {
        .dma = PERIPHERAL_DMA1, .channel = ch_read, .request = DMAMUX_CORDIC_READ,
        .dir = DMA_DIR_PERIPH_TO_MEM, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_32,
        .mem_inc = true, .priority = DMA_PRIORITY_HIGH, .irq_tc = true, .irq_te = true,
    };
    const dma_channel_config_t wr = {
        .dma = PERIPHERAL_DMA1, .channel = ch_write, .request = DMAMUX_CORDIC_WRITE,
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_32,
        .mem_inc = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_register_handler(PERIPHERAL_DMA1, ch_read, on_read_done, NULL);
    dma_channel_configure(&rd, &CORDIC->RDATA, results, n);
    dma_channel_configure(&wr, &CORDIC->WDATA, (void *) args, n);
    dma_channel_enable(PERIPHERAL_DMA1, ch_read);
    dma_channel_enable(PERIPHERAL_DMA1, ch_write);
    CORDIC->CSR |= CSR_DMAREN | CSR_DMAWEN;   // requests start flowing
}

bool cordic_dma_busy(void)
{
    return dma.busy;
}
