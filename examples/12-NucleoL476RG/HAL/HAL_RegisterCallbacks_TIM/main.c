/* HAL_RegisterCallbacks_TIM (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * TIM3: 80 MHz / 8000 = 10 kHz counter clock, period 10000 -> one update
 * per second. A callback registered for the update interrupt toggles LD2.
 * (stefi's timer_update_interrupt_register_handler() is the equivalent of
 * HAL_TIM_RegisterCallback(HAL_TIM_PERIOD_ELAPSED_CB_ID).) */
#include "l4_example.h"
#include "libstefi/timer.h"

static void my_period_elapsed(void *aux)
{
    (void) aux;
    led_toggle(LED2);
}

void main(void)
{
    board_init();
    timer_init(TIMER3);
    timer_set_period(TIMER3, (uint16_t) (system_get_core_clock() / 10000u), 10000);
    timer_update_interrupt_register_handler(TIMER3, my_period_elapsed);
    timer_enable_interrupt(TIMER3);
    timer_start(TIMER3);
    for (;;);
}
