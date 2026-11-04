/*
 * COMP_Interrupt: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * COMP1: PC5 against VREFINT; every crossing toggles LED2 and wakes the
 * MCU from Stop 2 (entered 5 s after each wakeup).
 */

#include "board.h"
#include "led.h"

#include <libstefi/comp.h>
#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <stddef.h>

/* back to 80 MHz, e.g. after Stop (which wakes up on MSI) */
static void restore_core_clock() {
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
}

static void on_crossing(void *aux) {
    (void) aux;
    led_toggle(LED2);
}

void main() {
    board_init();
    led_off(LED2);

    const comp_config_t cfg = {.instance = 1, .inp = 0, .inm = COMP_INM_VREFINT,
                               .power = COMP_POWER_ULTRALOW};
    comp_init(&cfg);
    comp_enable_interrupt(1, true, true, on_crossing, NULL);

    while(1) {
        systick_delay_ms(5000);
        pwr_enter_stop2();
        restore_core_clock();     // woken by the comparator
    }
}
