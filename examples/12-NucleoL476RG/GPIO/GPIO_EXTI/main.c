/* GPIO_EXTI (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * EXTI line 13 (PC13, B1) on the falling edge; the interrupt toggles LD2. */
#include "l4_example.h"

static void on_button(void *aux)
{
    (void) aux;
    led_toggle(LED2);
}

void main(void)
{
    board_init();
    example_button_irq(on_button);
    for (;;);
}
