/*
 * CORTEXM_MPU: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * MPU regions for SRAM, flash, peripherals and a privileged read-only array;
 * LED2 toggles. Variant _fault writes the array: MemManage fault, LED2 on.
 */

#include "board.h"
#include "led.h"

#include <libstefi/cortex.h>
#include <libstefi/systick.h>

static uint8_t privileged_ro_array[32] __attribute__((aligned(32)));

static bool on_memmanage(void) {
    led_on(LED2);
    while(1);                        // stay here, LD2 on
    return false;
}

void main() {
    board_init();

    mpu_disable();
    mpu_set_region(&(mpu_region_t) {.number = 0, .base = 0x20000000u, .size_log2 = 17,
                                     .access = MPU_AP_FULL, .cacheable = 1, .shareable = 1});
    mpu_set_region(&(mpu_region_t) {.number = 1, .base = 0x08000000u, .size_log2 = 20,
                                     .access = MPU_AP_RO, .cacheable = 1});
    mpu_set_region(&(mpu_region_t) {.number = 2, .base = 0x40000000u, .size_log2 = 29,
                                     .access = MPU_AP_FULL, .execute_never = true, .bufferable = 1});
    mpu_set_region(&(mpu_region_t) {.number = 3, .base = (uint32_t) privileged_ro_array, .size_log2 = 5,
                                     .access = MPU_AP_PRIV_RO, .execute_never = true, .cacheable = 1});
    cortex_on_memmanage_fault(on_memmanage);
    mpu_enable();

    (void) *(volatile uint8_t *) &privileged_ro_array[0];    // read: allowed
#if defined(WRITE_TO_RO)
    *(volatile uint8_t *) &privileged_ro_array[0] = 'e';     // write: MemManage fault
#endif

    while(1) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
}
