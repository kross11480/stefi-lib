#include "board.h"

#include <libstefi/gpio.h>
#include <libstefi/peripheral.h>
#include <libstefi/systick.h>

#include <stdint.h>
#include <stdio.h>

/*
 * RN4020 wiring / power diagnostic — NO rewire, module stays connected.
 *
 * The bring-up gets "(no reply)" on every command, which means either the
 * MCU's UART4 RX is dead or the module never drives its TX. This probes the
 * physical link from software, without UART:
 *
 *  (1) PA1 = RN4020 UART_TX (module -> MCU). A powered RN4020 holds its TX
 *      idle HIGH. We sample PA1 as a plain input under PULL_DOWN and PULL_UP:
 *        PD=HIGH, PU=HIGH  -> line actively driven HIGH  => module powered + TX wire OK
 *        PD=LOW,  PU=HIGH  -> line floating              => module unpowered or TX wire open
 *        PD=LOW,  PU=LOW   -> line held LOW               => shorted to GND / wrong pin
 *      (this is the key result for the "no reply" fault.)
 *
 *  (2) Wake/CMD outputs are driven to their command-mode levels and reported,
 *      so you can probe CON20AP with a meter to confirm continuity to the
 *      module pins:  PC10/INT0=SWAKE=HIGH, PC8/ECLK=WAKE_HW=HIGH, PC9/OC2=CMD=LOW.
 *
 *  (3) PA0 = RN4020 UART_RX (MCU -> module): toggled at ~2 Hz so you can probe
 *      CON20AP ADC0 with a meter/LED and confirm the wire reaches the module.
 */

#define BLE_SWAKE    C10   // RN4020 WAKE_SW  (CON20AP INT0)
#define BLE_CMD      C9    // RN4020 CMD/MLDP (CON20AP OC2)  low = command mode
#define BLE_WAKE_HW  C8    // RN4020 WAKE_HW  (CON20AP ECLK)
#define BLE_RX_SENSE A1    // RN4020 UART_TX  (CON20AP ADC1) module drives this
#define BLE_TX_PIN   A0    // RN4020 UART_RX  (CON20AP ADC0) MCU drives this

static sig_t sense(pupdr_t pull) {
    gpio_set_pupd(BLE_RX_SENSE, pull);
    systick_delay_ms(5);            // let the pull settle
    return gpio_read(BLE_RX_SENSE);
}

void main(void) {
    board_init();

    // Wake the module the same way the bring-up does.
    peripheral_gpio_enable(PERIPHERAL_GPIOC);
    gpio_set_mode(BLE_SWAKE, MODER_OUTPUT);
    gpio_set_mode(BLE_CMD, MODER_OUTPUT);
    gpio_set_mode(BLE_WAKE_HW, MODER_OUTPUT);
    gpio_write(BLE_CMD, LOW);
    gpio_write(BLE_WAKE_HW, LOW);
    gpio_write(BLE_SWAKE, LOW);
    systick_delay_ms(100);
    gpio_write(BLE_WAKE_HW, HIGH);
    gpio_write(BLE_SWAKE, HIGH);
    systick_delay_ms(300);

    // PA1 as plain input (NOT alternate function) so we can read the raw level.
    peripheral_gpio_enable(PERIPHERAL_GPIOA);
    gpio_set_mode(BLE_RX_SENSE, MODER_INPUT);
    gpio_set_mode(BLE_TX_PIN, MODER_OUTPUT);

    printf("\r\n=== RN4020 wiring/power diagnostic (no UART) ===\r\n");
    printf("Outputs set: SWAKE(PC10)=HIGH WAKE_HW(PC8)=HIGH CMD(PC9)=LOW\r\n");
    printf("Probe CON20AP: INT0/ECLK ~3.3V, OC2 ~0V, 3V3(pin23)=3.3V\r\n\r\n");

    uint32_t tx = 0;
    while (1) {
        sig_t pd = sense(PULL_DOWN);
        sig_t pu = sense(PULL_UP);

        const char *verdict;
        if (pd == HIGH && pu == HIGH)      verdict = "DRIVEN HIGH  -> module powered + TX wire OK";
        else if (pd == LOW && pu == HIGH)  verdict = "FLOATING     -> module unpowered or TX wire open";
        else if (pd == LOW && pu == LOW)   verdict = "HELD LOW     -> short to GND / wrong pin";
        else                               verdict = "indeterminate";

        printf("PA1(module TX): pull-down=%s pull-up=%s | %s\r\n",
               pd == HIGH ? "H" : "L", pu == HIGH ? "H" : "L", verdict);

        // toggle PA0 so the MCU->module RX wire can be probed externally
        gpio_write(BLE_TX_PIN, (tx++ & 1) ? HIGH : LOW);
        systick_delay_ms(500);
    }
}
