/* CORTEXM_MPU (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * Built twice: l4_cortexm_mpu (reads only) and l4_cortexm_mpu_fault (WRITE_TO_RO).
 *
 * MPU regions: SRAM (128 KB) full access, flash (1 MB) read-only,
 * peripherals (512 MB, execute-never) full access, and a 32-byte array as
 * region 3: privileged read-only. Reading the array is allowed — LD2
 * toggles. With WRITE_TO_RO the program writes to it: MemManage fault,
 * LD2 stays on. */
#include "l4_example.h"
#include "libstefi/cortex.h"

static uint8_t privileged_ro_array[32] __attribute__((aligned(32)));

static bool on_memmanage(void)
{
    led_on(LED2);
    for (;;);                        // stay here, LD2 on
    return false;
}

void main(void)
{
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

    for (;;) {
        led_toggle(LED2);
        systick_delay_ms(100);
    }
}
