/*
 * RTC_Tamper: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Backup registers written; a tamper event on PC13 (B1) erases them.
 * LED2 fast blinking: erased as expected; 1 s blinking: error.
 */

#include "board.h"
#include "led.h"

#include <libstefi/rtc.h>
#include <libstefi/systick.h>

#include <stddef.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

#define BACKUP_COUNT 32u

static volatile bool tampered;

static void on_tamper(void *aux) { (void) aux; tampered = true; }

void main() {
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);
    rtc_tamper1_enable(true, on_tamper, NULL);

    for (uint32_t i = 0; i < BACKUP_COUNT; i++) rtc_backup_write((uint8_t) i, 0xDF59u + i * 0x5Au);
    for (uint32_t i = 0; i < BACKUP_COUNT; i++) {
        if (rtc_backup_read((uint8_t) i) != 0xDF59u + i * 0x5Au) led_error_forever();
    }

    while (!tampered) { led_toggle(LED2); systick_delay_ms(1000); }
    rtc_tamper1_disable();
    for (uint32_t i = 0; i < BACKUP_COUNT; i++) {
        if (rtc_backup_read((uint8_t) i) != 0) led_error_forever();
    }
    while(1) { led_toggle(LED2); systick_delay_ms(100); }
}
