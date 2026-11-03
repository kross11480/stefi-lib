/* CORTEXM_SysTick (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * SysTick at 1 ms (HCLK / 80000); LD2 toggles every 50 ms using it. */
#include "l4_example.h"
#include "libstefi/cortex.h"

void main(void)
{
    board_init();                    // systick_init(80 MHz): reload 80000 - 1 cycles per ms
    led_on(LED2);
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(50);
    }
}
