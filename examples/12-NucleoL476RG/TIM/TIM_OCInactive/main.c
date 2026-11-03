/* TIM_OCInactive (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM1, 10 kHz; CH1..CH4 in output-compare "inactive" mode with CC
 * interrupts. PC5, PC6, PC8, PC9 are set high at start; each CCx
 * interrupt (1 s, 500 ms, 250 ms, 125 ms) clears the matching pin. */
#include "l4_example.h"
#include "libstefi/timer.h"
#include "libstefi/peripheral.h"

static const gpio_id_t out[4] = {C5, C6, C8, C9};
static const uint32_t pulse[4] = {10000, 5000, 2500, 1250};

static void cc1(void *a) { (void) a; gpio_write(C5, LOW); }
static void cc2(void *a) { (void) a; gpio_write(C6, LOW); }
static void cc3(void *a) { (void) a; gpio_write(C8, LOW); }
static void cc4(void *a) { (void) a; gpio_write(C9, LOW); }

void main(void)
{
    board_init();
    peripheral_gpio_enable(PERIPHERAL_GPIOC);
    for (int i = 0; i < 4; i++) { gpio_set_mode(out[i], MODER_OUTPUT); gpio_set_pupd(out[i], PULL_UP); }

    timer_init(TIMER1);
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 10000u), 65536);
    const callbackfn_t cb[4] = {cc1, cc2, cc3, cc4};
    for (int i = 0; i < 4; i++) {
        timer_channel_t c = (timer_channel_t) (i + 1);
        timer_set_oc_mode(TIMER1, c, TIMER_OC_INACTIVE);
        timer_set_compare(TIMER1, c, pulse[i]);
        timer_cc_interrupt_register_handler(TIMER1, (timer_interrupt_t) (TIMER_CC1_IRQ + i), cb[i]);
        timer_cc_enable(TIMER1, c, true);             // CCxIE + TIM1_CC interrupt line
        timer_set_oc_mode(TIMER1, c, TIMER_OC_INACTIVE);   // cc_enable(input) set CCxS: back to output
    }
    for (int i = 0; i < 4; i++) gpio_write(out[i], HIGH);
    timer_start(TIMER1);
    for (;;);
}
