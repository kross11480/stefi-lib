/* TIM_TimeBase (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM3: 80 MHz / 8000 = 10 kHz, period 10000: an update interrupt every
 * second toggles LD2. */
#include "l4_example.h"
#include "libstefi/timer.h"

static void on_update(void *aux) { (void) aux; led_toggle(LED2); }

void main(void)
{
    board_init();
    timer_init(TIMER3);
    timer_set_period(TIMER3, (uint16_t) (system_get_core_clock() / 10000u), 10000);
    timer_update_interrupt_register_handler(TIMER3, on_update);
    timer_enable_interrupt(TIMER3);
    timer_start(TIMER3);
    for (;;);
}
