/* PWR_STOP1 / PWR_STOP2 (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * Built twice: STOP_MODE=1 (l4_pwr_stop1) and STOP_MODE=2 (l4_pwr_stop2).
 *
 * LD2 toggles for 5 s, then Stop 1 / Stop 2 (LD2 off). B1 (EXTI 13) wakes
 * the MCU, the clock is restored to 80 MHz, and the cycle repeats.
 * Not usable under a debugger (core not clocked in Stop). */
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
#if STOP_MODE == 2
        pwr_enter_stop2();
#else
        pwr_enter_stop1();
#endif
        example_restore_clock();
    }
}
