#include "event_fsm.h"

#include <stddef.h>

#if defined(__arm__)
static inline uint32_t irq_save(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask\n cpsid i" : "=r" (primask) :: "memory");
    return primask;
}

static inline void irq_restore(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}
#else   /* host build (unit tests): no interrupts to mask */
static inline uint32_t irq_save(void) { return 0; }
static inline void irq_restore(uint32_t primask) { (void) primask; }
#endif

void fsm_init(fsm_t *fsm, uint8_t *queue_buf, uint16_t queue_size, void *ctx)
{
    fsm->state = NULL;
    fsm->ctx = ctx;
    fsm->dropped = 0;
    rb_init(&fsm->queue, queue_buf, queue_size);
}

void fsm_dispatch(fsm_t *fsm, fsm_event_t event)
{
    if (fsm->state) fsm->state(fsm, event);
}

void fsm_start(fsm_t *fsm, fsm_state_t initial)
{
    fsm->state = initial;
    fsm_dispatch(fsm, FSM_EV_ENTRY);
}

void fsm_transition(fsm_t *fsm, fsm_state_t next)
{
    fsm_dispatch(fsm, FSM_EV_EXIT);
    fsm->state = next;
    fsm_dispatch(fsm, FSM_EV_ENTRY);
}

bool fsm_post(fsm_t *fsm, fsm_event_t event)
{
    bool ok;
    uint32_t key = irq_save();   // producers: several ISRs of any priority
    ok = !rb_is_full(&fsm->queue);
    if (ok) rb_insert(&fsm->queue, event); else fsm->dropped++;
    irq_restore(key);
    return ok;
}

bool fsm_get(fsm_t *fsm, fsm_event_t *event)
{
    bool ok;
    uint32_t key = irq_save();
    ok = !rb_is_empty(&fsm->queue);
    if (ok) *event = rb_remove(&fsm->queue);
    irq_restore(key);
    return ok;
}

bool fsm_poll(fsm_t *fsm)
{
    fsm_event_t ev;
    if (!fsm_get(fsm, &ev)) return false;
    fsm_dispatch(fsm, ev);
    return true;
}

void fsm_flush(fsm_t *fsm)
{
    uint32_t key = irq_save();
    fsm->queue.head = fsm->queue.tail;
    irq_restore(key);
}
