/* GPIO_Demo (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * LD2 blinks; each B1 press selects the next speed: slow, medium, fast. */
#include "l4_example.h"

#define LED_BLINK_SLOW   800
#define LED_BLINK_MEDIUM 400
#define LED_BLINK_FAST   70

static volatile uint32_t blink_interval = LED_BLINK_SLOW;

static void on_button(void *aux)
{
    (void) aux;
    static uint32_t presses;
    static const uint32_t speed[3] = {LED_BLINK_SLOW, LED_BLINK_MEDIUM, LED_BLINK_FAST};
    presses = (presses % 3u) + 1u;
    blink_interval = speed[presses - 1u];
}

void main(void)
{
    board_init();
    example_button_irq(on_button);
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(blink_interval);
    }
}
