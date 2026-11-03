/* RTC_LSI (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * RTC on the LSI with the default prescalers (assuming 32 kHz); the wakeup
 * timer (ck_spre, 1 s) toggles LD2 — off by the LSI error. A key in backup
 * register 1 tells a restart that the wakeup timer is already set up.
 * Press B1: the LSI is measured with TIM16 (CH1 = LSI) and the synchronous
 * prescaler set to f_LSI / 128 - 1, so the 1 s period becomes accurate. */
#include "l4_example.h"
#include "libstefi/rtc.h"

#define WAKEUP_TIMER_ENABLE 0x32F2u

static void on_wakeup(void *aux) { (void) aux; led_toggle(LED2); }

void main(void)
{
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);           // (32 kHz / 128) - 1
    if (rtc_backup_read(1) == WAKEUP_TIMER_ENABLE) rtc_wakeup_stop();
    rtc_wakeup_start(RTC_WAKEUP_CLK_1HZ, 0, on_wakeup, NULL);
    rtc_backup_write(1, WAKEUP_TIMER_ENABLE);

    while (!example_button_pressed());
    while (example_button_pressed());

    uint32_t lsi_hz = system_measure_lsi_hz();
    rtc_set_prescaler(0x7F, (uint16_t) (lsi_hz / 128u - 1u));
    for (;;);
}
