/*
 * DAC_SimpleConversion: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * DAC ch1 (PA4) outputs 0xFF (3.3 V); after B1, in sample & hold mode with
 * the core in Sleep.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/dac.h>
#include <libstefi/pwr.h>
#include <libstefi/systick.h>
#include <libstefi/util.h>

void main() {
    board_init();

    dac_init(1, DAC_MODE_NORMAL_BUFFERED, DAC_TRIGGER_NONE);
    dac_write8(1, 0xFF);

    while (!button_is_pressed(BUTTON_USER));

    // sample & hold runs on the LSI
    *(volatile uint32_t *) 0x40021094u |= BIT(0);            // RCC_CSR LSION
    while (!(*(volatile uint32_t *) 0x40021094u & BIT(1)));
    dac_disable(1);
    dac_sample_hold(1, 20, 10, 5);
    dac_init(1, DAC_MODE_SAMPLE_HOLD_BUFFERED, DAC_TRIGGER_NONE);
    dac_write8(1, 0xFF);

    systick_stop();                                          // no tick interrupt in Sleep
    while(1) pwr_enter_sleep();
}
