/* RTC_Tamper (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * Tamper 1 = PC13 (B1), falling edge. 32 backup registers are written
 * (0xDF59 + n * 0x5A) and checked; LD2 toggles every second while waiting.
 * Press B1: the tamper event erases all backup registers and raises the
 * interrupt; the registers are checked to be 0, then LD2 blinks fast.
 * Errors (backup content wrong): LD2 blinking 1 s. */
#include "l4_example.h"
#include "libstefi/rtc.h"

#define BACKUP_COUNT 32u

static volatile bool tampered;

static void on_tamper(void *aux) { (void) aux; tampered = true; }

void main(void)
{
    board_init();
    rtc_init_lsi();
    rtc_set_prescaler(0x7F, 0xF9);
    rtc_tamper1_enable(true, on_tamper, NULL);

    for (uint32_t i = 0; i < BACKUP_COUNT; i++) rtc_backup_write((uint8_t) i, 0xDF59u + i * 0x5Au);
    for (uint32_t i = 0; i < BACKUP_COUNT; i++) {
        if (rtc_backup_read((uint8_t) i) != 0xDF59u + i * 0x5Au) example_error();
    }

    while (!tampered) { led_toggle(LED2); systick_delay_ms(1000); }
    rtc_tamper1_disable();
    for (uint32_t i = 0; i < BACKUP_COUNT; i++) {
        if (rtc_backup_read((uint8_t) i) != 0) example_error();
    }
    for (;;) { led_toggle(LED2); systick_delay_ms(100); }
}
