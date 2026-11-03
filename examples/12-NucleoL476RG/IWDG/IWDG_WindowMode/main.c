/* IWDG_WindowMode (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * IWDG: LSI / 16, reload (32000 * 762) / 16000 (~762 ms), window
 * (32000 * 400) / 16000 (~400 ms): a refresh is only accepted in the last
 * ~400 ms before the timeout. The loop refreshes every 450 ms and toggles
 * LD2. B1 shortens the period to 200 ms: the refresh then comes before the
 * window opens -> IWDG reset. After it LD2 is on for 4 s. */
#include "l4_example.h"
#include "libstefi/iwdg.h"

static volatile uint32_t refresh_period_ms = 450;

static void on_button(void *aux) { (void) aux; refresh_period_ms = 200; }

void main(void)
{
    board_init();
    example_button_irq(on_button);

    if (system_reset_flags() & RESET_CAUSE_IWDG) {
        led_on(LED2);
        systick_delay_ms(4000);
        led_off(LED2);
    }
    system_clear_reset_flags();

    iwdg_start(IWDG_PRESCALER_16, (32000u * 762u) / 16000u, (32000u * 400u) / 16000u);
    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(refresh_period_ms);
        iwdg_refresh();
    }
}
