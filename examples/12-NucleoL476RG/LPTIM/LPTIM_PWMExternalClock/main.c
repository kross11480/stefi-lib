/*
 * LPTIM_PWMExternalClock: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LPTIM1 clocked from IN1 (PB5) outputs PWM on PB2 (input / 100, 50 %) while
 * the MCU is in Stop 1; B1 stops it.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/lptim.h>
#include <libstefi/pwr.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

/* back to 80 MHz, e.g. after Stop (which wakes up on MSI) */
static void restore_core_clock() {
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
}

static void on_button(void *aux) { (void) aux; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

    lptim_init(LPTIM_CLK_LSI, LPTIM_COUNT_IN1_CLOCK, 0);
    lptim_pwm_start(100 - 1, 50 - 1);
    pwr_enter_stop1();              // until B1
    restore_core_clock();
    lptim_stop();
    while(1);
}
