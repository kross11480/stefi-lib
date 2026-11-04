/*
 * HAL_TimeBase_TIM: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * The 1 ms time base comes from TIM6 instead of SysTick; LED2 toggles every
 * second. B1 suspends / resumes the tick.
 */

#include "board.h"
#include "led.h"
#include "button.h"
#include "hardware_timer.h"

#include <libstefi/systick.h>

#include <stdbool.h>

static volatile uint32_t tim6_ticks;
static volatile bool tick_suspended;

void tick_cb() {
    tim6_ticks++;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = tim6_ticks;
    while((tim6_ticks - start) < ms);
}

void button_cb() {
    tick_suspended = !tick_suspended;
    if (tick_suspended) {
        htimer_stop(TIMER6);
    } else {
        htimer_start(TIMER6);
    }
}

void main() {
    board_init();
    systick_stop();                      //TIM6 is the time base here
    htimer_init_periodic_ms(TIMER6, 1, tick_cb);
    htimer_start(TIMER6);
    button_interrupt_init(BUTTON_USER, button_cb);

    while(1) {
        delay_ms(1000);
        led_toggle(LED2);
    }
}
