/* PWR_LPSLEEP (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * SYSCLK = MSI 32 MHz. LD2 toggles for 5 s; then MSI 2 MHz (range 5),
 * low-power run regulator and Sleep: low-power sleep (LD2 off). B1 (EXTI
 * 13) wakes the MCU; main regulator and 32 MHz again; the cycle repeats. */
#include "l4_example.h"
#include "libstefi/pwr.h"

static void on_button(void *aux) { (void) aux; }

void main(void)
{
    board_init();
    example_button_irq(on_button);
    for (;;) {
        system_init_l4_msi(10);
        systick_init(32000000u);
        systick_start();
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        system_init_l4_msi(5);               // 2 MHz
        pwr_enter_lp_sleep();                // until B1
        pwr_lprun_exit();
    }
}
