/*
 * GPIO_Demo: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 blinks; each B1 press selects the next speed (800, 400, 70 ms).
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/systick.h>

#define LED_BLINK_SLOW   800
#define LED_BLINK_MEDIUM 400
#define LED_BLINK_FAST   70

static volatile uint32_t blink_interval = LED_BLINK_SLOW;

static void on_button(void *aux) {
    (void) aux;
    static uint32_t presses;
    static const uint32_t speed[3] = {LED_BLINK_SLOW, LED_BLINK_MEDIUM, LED_BLINK_FAST};
    presses = (presses % 3u) + 1u;
    blink_interval = speed[presses - 1u];
}

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(blink_interval);
    }
}
