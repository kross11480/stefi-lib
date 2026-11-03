/* TIM_DMABurst (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM2 CH1 (PA0, AF1) PWM at 16 MHz counter clock. On every update DMA
 * request (DMA1 ch2, CSELR 4) the DMA burst writes 3 registers starting at
 * ARR: ARR = 0x0FFF, RCR = 0 (TIM2 has none), CCR1 = 0x0555 ->
 * 16 MHz / 4096 = 3.9 kHz with ~33 % duty. */
#include "l4_example.h"
#include "libstefi/dma_channel.h"
#include "libstefi/timer.h"

static uint32_t src_buffer[3] = {0x0FFF, 0x0000, 0x0555};

void main(void)
{
    board_init();
    gpio_set_alternate_function(A0, AF1);
    gpio_set_mode(A0, MODER_AF);
    timer_init(TIMER2);
    timer_set_period(TIMER2, (uint16_t) (system_get_core_clock() / 16000000u), 0x10000);
    timer_set_mode_pwm(TIMER2, TIMER_CHANNEL1);
    timer_set_compare(TIMER2, TIMER_CHANNEL1, 0xFFF);
    timer_cc_enable(TIMER2, TIMER_CHANNEL1, false);
    timer_set_oc_polarity(TIMER2, TIMER_CHANNEL1, true);

    volatile uint32_t *dmar = timer_dma_burst(TIMER2, TIMER_REG_ARR, 3);
    const dma_channel_config_t dma = {
        .dma = PERIPHERAL_DMA1, .channel = 2, .request = 4,      // TIM2_UP
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_32,
        .mem_inc = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_configure(&dma, dmar, src_buffer, 3);
    dma_channel_enable(PERIPHERAL_DMA1, 2);
    timer_enable_dma_request(TIMER2, TIMER_DMA_UPDATE);
    timer_start(TIMER2);
    for (;;);
}
