/*
 * FIREWALL_VolatileData_Shared: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * The firewall protects a code segment (0x08010000, 512 B), constant data
 * (0x080FF000, 256 B) and SRAM data (0x20017000, 576 B, shared). Two firewall
 * resets are provoked on purpose: reading the constant data from outside,
 * then reading the SRAM data after the protected code made it unshared.
 * Progress is kept in RTC backup register 4. LED2 on: both resets happened as
 * expected; blinking 1 s: error. (ST: SRAM data at 0x20000000.)
 */

#include "board.h"
#include "led.h"

#include <libstefi/firewall.h>
#include <libstefi/rtc.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>

#define N 144                           //576 bytes of protected SRAM data

/* protected segments, placed by firewall.ld */
__attribute__((section(".fw_code.0_pad"), used)) static const uint32_t fw_gate_pad = 0;
__attribute__((section(".fw_nvdata"), used)) const uint32_t protected_non_volatile_array[64] = {
    0x12345678, 0x9ABCDEF0, 0x0F1E2D3C, 0x4B5A6978,
};
__attribute__((section(".fw_vdata"))) uint32_t protected_volatile_array[N];

/* Entered through the call gate: the firewall opens. Calls nothing outside. */
__attribute__((section(".fw_code.1_gate"), noinline, used))
uint32_t protected_function(uint32_t unshare) {
    for (uint32_t j = 0; j < N; j++) protected_volatile_array[j] = j * 3u + 1u;
    if (unshare) {
        *(volatile uint32_t *) 0x40011C20 &= ~2u;    //FW_CR.VDS = 0 (only from inside)
    }
    return protected_non_volatile_array[0];
}

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

void main() {
    board_init();
    rtc_init_lsi();                     //backup registers

    uint32_t stage = rtc_backup_read(4);
    bool fw_reset = firewall_reset_occurred();
    system_clear_reset_flags();
    if (stage == 2 && fw_reset) {       //second firewall reset: done
        rtc_backup_write(4, 0);
        led_on(LED2);
        while(1);
    }
    if (stage == 1 && fw_reset) {
        stage = 2;
    } else {
        stage = 1;
    }
    rtc_backup_write(4, stage);

    const firewall_config_t cfg = {
        .code_start = 0x08010000, .code_len = 512,
        .nvdata_start = 0x080FF000, .nvdata_len = 256,
        .vdata_start = 0x20017000, .vdata_len = 576,
        .vdata_shared = true, .vdata_executable = false,
    };
    firewall_enable(&cfg);
    if (protected_function(stage == 2) != 0x12345678) led_error_forever();

    if (stage == 1) {
        //shared SRAM data: readable from outside
        for (uint32_t j = 0; j < N; j++) {
            if (protected_volatile_array[j] != j * 3u + 1u) led_error_forever();
        }
        systick_delay_ms(1);
        (void) *(volatile const uint32_t *) &protected_non_volatile_array[0];   //firewall reset
    } else {
        (void) *(volatile uint32_t *) &protected_volatile_array[0];              //now unshared: reset
    }
    led_error_forever();                //not reached when the firewall works
}
