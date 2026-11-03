/* PWR_LPRUN_SRAM1 (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * As PWR_LPRUN, but while in low-power run the code waiting for B1 runs
 * from SRAM1 and the flash is powered down (FLASH_ACR.RUN_PD), the lowest
 * LP-run current. B1 is polled from that RAM routine (no interrupt: the
 * vector table and handlers are in flash). */
#include "l4_example.h"
#include "libstefi/pwr.h"

#define FLASH_ACR    (*(volatile uint32_t *) 0x40022000u)
#define FLASH_PDKEYR (*(volatile uint32_t *) 0x40022004u)
#define GPIOC_IDR    (*(volatile uint32_t *) 0x48000810u)
#define PWR_CR1_REG  (*(volatile uint32_t *) 0x40007000u)
#define PWR_SR2_REG  (*(volatile uint32_t *) 0x40007014u)

/* Runs from SRAM: flash off, wait for B1 (PC13 low), flash back on. */
__attribute__((section(".data.ramfunc"), noinline, long_call))
static void wait_button_flash_off(void)
{
    FLASH_PDKEYR = 0x04152637u;
    FLASH_PDKEYR = 0xFAFBFCFDu;
    FLASH_ACR |= (1u << 13);                 // RUN_PD: flash in power-down
    PWR_CR1_REG |= (1u << 14);               // LPR
    while (GPIOC_IDR & (1u << 13));          // until B1 pressed
    while (!(GPIOC_IDR & (1u << 13)));       // and released
    PWR_CR1_REG &= ~(1u << 14);
    while (PWR_SR2_REG & (1u << 9));         // REGLPF
    FLASH_PDKEYR = 0x04152637u;
    FLASH_PDKEYR = 0xFAFBFCFDu;
    FLASH_ACR &= ~(1u << 13);                // flash on
    for (volatile uint32_t i = 0; i < 100; i++);   // tFLASH wakeup
}

void main(void)
{
    board_init();
    for (;;) {
        system_init_l4_msi(10);
        systick_init(32000000u);
        systick_start();
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        system_init_l4_msi(0);               // 100 kHz
        __asm volatile ("cpsid i");
        wait_button_flash_off();
        __asm volatile ("cpsie i");
    }
}
