/* RTC_TimeStamp (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * Calendar set to Monday 2014-04-14 08:10:00; timestamp on the TS pin
 * (PC13 = B1), rising edge (release of B1). The current time is printed
 * on the VCP every second; on each timestamp event LD2 toggles and the
 * latched time/date is printed. */
#include <stdio.h>

#include "l4_example.h"
#include "libstefi/rtc.h"

static volatile bool stamped;

static void on_timestamp(void *aux)
{
    (void) aux;
    stamped = true;
    led_toggle(LED2);
}

void main(void)
{
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);
    const rtc_datetime_t t = {.hours = 8, .minutes = 10, .seconds = 0,
                              .year = 14, .month = 4, .day = 14, .weekday = 1};
    rtc_set_datetime(&t);
    rtc_timestamp_enable(false, on_timestamp, NULL);

    for (;;) {
        rtc_datetime_t now;
        rtc_get_datetime(&now);
        printf("%02u:%02u:%02u  %02u-%02u-%02u\r\n", now.hours, now.minutes, now.seconds,
               now.month, now.day, 2000u + now.year);
        if (stamped) {
            stamped = false;
            rtc_datetime_t ts;
            rtc_timestamp_get(&ts);
            printf("timestamp: %02u:%02u:%02u  %02u-%02u\r\n", ts.hours, ts.minutes, ts.seconds, ts.month, ts.day);
        }
        systick_delay_ms(1000);
    }
}
