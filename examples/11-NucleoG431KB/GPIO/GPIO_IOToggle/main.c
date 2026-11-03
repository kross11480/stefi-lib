/*
 * GPIO_IOToggle (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * PB8 (LD2), push-pull output, toggles in a forever loop. HCLK = 170 MHz.
 */
#include "g4_example.h"

void main(void)
{
    board_init();                 // 170 MHz, LD2 on PB8 as push-pull output

    for (;;) {
        gpio_toggle(B8);
        systick_delay_ms(100);
    }
}
