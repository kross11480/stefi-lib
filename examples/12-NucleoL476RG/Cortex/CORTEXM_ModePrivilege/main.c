/*
 * CORTEXM_ModePrivilege: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Thread mode to the process stack, unprivileged, and back to privileged
 * through an SVC. LED2 on: sequence as expected.
 */

#include "board.h"
#include "led.h"

#include <libstefi/cortex.h>
#include <libstefi/systick.h>

#include <stdio.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* LED2 on: passed */
static void led_success_forever() {
    led_on(LED2);
    while(1);
}

#define SP_PROCESS_SIZE 0x200u

static uint32_t psp_stack[SP_PROCESS_SIZE / 4] __attribute__((aligned(8)));
volatile bool on_process_stack, privileged_after_drop, privileged_after_write, privileged_after_svc;
volatile uint32_t psp_value;

void main() {
    board_init();

    cortex_use_process_stack(&psp_stack[SP_PROCESS_SIZE / 4]);
    on_process_stack = cortex_uses_process_stack();
    psp_value = cortex_get_psp();

    cortex_drop_privilege();
    privileged_after_drop = cortex_is_privileged();

    // an unprivileged write of CONTROL.nPRIV = 0 is ignored by the core
    uint32_t c;
    __asm volatile ("mrs %0, control" : "=r" (c));
    c &= ~1u;
    __asm volatile ("msr control, %0\n isb" :: "r" (c) : "memory");
    privileged_after_write = cortex_is_privileged();

    cortex_regain_privilege();       // svc #0
    privileged_after_svc = cortex_is_privileged();

    printf("PSP in use: %d (0x%08lx), unprivileged: %d, write ignored: %d, after SVC privileged: %d\r\n",
           on_process_stack, (unsigned long) psp_value, !privileged_after_drop,
           !privileged_after_write, privileged_after_svc);

    if (on_process_stack && !privileged_after_drop && !privileged_after_write && privileged_after_svc) {
        led_success_forever();
    }
    led_error_forever();
}
