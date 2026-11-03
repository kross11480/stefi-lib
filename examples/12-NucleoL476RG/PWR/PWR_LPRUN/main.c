/* PWR_LPRUN (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * SYSCLK = MSI 32 MHz. LD2 toggles (Run mode) for 5 s; then the clock
 * drops to MSI 100 kHz (range 0) and the regulator to low-power run mode
 * (LD2 off). A B1 press (EXTI 13) exits low-power run: main regulator,
 * back to 32 MHz, 5 s of Run, and so on. */
#include "l4_example.h"
#include "libstefi/pwr.h"

static volatile bool button;

static void on_button(void *aux) { (void) aux; button = true; }

static void run_clock(void)
{
    system_init_l4_msi(10);                  // 32 MHz
    systick_init(32000000u);
    systick_start();
}

void main(void)
{
    board_init();
    run_clock();
    example_button_irq(on_button);
    for (;;) {
        for (uint32_t t = 0; t < 5000; t += 100) { led_toggle(LED2); systick_delay_ms(100); }
        led_off(LED2);
        systick_stop();
        system_init_l4_msi(0);               // 100 kHz
        button = false;
        pwr_lprun_enter();
        while (!button);                     // low-power run until B1
        pwr_lprun_exit();
        run_clock();
    }
}
