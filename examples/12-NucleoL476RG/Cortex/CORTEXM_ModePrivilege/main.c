/* CORTEXM_ModePrivilege (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 *  1. Thread mode stack: main stack -> process stack (PSP, 512 bytes)
 *  2. Thread mode: privileged -> unprivileged
 *  3. back to privileged: writing CONTROL from unprivileged code has no
 *     effect, so an SVC handler (privileged) does it
 * The state after each step is kept in the variables below (watch them in
 * the debugger) and printed on the VCP. LD2 on: the sequence behaved as
 * expected. */
#include <stdio.h>

#include "l4_example.h"
#include "libstefi/cortex.h"

#define SP_PROCESS_SIZE 0x200u

static uint32_t psp_stack[SP_PROCESS_SIZE / 4] __attribute__((aligned(8)));
volatile bool on_process_stack, privileged_after_drop, privileged_after_write, privileged_after_svc;
volatile uint32_t psp_value;

void main(void)
{
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
        example_success();
    }
    example_error();
}
