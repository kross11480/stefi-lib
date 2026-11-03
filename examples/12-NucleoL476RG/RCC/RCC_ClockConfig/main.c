/* RCC_ClockConfig (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * SYSCLK = 80 MHz from the PLL, fed by MSI at start. Each B1 press
 * switches the PLL source: MSI -> HSI16 -> MSI ... (via MSI as temporary
 * system clock, PLL reprogrammed, back to the PLL). SYSCLK is output on
 * MCO (PA8) — 80 MHz either way, check it with an oscilloscope. LD2
 * toggles every 100 ms. */
#include "l4_example.h"

static volatile bool switch_clock;

static void on_button(void *aux) { (void) aux; switch_clock = true; }

void main(void)
{
    board_init();
    example_button_irq(on_button);
    system_mco(MCO_SYSCLK, 0);

    for (;;) {
        if (switch_clock) {
            switch_clock = false;
            system_clk_src_t next = (system_pll_source() == SYSTEM_CLK_MSI) ? SYSTEM_CLK_HSI16 : SYSTEM_CLK_MSI;
            system_init_l4(next, BOARD_SYSCLK);
        }
        led_toggle(LED2);
        systick_delay_ms(100);
    }
}
