/*
 * FLASH_DualBoot: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Build variants _bank1 (LED2 blinks) / _bank2 (LED2 on, load at 0x08080000).
 * B1 toggles boot-from-bank-2 (BFB2) and resets.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/flash.h>
#include <libstefi/systick.h>

/* LED2 blinking 100 ms on / 2 s off: a flash operation failed */
static void led_flash_forever() {
    while(1) {
        led_on(LED2);
        systick_delay_ms(100);
        led_off(LED2);
        systick_delay_ms(2000);
    }
}

void main() {
    board_init();
    led_on(LED2);
    while(1) {
        if (button_is_pressed(BUTTON_USER)) {
            while (button_is_pressed(BUTTON_USER));
            flash_set_dual_boot(!flash_dual_boot_enabled());   // resets on success
            led_flash_forever();
        }
#if defined(FLASH_BANK1)
        led_toggle(LED2);
#endif
        systick_delay_ms(50);
    }
}
