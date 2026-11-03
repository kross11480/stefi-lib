/* DAC_SimpleConversion (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * Step 0: DAC channel 1 (PA4), normal mode with output buffer, outputs
 *         0xFF in 8-bit right alignment: 3.3 V.
 * Press B1 -> step 1: the same in sample & hold (low-power) mode, clocked
 *         by the LSI (sample 20, hold 10, refresh 5 cycles), with the core
 *         in Sleep mode. Measure PA4 with an oscilloscope. */
#include "l4_example.h"
#include "libstefi/dac.h"
#include "libstefi/pwr.h"

void main(void)
{
    board_init();

    dac_init(1, DAC_MODE_NORMAL_BUFFERED, DAC_TRIGGER_NONE);
    dac_write8(1, 0xFF);

    while (!example_button_pressed());

    // sample & hold runs on the LSI
    *(volatile uint32_t *) 0x40021094u |= BIT(0);            // RCC_CSR LSION
    while (!(*(volatile uint32_t *) 0x40021094u & BIT(1)));
    dac_disable(1);
    dac_sample_hold(1, 20, 10, 5);
    dac_init(1, DAC_MODE_SAMPLE_HOLD_BUFFERED, DAC_TRIGGER_NONE);
    dac_write8(1, 0xFF);

    systick_stop();                                          // no tick interrupt in Sleep
    for (;;) pwr_enter_sleep();
}
