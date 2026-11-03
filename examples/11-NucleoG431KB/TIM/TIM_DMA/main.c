/*
 * TIM_DMA (STM32CubeG4, NUCLEO-G431KB) on stefi-lib. SYSCLK 150 MHz.
 *
 * TIM3 CH3 (PB0, AF2): 150 MHz / 8537 = 17.57 kHz PWM, active high. Its
 * capture/compare DMA request writes the next duty cycle from memory to
 * CCR3 (DMA1 ch1, word, circular): 75 %, 50 %, 25 %, 75 %, ...
 * (ST's text also mentions a repetition counter of 3, which TIM3 does not
 * have; the DMA request comes once per period.)
 */
#include "g4_example.h"
#include "libstefi/dma_channel.h"
#include "libstefi/timer.h"

#define DMAMUX_TIM3_CH3 63u   /* RM0440 table 91 */

static uint32_t cc_values[3];

void main(void)
{
    board_init();
    example_set_clock(150000000);

    uint32_t period = system_get_core_clock() / 17570u - 1u;   // ARR, as in ST's example
    cc_values[0] = 75u * (period - 1u) / 100u;
    cc_values[1] = 50u * (period - 1u) / 100u;
    cc_values[2] = 25u * (period - 1u) / 100u;

    timer_init(TIMER3);
    timer_set_period(TIMER3, 1, period + 1u);
    gpio_set_alternate_function(B0, AF2);
    gpio_set_output_speed(B0, HIGH_S);
    gpio_set_mode(B0, MODER_AF);
    timer_set_mode_pwm(TIMER3, TIMER_CHANNEL3);
    timer_set_compare(TIMER3, TIMER_CHANNEL3, cc_values[0]);
    timer_cc_enable(TIMER3, TIMER_CHANNEL3, false);
    timer_set_oc_polarity(TIMER3, TIMER_CHANNEL3, true);

    const dma_channel_config_t dma = {
        .dma = PERIPHERAL_DMA1, .channel = 1, .request = DMAMUX_TIM3_CH3,
        .dir = DMA_DIR_MEM_TO_PERIPH, .periph_width = DMA_WIDTH_32, .mem_width = DMA_WIDTH_32,
        .mem_inc = true, .circular = true, .priority = DMA_PRIORITY_HIGH,
    };
    dma_channel_configure(&dma, timer_ccr_address(TIMER3, TIMER_CHANNEL3), cc_values, 3);
    dma_channel_enable(PERIPHERAL_DMA1, 1);
    timer_enable_dma_request(TIMER3, TIMER_DMA_CC3);
    timer_start(TIMER3);

    for (;;);
}
