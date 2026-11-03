/* IWDG_Reset (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * The LSI frequency is measured with TIM16 (CH1 internally connected to
 * the LSI) and the IWDG is set to a 1 s timeout from it (prescaler 32,
 * reload = f_LSI / 32). The main loop refreshes every 990 ms and toggles
 * LD2. B1 simulates a software failure (its interrupt writes to an invalid
 * address: HardFault, no more refresh) -> IWDG reset; after it LD2 is on
 * for 4 s. */
#include "l4_example.h"
#include "libstefi/iwdg.h"

static void on_button(void *aux)
{
    (void) aux;
    *(volatile uint32_t *) 0xA0003000u = 0xFF;   // no memory there: bus fault
}

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

    uint32_t lsi_hz = system_measure_lsi_hz();
    iwdg_start(IWDG_PRESCALER_32, (uint16_t) (lsi_hz / 32u), IWDG_WINDOW_DISABLED);

    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(990);
        iwdg_refresh();
    }
}
