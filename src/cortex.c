/**********************************************************************/
/* Cortex-M4 core: MPU (ARMv7-M), CONTROL register, SVC               */
/**********************************************************************/
#include "libstefi/cortex.h"
#include "libstefi/util.h"
#include "internal/cortexm4_internal.h"
#include "internal/systick_internal.h"

#include <stddef.h>


static inline void barrier(void) { __asm volatile ("dsb\n isb" ::: "memory"); }

void mpu_set_region(const mpu_region_t *r)
{
    MPU->RNR = r->number;
    MPU->RBAR = r->base & ~0x1Fu;
    MPU->RASR = ((uint32_t) (r->execute_never ? 1u : 0u) << 28)
             | ((uint32_t) r->access << 24)
             | ((uint32_t) (r->tex & 7u) << 19) | ((uint32_t) (r->shareable & 1u) << 18)
             | ((uint32_t) (r->cacheable & 1u) << 17) | ((uint32_t) (r->bufferable & 1u) << 16)
             | ((uint32_t) (r->size_log2 - 1u) << 1) | 1u;   // SIZE, ENABLE
    barrier();
}

void mpu_disable_region(uint8_t number)
{
    MPU->RNR = number;
    MPU->RASR = 0;
    barrier();
}

void mpu_enable(void)
{
    MPU->CTRL = BIT(2) | BIT(0);      // PRIVDEFENA | ENABLE
    barrier();
}

void mpu_disable(void)
{
    barrier();
    MPU->CTRL = 0;
}

static bool (*memfault_fn)(void);

void cortex_on_memmanage_fault(bool (*fn)(void))
{
    memfault_fn = fn;
    SCB->SHCSR |= BIT(16);            // MEMFAULTENA
}

/* Called with the exception stack frame; skip the faulting (16-bit or
 * 32-bit) instruction when the application says so. */
void memmanage_dispatch(uint32_t *frame)
{
    bool resume = memfault_fn ? memfault_fn() : false;
    SCB->CFSR = 0xFFu;                // clear MMFSR
    if (!resume) for (;;);
    uint16_t op = *(uint16_t *) frame[6];
    frame[6] += ((op & 0xE000u) == 0xE000u && (op & 0x1800u)) ? 4u : 2u;
}

__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile (
        "tst lr, #4      \n"
        "ite eq          \n"
        "mrseq r0, msp   \n"
        "mrsne r0, psp   \n"
        "b memmanage_dispatch \n");
}

void cortex_use_process_stack(uint32_t *psp_top)
{
    __asm volatile ("msr psp, %0" :: "r" (psp_top) : "memory");
    uint32_t c;
    __asm volatile ("mrs %0, control" : "=r" (c));
    c |= 2u;                         // SPSEL
    __asm volatile ("msr control, %0\n isb" :: "r" (c) : "memory");
}

void cortex_drop_privilege(void)
{
    uint32_t c;
    __asm volatile ("mrs %0, control" : "=r" (c));
    c |= 1u;                         // nPRIV
    __asm volatile ("msr control, %0\n isb" :: "r" (c) : "memory");
}

bool cortex_is_privileged(void)
{
    uint32_t c, ipsr;
    __asm volatile ("mrs %0, control" : "=r" (c));
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return ipsr != 0 || !(c & 1u);
}

bool cortex_uses_process_stack(void)
{
    uint32_t c;
    __asm volatile ("mrs %0, control" : "=r" (c));
    return (c & 2u) != 0;
}

uint32_t cortex_get_psp(void) { uint32_t v; __asm volatile ("mrs %0, psp" : "=r" (v)); return v; }
uint32_t cortex_get_msp(void) { uint32_t v; __asm volatile ("mrs %0, msp" : "=r" (v)); return v; }

static void (*svc_handlers[16])(void);

void cortex_svc_register(uint8_t n, void (*fn)(void))
{
    if (n > 0 && n < 16) svc_handlers[n] = fn;
}

void cortex_regain_privilege(void)
{
    cortex_svc(0);
}

void svc_dispatch(uint32_t *frame)
{
    uint8_t n = ((uint8_t *) frame[6])[-2];   // immediate of the svc instruction
    if (n == 0) {
        uint32_t c;
        __asm volatile ("mrs %0, control" : "=r" (c));
        c &= ~1u;                    // privileged Thread mode on return
        __asm volatile ("msr control, %0\n isb" :: "r" (c) : "memory");
    } else if (n < 16 && svc_handlers[n]) {
        svc_handlers[n]();
    }
}

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile (
        "tst lr, #4      \n"
        "ite eq          \n"
        "mrseq r0, msp   \n"
        "mrsne r0, psp   \n"
        "b svc_dispatch  \n");
}

uint32_t cortex_systick_reload(void)
{
    return SysTick->LOAD;
}
