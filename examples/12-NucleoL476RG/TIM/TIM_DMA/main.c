/* TIM_DMA (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM1 CH3 (PA10, AF1): 80 MHz / 4553 = 17.57 kHz PWM, active high,
 * repetition counter 3. The CC3 DMA request (DMA1 ch7, CSELR 7, word,
 * circular) loads the next duty from memory into CCR3; with RCR = 3 a new
 * value takes effect every 4 periods: 75 %, 50 %, 25 %, 75 %, ... */
#include "l4_example.h"
#include "libstefi/dma_channel.h"
#include "libstefi/timer.h"

static uint32_t cc_values[3];

void main(void)
{
    board_init();
    uint32_t period = system_get_core_clock() / 17570u - 1u;
    cc_values[0] = 75u * (period - 1u) / 100u;
    cc_values[1] = 50u * (period - 1u) / 100u;
    cc_values[2] = 25u * (period - 1u) / 100u;

    gpio_set_alternate_function(A10, AF1);
    gpio_set_mode(A10, MODER_AF);
    timer_init(TIMER1);
    timer_set_period(TIMER1, 1, period + 1u);
    timer_set_repetition(TIMER1, 3);
    timer_set_mode_pwm(TIMER1, TIMER_CHANNEL3);
    timer_set_compare(TIMER1, TIMER_CHANNEL3, cc_values[0]);
    timer_cc_enable(TIMER1, TIMER_CHANNEL3, false);
    timer_set_oc_polarity(TIMER1, TIMER_CHANNEL3, true);

    const dma_channel_config_t dma = {
        .dma = PERIPHERAL_DMA1, .channel = 7, .request = 7,      // TIM1_CH3
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_32,
        .mem_inc = true, .circular = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_configure(&dma, timer_ccr_address(TIMER1, TIMER_CHANNEL3), cc_values, 3);
    dma_channel_enable(PERIPHERAL_DMA1, 7);
    timer_enable_dma_request(TIMER1, TIMER_DMA_CC3);
    timer_start(TIMER1);
    for (;;);
}
