/*
 * RCC_ClockConfig: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * SYSCLK 80 MHz on MCO (PA8); B1 switches the PLL source MSI <-> HSI16.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>

static volatile bool switch_clock;

static void on_button(void *aux) { (void) aux; switch_clock = true; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);
    system_mco(MCO_SYSCLK, 0);

    while(1) {
        if (switch_clock) {
            switch_clock = false;
            system_clk_src_t next = (system_pll_source() == SYSTEM_CLK_MSI) ? SYSTEM_CLK_HSI16 : SYSTEM_CLK_MSI;
            system_init_l4(next, BOARD_SYSCLK);
        }
        led_toggle(LED2);
        systick_delay_ms(100);
    }
}
