#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* Cortex-M4 core features: MPU, Thread-mode privilege and stacks,    */
/* SVC calls, fault handlers.                                         */
/**********************************************************************/

/* ------------------------------- MPU ------------------------------- */
typedef enum {
    MPU_AP_NONE = 0,             /* no access */
    MPU_AP_PRIV_RW = 1,          /* privileged read/write, unprivileged none */
    MPU_AP_PRIV_RW_USER_RO = 2,
    MPU_AP_FULL = 3,
    MPU_AP_PRIV_RO = 5,          /* privileged read-only */
    MPU_AP_RO = 6,               /* read-only for all */
} mpu_access_t;

typedef struct {
    uint8_t number;              /* region 0..7 */
    uint32_t base;               /* aligned to its size */
    uint8_t size_log2;           /* 5..32: region = 2^size_log2 bytes */
    mpu_access_t access;
    bool execute_never;
    uint8_t tex, cacheable, bufferable, shareable;   /* memory attributes (0 = strongly ordered) */
} mpu_region_t;

void mpu_set_region(const mpu_region_t *r);
void mpu_disable_region(uint8_t number);
/** Enable the MPU; privileged code keeps the default map elsewhere. */
void mpu_enable(void);
void mpu_disable(void);

/** MemManage faults: enable the dedicated handler (else they escalate to
 *  HardFault) and call fn from it; returning resumes after the faulting
 *  instruction is skipped only if fn returns true. */
void cortex_on_memmanage_fault(bool (*fn)(void));

/* ------------------------- privilege / stacks ------------------------- */
/** Thread mode uses the process stack (PSP) from now on, top at @p psp_top. */
void cortex_use_process_stack(uint32_t *psp_top);
/** Thread mode unprivileged (CONTROL.nPRIV). One-way for thread code. */
void cortex_drop_privilege(void);
/** Back to privileged Thread mode: done by an SVC, since only handler mode
 *  may write CONTROL.nPRIV = 0. */
void cortex_regain_privilege(void);
bool cortex_is_privileged(void);
bool cortex_uses_process_stack(void);
uint32_t cortex_get_psp(void);
uint32_t cortex_get_msp(void);

/* -------------------------------- SVC -------------------------------- */
/** Register a handler for "svc #n" with n in 1..15 (0 is used by
 *  cortex_regain_privilege). Runs in handler mode (privileged). */
void cortex_svc_register(uint8_t n, void (*fn)(void));
#define cortex_svc(n) __asm volatile ("svc %0" :: "i" (n) : "memory")

/* ------------------------------ SysTick ------------------------------ */
/** Reload / current value / flag access for timing without interrupts. */
uint32_t cortex_systick_reload(void);
