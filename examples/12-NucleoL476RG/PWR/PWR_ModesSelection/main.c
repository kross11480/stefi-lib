/*
 * PWR_ModesSelection: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Menu on the VCP: choose a low-power mode to measure IDD on JP6.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/pwr.h>
#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* back to 80 MHz, e.g. after Stop (which wakes up on MSI) */
static void restore_core_clock() {
    system_init_l4(SYSTEM_CLK_MSI, BOARD_SYSCLK);
    systick_init(BOARD_SYSCLK);
    systick_start();
}

static void on_button(void *aux) { (void) aux; }

static void read_line(char *buf, size_t n) {
    size_t i = 0;
    while(1) {
        char c = (char) uart_poll_in();
        uart_putchar(c);
        if (c == '\r' || c == '\n') { buf[i] = 0; return; }
        if (i + 1 < n) buf[i++] = c;
    }
}

static int atoi_simple(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

static void rtc_33s(void) {
    rtc_init_lsi();
    rtc_wakeup_start(RTC_WAKEUP_CLK_DIV16, 0xFFFF, NULL, NULL);
}

void main() {
    board_init();
    if (pwr_woke_from_standby()) pwr_clear_standby_flag();
    pwr_disable_wakeup_pin(2);
    pwr_clear_wakeup_flags();
    button_interrupt_init(BUTTON_USER, on_button);
    systick_delay_ms(1000);

    printf("\r\nREADY\r\nPlease enter the test number according below options\r\n");
    printf("      0       (SHUTDOWN)\r\n      1       (STANDBY)\r\n      2       (STANDBY + RTC)\r\n");
    printf("      3       (STANDBY + RTC + SRAM2)\r\n      4       (STOP2)\r\n      5       (STOP2   + RTC)\r\n");
    printf("      6       (STOP1)\r\n      7       (STOP1   + RTC)\r\n      8       (LPSLEEP 2MHz)\r\n");
    printf("      9       (LPRUN   2MHz)\r\n     10       (SLEEP Range 2, 24MHz)\r\n     11       (SLEEP Range 1, 80MHz)\r\n");
    printf("     12       (RUN   Range 2, 24MHz)\r\n     13       (RUN   Range 1, 80MHz)\r\n --> \r\n");

    char cmd[8];
    while(1) {
        read_line(cmd, sizeof cmd);
        int t = (cmd[0] >= '0' && cmd[0] <= '9') ? atoi_simple(cmd) : -1;
        printf("\r\nYou entered below test number:\r\n%d\r\n", t);
        while (button_is_pressed(BUTTON_USER));
        switch (t) {
            case 0: pwr_enable_wakeup_pin(2, true); pwr_enter_shutdown();
            case 1: pwr_enable_wakeup_pin(2, true); pwr_enter_standby();
            case 2: rtc_33s(); pwr_enter_standby();
            case 3: rtc_33s(); pwr_retain_sram2_in_standby(true); pwr_enter_standby();
            case 4: pwr_enter_stop2(); break;
            case 5: rtc_33s(); pwr_enter_stop2(); rtc_wakeup_stop(); break;
            case 6: pwr_enter_stop1(); break;
            case 7: rtc_33s(); pwr_enter_stop1(); rtc_wakeup_stop(); break;
            case 8: systick_stop(); system_init_l4_msi(5); pwr_enter_lp_sleep(); pwr_lprun_exit(); break;
            case 9:
                systick_stop(); system_init_l4_msi(5); pwr_lprun_enter();
                while (!button_is_pressed(BUTTON_USER));
                pwr_lprun_exit();
                break;
            case 10: systick_stop(); system_init_l4_msi(9); pwr_set_voltage_range(2);
                     pwr_enter_sleep(); pwr_set_voltage_range(1); break;
            case 11: systick_stop(); pwr_enter_sleep(); break;
            case 12: system_init_l4_msi(9); pwr_set_voltage_range(2); while(1);
            case 13: while(1);
            default: printf("Invalid test number.. Please enter again\r\n"); continue;
        }
        restore_core_clock();
        printf("Woken up. Next test number --> \r\n");
    }
}
