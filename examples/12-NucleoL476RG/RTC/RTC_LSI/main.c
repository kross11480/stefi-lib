/*
 * RTC_LSI: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * RTC wakeup every second toggles LED2; B1 calibrates the prescaler from the
 * measured LSI frequency.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <stddef.h>

#define WAKEUP_TIMER_ENABLE 0x32F2u

static void on_wakeup(void *aux) { (void) aux; led_toggle(LED2); }

void main() {
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);           // (32 kHz / 128) - 1
    if (rtc_backup_read(1) == WAKEUP_TIMER_ENABLE) rtc_wakeup_stop();
    rtc_wakeup_start(RTC_WAKEUP_CLK_1HZ, 0, on_wakeup, NULL);
    rtc_backup_write(1, WAKEUP_TIMER_ENABLE);

    while (!button_is_pressed(BUTTON_USER));
    while (button_is_pressed(BUTTON_USER));

    uint32_t lsi_hz = system_measure_lsi_hz();
    rtc_set_prescaler(0x7F, (uint16_t) (lsi_hz / 128u - 1u));
    while(1);
}
