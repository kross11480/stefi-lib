/* PWR_STANDBY (STM32CubeL4, NUCLEO-L476RG) on stefi-lib. HCLK 80 MHz.
 *
 * LD2 toggles while running. A B1 press (EXTI 13) enables the wakeup pin
 * WKUP2 (= PC13, active on the falling edge), clears its flag and enters
 * Standby (LD2 off). The next B1 press — or the reset button — wakes the
 * MCU, which restarts from reset. */
#include "l4_example.h"
#include "libstefi/pwr.h"

static void on_button(void *aux)
{
    (void) aux;
    while (example_button_pressed());          // released, so the next press wakes
    pwr_enable_wakeup_pin(2, true);
    pwr_enter_standby();
}

void main(void)
{
    board_init();
    if (pwr_woke_from_standby()) pwr_clear_standby_flag();
    pwr_disable_wakeup_pin(2);
    pwr_clear_wakeup_flags();
    example_button_irq(on_button);
    for (;;) { led_toggle(LED2); systick_delay_ms(200); }
}
