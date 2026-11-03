/* COMP_Interrupt (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * COMP1 compares the voltage on PC5 (+) with VREFINT (1.22 V, -) in
 * ultra-low-power mode. Each crossing (rising or falling) raises an
 * interrupt (EXTI line 21) that toggles LD2 — and wakes the MCU from
 * Stop 2, which it enters 5 s after start and after every wakeup.
 * Apply a variable voltage (0..3.3 V) to PC5. */
#include "l4_example.h"
#include "libstefi/comp.h"
#include "libstefi/pwr.h"

static void on_crossing(void *aux)
{
    (void) aux;
    led_toggle(LED2);
}

void main(void)
{
    board_init();
    led_off(LED2);

    const comp_config_t cfg = {.instance = 1, .inp = 0, .inm = COMP_INM_VREFINT,
                               .power = COMP_POWER_ULTRALOW};
    comp_init(&cfg);
    comp_enable_interrupt(1, true, true, on_crossing, NULL);

    for (;;) {
        systick_delay_ms(5000);
        pwr_enter_stop2();
        example_restore_clock();     // woken by the comparator
    }
}
