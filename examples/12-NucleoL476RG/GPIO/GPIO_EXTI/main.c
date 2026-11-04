/*
 * GPIO_EXTI: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Each B1 press (EXTI 13) toggles LED2.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/systick.h>

static void on_button(void *aux) {
    (void) aux;
    led_toggle(LED2);
}

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);
    while(1);
}
