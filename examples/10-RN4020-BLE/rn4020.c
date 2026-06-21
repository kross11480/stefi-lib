#include "board.h"

#include <libstefi/gpio.h>
#include <libstefi/peripheral.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

#include <stdint.h>
#include <stdio.h>

/*
 * RN4020 BLE bring-up on the STefi (NUCLEO-L476), wired to CON20AP (J1).
 *
 *   RN4020 5  UART_TX  -> CON20AP ADC1 = PA1 (UART4_RX)
 *   RN4020 6  UART_RX  <- CON20AP ADC0 = PA0 (UART4_TX)
 *   RN4020 7  SWAKE    <- CON20AP INT0 = PC10
 *   RN4020 8  CMD/MLDP <- CON20AP OC2  = PC9
 *   RN4020 15 WAKE_HW  <- CON20AP ECLK = PC8
 *   RN4020 23 VDD / GND -> CON20AP 3V3 / GND
 *
 * Logging stays on printf (USART2 / ST-Link VCP) — the module is on UART4, a
 * separate peripheral, so you watch the whole handshake on the terminal at
 * 115200. The HAL only drives USART2, so UART4 gets a small driver here
 * (clocked from HSI16, same fix that made printf reliable).
 *
 * PASS criterion: the "V" command returns a version string ("MCHP BTLE ...").
 * Then it sets a name and advertises — scan with nRF Connect / LightBlue.
 */

/* ---- UART4 (RN4020 link), L476 registers ----------------------------- */
#define UART4_BASE   0x40004C00u
#define UART4_CR1    (*(volatile uint32_t *)(UART4_BASE + 0x00))
#define UART4_BRR    (*(volatile uint32_t *)(UART4_BASE + 0x0C))
#define UART4_ISR    (*(volatile uint32_t *)(UART4_BASE + 0x1C))
#define UART4_RDR    (*(volatile uint32_t *)(UART4_BASE + 0x24))
#define UART4_TDR    (*(volatile uint32_t *)(UART4_BASE + 0x28))

#define RCC_CR       (*(volatile uint32_t *)0x40021000u)
#define RCC_APB1ENR1 (*(volatile uint32_t *)0x40021058u)
#define RCC_CCIPR    (*(volatile uint32_t *)0x40021088u)

#define BLE_SWAKE    C10   // RN4020 WAKE_SW
#define BLE_CMD      C9    // RN4020 CMD/MLDP (low = command mode)
#define BLE_WAKE_HW  C8    // RN4020 WAKE_HW

static void ble_uart_init(void) {
    RCC_CR |= (1u << 8);                 // HSION
    while (!(RCC_CR & (1u << 10))) {}    // HSIRDY
    RCC_CCIPR &= ~(0x3u << 6);
    RCC_CCIPR |=  (0x2u << 6);           // UART4SEL = HSI16
    RCC_APB1ENR1 |= (1u << 19);          // UART4EN

    peripheral_gpio_enable(PERIPHERAL_GPIOA);
    gpio_set_alternate_function(A0, AF8);  // UART4_TX
    gpio_set_alternate_function(A1, AF8);  // UART4_RX
    gpio_set_mode(A0, MODER_AF);
    gpio_set_mode(A1, MODER_AF);

    UART4_BRR = (16000000u + 115200u / 2) / 115200u;   // 115200 from HSI16
    UART4_CR1 = (1u << 0) | (1u << 2) | (1u << 3);      // UE | RE | TE
}

static void ble_pins_init(void) {
    peripheral_gpio_enable(PERIPHERAL_GPIOC);
    gpio_set_mode(BLE_SWAKE, MODER_OUTPUT);
    gpio_set_mode(BLE_CMD, MODER_OUTPUT);
    gpio_set_mode(BLE_WAKE_HW, MODER_OUTPUT);

    gpio_write(BLE_CMD, LOW);        // command (ASCII) mode, not MLDP
    // Deliberate LOW->HIGH edge on WAKE_SW: datasheet says this transition
    // moves the module Deep Sleep -> Idle and activates the command interface.
    gpio_write(BLE_WAKE_HW, LOW);
    gpio_write(BLE_SWAKE, LOW);
    systick_delay_ms(100);
    gpio_write(BLE_WAKE_HW, HIGH);   // wake from Dormant
    gpio_write(BLE_SWAKE, HIGH);     // wake from Deep Sleep -> command interface
    systick_delay_ms(200);           // module settle
}

static void ble_putc(char c) {
    while (!(UART4_ISR & (1u << 7))) {}   // TXE
    UART4_TDR = (uint8_t)c;
}

static void ble_send(const char *s) {
    while (*s) ble_putc(*s++);
}

//returns byte, or -1 on timeout
static int ble_getc(uint32_t timeout_ms) {
    uint32_t t0 = systick_get_ms();
    while (!(UART4_ISR & (1u << 5))) {     // RXNE
        if (systick_get_ms() - t0 >= timeout_ms) return -1;
    }
    return (int)(UART4_RDR & 0xFF);
}

/* send a command (CR-terminated) and echo the module's reply to the VCP */
static void ble_cmd(const char *cmd) {
    printf(">> %s", cmd);
    ble_send(cmd);

    printf("<< ");
    uint32_t t0 = systick_get_ms();
    int any = 0;
    while (systick_get_ms() - t0 < 700) {
        int c = ble_getc(150);
        if (c < 0) {
            if (any) break;     // got a reply then went quiet
            continue;           // still waiting for the first byte
        }
        any = 1;
        uart_putchar((char)c);  // -> USART2 / VCP log
    }
    if (!any) printf("(no reply)");
    printf("\r\n");
}

void main(void) {
    board_init();        // printf on USART2 (HSI16), SysTick
    ble_uart_init();
    ble_pins_init();

    printf("\r\n=== RN4020 BLE bring-up (UART4 on CON20AP PA0/PA1) ===\r\n");
    printf("log = USART2/VCP, module = UART4 (printf preserved)\r\n");
    systick_delay_ms(300);   // let the module boot after SWAKE high

    ble_cmd("V\r");              // firmware version -> proves the link + module
    ble_cmd("D\r");              // device info (MAC address, state)
    ble_cmd("SN,StefiBLE\r");    // set advertised name
    ble_cmd("R,1\r");            // reboot to apply
    systick_delay_ms(1500);      // module reboots (~1 s)
    ble_cmd("A\r");              // start advertising

    printf("scan with nRF Connect / LightBlue for 'StefiBLE'\r\n");
    printf("--- streaming BLE events (1s heartbeat) ---\r\n");

    uint32_t last = systick_get_ms();
    while (1) {
        int c = ble_getc(50);                 // echo unsolicited events (Connected, ...)
        if (c >= 0) { uart_putchar((char)c); continue; }
        if (systick_get_ms() - last >= 5000) {
            last = systick_get_ms();
            ble_cmd("V\r");                   // re-query version every 5s (diagnostic)
        }
    }
}
