#include <libstefi/rng.h>

#include "stdio.h"
#include "board.h"
#include "led.h"
#include "button.h"
#include "ssd1306.h"
#include "hardware_timer.h"

#define TIMER_ID TIMER4

void s0_action() {
    //wait a random timer
    led_off(LED0_RED);
    htimer_start(TIMER_ID);
    //random time between 0.5 and 1s
    volatile uint32_t rnd_time = 500 + rng_get_rand()/(0x1 << 20);
    while(htimer_get_count(TIMER_ID) <= rnd_time);
    htimer_stop(TIMER_ID);
    // start game
    led_on(LED0_RED);
    // start reaction timer
    htimer_start(TIMER_ID);
}

void s1_action() {
    char buffer[10];
    led_off(LED0_RED);
    ssd1306_clear_screen();
    sprintf(buffer, "TIME: %4d", htimer_get_count(TIMER_ID));
    ssd1306_putstring( 5, 0, buffer);
    ssd1306_update_screen();
    timer_stop(TIMER_ID);
}

void timer_expires() {
    printf("Slow Finger \r\n");
    timer_stop(TIMER_ID);
}

void setup() {
    //initialize button
    board_init();
    rng_init();
    button_interrupt_init(BUTTON_S0, s0_action);
    button_interrupt_init(BUTTON_S1, s1_action);
    //initialize reaction timer
    htimer_init_periodic_ms(TIMER_ID, 5000, timer_expires); //Timer freq = 100Hz

    printf("\r\n Start Reaction Game \r\n");
}

void main() {
    //initialize uart
    setup();
    while(1) {
    }
}