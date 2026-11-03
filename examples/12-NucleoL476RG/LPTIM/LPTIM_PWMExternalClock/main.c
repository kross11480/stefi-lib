/* LPTIM_PWMExternalClock (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * LPTIM1 counts an external clock on IN1 (PB5) — no internal kernel clock
 * needed — and generates a PWM on OUT (PB2): period 100 counts,
 * output = input / 100, duty 50 %. The MCU then waits in Stop 1 (the LPTIM
 * keeps running); B1 wakes it and stops the PWM. */
#include "l4_example.h"
#include "libstefi/lptim.h"
#include "libstefi/pwr.h"

static void on_button(void *aux) { (void) aux; }

void main(void)
{
    board_init();
    example_button_irq(on_button);

    lptim_init(LPTIM_CLK_LSI, LPTIM_COUNT_IN1_CLOCK, 0);
    lptim_pwm_start(100 - 1, 50 - 1);
    pwr_enter_stop1();              // until B1
    example_restore_clock();
    lptim_stop();
    for (;;);
}
