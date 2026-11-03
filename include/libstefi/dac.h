#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* DAC1, channels 1 (PA4) and 2 (PA5) (STM32L4)                       */
/*                                                                    */
/* Values are 12-bit right aligned (8-bit with dac_write8). With a    */
/* trigger, the output updates on the trigger event (e.g. TIM6/TIM2   */
/* TRGO); the noise / triangle generators and DMA work on triggers.   */
/**********************************************************************/

typedef enum {
    DAC_TRIGGER_NONE = -1,
    DAC_TRIGGER_TIM6 = 0, DAC_TRIGGER_TIM8 = 1, DAC_TRIGGER_TIM7 = 2, DAC_TRIGGER_TIM5 = 3,
    DAC_TRIGGER_TIM2 = 4, DAC_TRIGGER_TIM4 = 5, DAC_TRIGGER_EXTI9 = 6, DAC_TRIGGER_SOFTWARE = 7,
} dac_trigger_t;

typedef enum {
    DAC_MODE_NORMAL_BUFFERED = 0,        /* output buffer on, to the pin */
    DAC_MODE_NORMAL_UNBUFFERED = 2,
    DAC_MODE_SAMPLE_HOLD_BUFFERED = 4,   /* low power: needs LSI/LSE (dac_sample_hold) */
} dac_mode_t;

/** Clock on, pin analog, mode and trigger set, channel enabled. */
void dac_init(uint8_t channel, dac_mode_t mode, dac_trigger_t trigger);
void dac_disable(uint8_t channel);
void dac_write(uint8_t channel, uint16_t value12);
void dac_write8(uint8_t channel, uint8_t value8);
void dac_software_trigger(uint8_t channel);

/** Triangle wave on top of the data value, amplitude 2^(amp_log2+1) - 1 (amp_log2 0..11). */
void dac_triangle(uint8_t channel, uint8_t amp_log2);

/** Sample & hold timings, in LSI cycles (mode DAC_MODE_SAMPLE_HOLD_*). */
void dac_sample_hold(uint8_t channel, uint16_t sample, uint16_t hold, uint16_t refresh);

/**
 * @brief Stream 12-bit values from memory on every trigger, circular, by
 * DMA (L4: ch1 -> DMA1 ch3 / CSELR 6, ch2 -> DMA1 ch4 / CSELR 5).
 */
void dac_start_dma(uint8_t channel, const uint16_t *values, uint16_t n);
void dac_stop_dma(uint8_t channel);
