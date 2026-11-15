/*
 * SPI_HalfDuplex_ComPolling: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Two boards, SPI1 on one data line: master MOSI PB5 <-> slave MISO PB4, SCK
 * PB3 <-> PB3, GND. The master writes and reads a counter register on the
 * slave (write, add 7, subtract 2, read back) forever, LED2 toggling.
 * Build variants _master / _slave; the master starts on B1. Error: LED2 1 s.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/spi.h>
#include <libstefi/systick.h>

#define READ_REGISTER       0x80
#define COMMAND_COUNTER     0x0F
#define COMMAND_COUNTER_ADD 0x01
#define COMMAND_COUNTER_SUB 0x02

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

#if defined(MASTER_BOARD)
static void synchro_wait() {
    for (volatile uint32_t i = 0; i < 300 * 80; i++);    //let the slave turn the line around
}

static void command_write(uint8_t command, uint8_t val) {
    spi_write_byte(PERIPHERAL_SPI1, command);
    spi_write_byte(PERIPHERAL_SPI1, val);
}

static uint8_t command_read(uint8_t command) {
    spi_write_byte(PERIPHERAL_SPI1, command | READ_REGISTER);
    synchro_wait();
    spi_disable(PERIPHERAL_SPI1);
    spi_half_duplex_direction(PERIPHERAL_SPI1, false);
    synchro_wait();
    uint8_t val = spi_read_byte(PERIPHERAL_SPI1, true);
    synchro_wait();
    spi_half_duplex_direction(PERIPHERAL_SPI1, true);
    spi_enable(PERIPHERAL_SPI1);
    synchro_wait();
    return val;
}

void main() {
    board_init();
    spi_gpio_init_pins(B3, B4, B5);
    const spi_config_t cfg = {.master = true, .baud_div_log2 = 3, .mode = 0};
    spi_configure_half_duplex(PERIPHERAL_SPI1, &cfg);
    while(!button_is_pressed(BUTTON_USER)) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
    led_off(LED2);
    spi_enable(PERIPHERAL_SPI1);

    uint8_t counter = 0;
    command_write(COMMAND_COUNTER, counter);
    if (command_read(COMMAND_COUNTER) != counter) led_error_forever();
    while(1) {
        counter += 7;
        command_write(COMMAND_COUNTER_ADD, 7);
        if (command_read(COMMAND_COUNTER) != counter) led_error_forever();
        counter -= 2;
        command_write(COMMAND_COUNTER_SUB, 2);
        if (command_read(COMMAND_COUNTER) != counter) led_error_forever();
        systick_delay_ms(16);
        led_toggle(LED2);
    }
}
#else
void main() {
    board_init();
    spi_gpio_init_pins(B3, B4, B5);
    const spi_config_t cfg = {.master = false, .mode = 0};
    spi_configure_half_duplex(PERIPHERAL_SPI1, &cfg);
    spi_half_duplex_direction(PERIPHERAL_SPI1, false);
    spi_enable(PERIPHERAL_SPI1);

    uint8_t counter = 0;
    while(1) {
        uint8_t cmd = spi_read_byte(PERIPHERAL_SPI1, false);
        switch (cmd) {
            case COMMAND_COUNTER | READ_REGISTER:
                spi_disable(PERIPHERAL_SPI1);
                spi_half_duplex_direction(PERIPHERAL_SPI1, true);
                spi_enable(PERIPHERAL_SPI1);
                spi_write_byte(PERIPHERAL_SPI1, counter);
                spi_disable(PERIPHERAL_SPI1);
                spi_half_duplex_direction(PERIPHERAL_SPI1, false);
                spi_enable(PERIPHERAL_SPI1);
                break;
            case COMMAND_COUNTER:
                counter = spi_read_byte(PERIPHERAL_SPI1, false);
                break;
            case COMMAND_COUNTER_ADD:
                counter += spi_read_byte(PERIPHERAL_SPI1, false);
                break;
            case COMMAND_COUNTER_SUB:
                counter -= spi_read_byte(PERIPHERAL_SPI1, false);
                break;
            default:
                led_error_forever();
        }
    }
}
#endif
