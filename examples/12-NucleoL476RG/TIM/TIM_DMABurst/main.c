/*
 * TIM_DMABurst: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM2 CH1 (PA0): a DMA burst rewrites ARR and CCR1 on every update.
 */

#include "board.h"
#include "led.h"

#include <libstefi/dma_channel.h>
#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

static uint32_t src_buffer[3] = {0x0FFF, 0x0000, 0x0555};

void main() {
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
    while(1);
}
