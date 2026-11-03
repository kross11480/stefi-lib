/* HAL_TimeBase_TIM (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * The 1 ms time base comes from TIM6 instead of SysTick: its update
 * interrupt increments the tick counter used for the delay. LD2 toggles
 * every second; B1 suspends / resumes the tick (TIM6 interrupt off/on),
 * which freezes / restarts the blinking. */
#include "l4_example.h"
#include "libstefi/timer.h"

static volatile uint32_t tim6_ticks;
static volatile bool tick_suspended;

static void on_tick(void *aux) { (void) aux; tim6_ticks++; }

static void delay_ms(uint32_t ms)
{
    uint32_t t0 = tim6_ticks;
    while (tim6_ticks - t0 < ms);
}

static void on_button(void *aux)
{
    (void) aux;
    tick_suspended = !tick_suspended;
    if (tick_suspended) timer_disable_interrupt(TIMER6);
    else timer_enable_interrupt(TIMER6);
}

void main(void)
{
    board_init();
    systick_stop();                                  // TIM6 is the time base here

    timer_init(TIMER6);
    timer_set_period(TIMER6, (uint16_t) (system_get_core_clock() / 1000000u), 1000);   // 1 MHz / 1000
    timer_update_interrupt_register_handler(TIMER6, on_tick);
    timer_enable_interrupt(TIMER6);
    timer_start(TIMER6);
    example_button_irq(on_button);

    for (;;) {
        delay_ms(1000);
        led_toggle(LED2);
    }
}
