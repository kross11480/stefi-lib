/* GPIO_IOToggle (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * PA5 (LD2), push-pull output, toggles forever. HCLK = 80 MHz. */
#include "l4_example.h"

void main(void)
{
    board_init();
    for (;;) {
        gpio_toggle(A5);
        systick_delay_ms(100);
    }
}
