/*
 * I2C_TwoBoards_RestartAdvComIT: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, I2C3 PC0/PC1, 7-bit address 0x1E (HAL form 0x3C). Each B1 press
 * on the master sends the next command; CHIP_NAME / CHIP_REVISION are
 * answered after a repeated start, LOW_POWER / WAKE_UP are write-only.
 * Results on the master's VCP. Build variants _master / _slave.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/i2c.h>
#include <libstefi/systick.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define I2C_ADDRESS 0x1E
#define I2C_TIMING  0x00D00E28

#if defined(MASTER_BOARD)
/* [command][answer expected] */
static const char *const commands[4][2] = {
    {"CHIP_NAME", "YES"}, {"CHIP_REVISION", "YES"}, {"LOW_POWER", "NO"}, {"WAKE_UP", "NO"},
};
#endif
static const char *const slave_info[2] = {"STM32L476xx", "1.2.3"};

#if defined(MASTER_BOARD)
void main() {
    board_init();
    const i2c_bus_config_t bus = {.bus = 3, .scl = C0, .sda = C1, .af = AF4,
                                  .clock = I2C_CLK_PCLK, .timing = I2C_TIMING};
    i2c_bus_init(&bus);

    uint8_t index = 0;
    while(1) {
        while(!button_is_pressed(BUTTON_USER));
        while(button_is_pressed(BUTTON_USER));
        const char *cmd = commands[index][0];
        bool answer = strcmp(commands[index][1], "YES") == 0;
        uint8_t rx[16] = {0};
        uint16_t rxn = answer ? (uint16_t) strlen(slave_info[index]) : 0;
        i2c_status_t st;
        do {
            //command, then (answer expected) repeated start + read
            i2c_master_transfer_it(3, I2C_ADDRESS, (const uint8_t *) cmd, (uint16_t) strlen(cmd),
                                   answer ? rx : NULL, rxn, NULL, NULL);
            while (i2c_busy(3));
            st = i2c_status(3);
        } while (st == I2C_NACK);
        if (answer) {
            printf("%s : %s\r\n", cmd, (char *) rx);
        } else {
            printf("Slave goes to %s.\r\n", cmd);
        }
        led_toggle(LED2);
        index = (index + 1) % 4;
    }
}
#else
static char rx_cmd[16];
static volatile uint16_t rx_len;

void on_address(void *aux, bool master_reads, uint8_t **buf, uint16_t *len) {
    if (master_reads) {
        //answer the command just received
        int i = (strncmp(rx_cmd, "CHIP_NAME", 9) == 0) ? 0 : 1;
        *buf = (uint8_t *) slave_info[i];
        *len = (uint16_t) strlen(slave_info[i]);
    } else {
        memset(rx_cmd, 0, sizeof rx_cmd);
        *buf = (uint8_t *) rx_cmd;
        *len = sizeof rx_cmd - 1u;
    }
}

void on_complete(void *aux, bool master_reads, uint16_t count) {
    if (!master_reads) {
        rx_len = count;
        led_toggle(LED2);
    }
}

static const i2c_listen_ops_t listen_ops = {on_address, on_complete};

void main() {
    board_init();
    const i2c_bus_config_t bus = {.bus = 3, .scl = C0, .sda = C1, .af = AF4,
                                  .clock = I2C_CLK_PCLK, .timing = I2C_TIMING,
                                  .own_address = I2C_ADDRESS};
    i2c_bus_init(&bus);
    i2c_slave_listen(3, &listen_ops, NULL);
    while(1);
}
#endif
