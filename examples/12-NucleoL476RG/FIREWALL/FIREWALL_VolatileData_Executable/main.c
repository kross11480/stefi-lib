/*
 * FIREWALL_VolatileData_Executable: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * The firewall protects SRAM1 0x2000F100..0x2000FFFF as executable,
 * non-shared volatile data: a function and its data live there. Calling it
 * through its call gate works; reading its data from outside resets the MCU
 * (firewall reset). Progress in RTC backup register 31.
 * LED2 on: function result correct and the firewall reset happened; blinking 1 s: error.
 */

#include "board.h"
#include "led.h"

#include <libstefi/firewall.h>
#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#include <string.h>

extern uint32_t _fw_vdata_start, _fw_vdata_end, _fw_vdata_load;

__attribute__((section(".fw_vdata.0_pad"), used)) static const uint32_t fw_gate_pad = 0;
__attribute__((section(".fw_vdata.2_data"))) uint32_t protected_vdata[8] = {1, 2, 3, 4, 5, 6, 7, 8};

/* runs from the protected SRAM segment, entered through the call gate */
__attribute__((section(".fw_vdata.1_gate"), noinline, long_call, used))
uint32_t vdata_protected_function(const uint32_t *array) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < 8; i++) sum += array[i] * protected_vdata[i];
    return sum;
}

static uint32_t unprotected_vdata[8] = {8, 7, 6, 5, 4, 3, 2, 1};

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

void main() {
    board_init();
    rtc_init_lsi();

    bool fw_reset = firewall_reset_occurred();
    system_clear_reset_flags();
    if (rtc_backup_read(31) == 1) {
        rtc_backup_write(31, 0);
        if (!fw_reset) led_error_forever();
        led_on(LED2);                   //the out-of-segment access was stopped
        while(1);
    }
    rtc_backup_write(31, 1);

    //the segment is not covered by the startup copy: load it here
    memcpy(&_fw_vdata_start, &_fw_vdata_load, (size_t) ((char *) &_fw_vdata_end - (char *) &_fw_vdata_start));

    const firewall_config_t cfg = {
        .vdata_start = 0x2000F100, .vdata_len = 3840,
        .vdata_shared = false, .vdata_executable = true,
    };
    firewall_enable(&cfg);

    uint32_t expected = 0;
    for (uint32_t i = 0; i < 8; i++) expected += unprotected_vdata[i] * (i + 1u);
    if (vdata_protected_function(unprotected_vdata) != expected) {
        rtc_backup_write(31, 0);
        led_error_forever();
    }
    (void) *(volatile uint32_t *) &protected_vdata[0];   //outside access: firewall reset
    rtc_backup_write(31, 0);
    led_error_forever();
}
