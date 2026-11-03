/**********************************************************************/
/* DAC1 (STM32L4, RM0351 §19)                                         */
/**********************************************************************/
#include "libstefi/dac.h"
#include "libstefi/dma_channel.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"

#define DAC_REG(off) (*(volatile uint32_t *) (0x40007400u + (off)))
#define DAC_CR      DAC_REG(0x00)
#define DAC_SWTRIGR DAC_REG(0x04)
#define DAC_DHR12R(ch) DAC_REG((ch) == 1 ? 0x08 : 0x14)
#define DAC_DHR8R(ch)  DAC_REG((ch) == 1 ? 0x10 : 0x1C)
#define DAC_SR      DAC_REG(0x34)
#define DAC_MCR     DAC_REG(0x3C)
#define DAC_SHSR(ch) DAC_REG((ch) == 1 ? 0x40 : 0x44)
#define DAC_SHHR    DAC_REG(0x48)
#define DAC_SHRR    DAC_REG(0x4C)

static inline uint32_t sh(uint8_t ch) { return (ch == 1) ? 0u : 16u; }

void dac_init(uint8_t channel, dac_mode_t mode, dac_trigger_t trigger)
{
    RCC->APB1ENR1 |= BIT(29);                        // DAC1EN
    (void) RCC->APB1ENR1;
    gpio_id_t pin = (channel == 1) ? A4 : A5;
    peripheral_gpio_enable(PERIPHERAL_GPIOA);
    gpio_set_mode(pin, MODER_ANALOG);

    DAC_CR &= ~(0xFFFFu << sh(channel));             // channel off, settings cleared
    DAC_MCR = (DAC_MCR & ~(0x7u << sh(channel))) | ((uint32_t) mode << sh(channel));
    uint32_t cr = 0;
    if (trigger != DAC_TRIGGER_NONE) cr |= BIT(2) | ((uint32_t) trigger << 3);   // TEN, TSEL
    DAC_CR |= (cr | BIT(0)) << sh(channel);          // + ENx
    for (volatile uint32_t i = 0; i < 200u; i++);    // tWAKEUP ~ 8 us
}

void dac_disable(uint8_t channel)
{
    DAC_CR &= ~(BIT(0) << sh(channel));
}

void dac_write(uint8_t channel, uint16_t value12) { DAC_DHR12R(channel) = value12 & 0xFFFu; }
void dac_write8(uint8_t channel, uint8_t value8)  { DAC_DHR8R(channel) = value8; }
void dac_software_trigger(uint8_t channel)        { DAC_SWTRIGR = BIT(channel - 1u); }

void dac_triangle(uint8_t channel, uint8_t amp_log2)
{
    // WAVE = 10 (triangle), MAMP; only taken while the channel is off
    uint32_t en = DAC_CR & (BIT(0) << sh(channel));
    DAC_CR &= ~en;
    DAC_CR = (DAC_CR & ~((0x3u << 6 | 0xFu << 8) << sh(channel)))
           | (((0x2u << 6) | ((uint32_t) (amp_log2 & 0xFu) << 8)) << sh(channel));
    DAC_CR |= en;
}

void dac_sample_hold(uint8_t channel, uint16_t sample, uint16_t hold, uint16_t refresh)
{
    // timings are written while the channel is disabled / BWST clear
    while (DAC_SR & ((1u << 15) << sh(channel)));
    DAC_SHSR(channel) = sample & 0x3FFu;
    DAC_SHHR = (DAC_SHHR & ~(0x3FFu << sh(channel))) | ((uint32_t) (hold & 0x3FFu) << sh(channel));
    DAC_SHRR = (DAC_SHRR & ~(0xFFu << sh(channel))) | ((uint32_t) (refresh & 0xFFu) << sh(channel));
}

void dac_start_dma(uint8_t channel, const uint16_t *values, uint16_t n)
{
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1,
        .channel = (channel == 1) ? 3 : 4,
        .request = (channel == 1) ? 6 : 5,           // L4 CSELR: DAC1 = 6 on ch3, DAC2 = 5 on ch4
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_16,
        .mem_inc = true, .circular = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_configure(&cfg, &DAC_DHR12R(channel), (void *) values, n);
    dma_channel_enable(PERIPHERAL_DMA1, cfg.channel);
    DAC_CR |= BIT(12) << sh(channel);                // DMAENx
}

void dac_stop_dma(uint8_t channel)
{
    DAC_CR &= ~(BIT(12) << sh(channel));
    dma_channel_disable(PERIPHERAL_DMA1, (channel == 1) ? 3 : 4);
}
