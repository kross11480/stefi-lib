/* PWR_SLEEP (STM32CubeL4, NUCLEO-L476RG) on stefi-lib. HCLK 80 MHz.
 *
 * LD2 toggles (Run mode) for 5 s, then the MCU enters Sleep mode (LD2
 * off, SysTick paused). B1 (EXTI 13, falling edge) wakes it; the cycle
 * repeats. Measure IDD on JP6. */
#include "l4_example.h"
#include "libstefi/pwr.h"

static void on_button(void *aux) { (void) aux; }

void main(void)
{
    board_init();
    example_button_irq(on_button);
    for (;;) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        pwr_enter_sleep();          // until B1
        systick_start();
    }
}
