/*
 * RTC_Alarm: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Calendar from 02:20:00, alarm A at 02:20:30 turns LED2 on; time on the VCP.
 */

#include "board.h"
#include "led.h"

#include <libstefi/rtc.h>
#include <libstefi/systick.h>

#include <stddef.h>
#include <stdio.h>

static char show_time[16];

static void on_alarm(void *aux) { (void) aux; led_on(LED2); }

void main() {
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);
    const rtc_datetime_t t = {.hours = 2, .minutes = 20, .seconds = 0,
                              .year = 14, .month = 2, .day = 18, .weekday = 2};
    rtc_set_datetime(&t);
    rtc_alarm_a_set(2, 20, 30, on_alarm, NULL);

    while(1) {
        rtc_datetime_t now;
        rtc_get_datetime(&now);
        snprintf(show_time, sizeof show_time, "%02u:%02u:%02u", now.hours, now.minutes, now.seconds);
        printf("%s\r\n", show_time);
        systick_delay_ms(1000);
    }
}
