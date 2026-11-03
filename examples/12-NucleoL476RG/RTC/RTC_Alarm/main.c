/* RTC_Alarm (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * RTC on the LSI (prescalers 127 / 249 -> 1 Hz), calendar set to
 * Tuesday 2014-02-18 02:20:00, alarm A at 02:20:30. The time is shown
 * on the VCP every second (ST keeps it in aShowTime); at the alarm LD2
 * turns on. */
#include <stdio.h>

#include "l4_example.h"
#include "libstefi/rtc.h"

static char show_time[16];

static void on_alarm(void *aux) { (void) aux; led_on(LED2); }

void main(void)
{
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);
    const rtc_datetime_t t = {.hours = 2, .minutes = 20, .seconds = 0,
                              .year = 14, .month = 2, .day = 18, .weekday = 2};
    rtc_set_datetime(&t);
    rtc_alarm_a_set(2, 20, 30, on_alarm, NULL);

    for (;;) {
        rtc_datetime_t now;
        rtc_get_datetime(&now);
        snprintf(show_time, sizeof show_time, "%02u:%02u:%02u", now.hours, now.minutes, now.seconds);
        printf("%s\r\n", show_time);
        systick_delay_ms(1000);
    }
}
