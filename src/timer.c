#include <stddef.h>
#include "libstefi/timer.h"

#include <stdbool.h>
#include <libstefi/peripheral.h>
#include <libstefi/util.h>
#include "internal/timer_internal.h"
#if defined(STEFI_MCU_STM32L4G4)
#include "internal/mcu_id.h"
#else
static inline bool mcu_is_g4(void) { return false; }
#endif

TIM_t *timers[] = {(TIM_t*) NULL, TIM1_BASE, TIM2_BASE, TIM3_BASE, TIM4_BASE, NULL, TIM6_BASE, TIM7_BASE, TIM8_BASE,
    NULL, NULL, NULL, NULL, NULL, NULL, TIM15_BASE, TIM16_BASE, TIM17_BASE, NULL, NULL, NULL};

static const enum _nvic_interrupt_sources timerid_to_irq[19] = {
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_TIM1_TIM16,
    INTERRUPT_SOURCE_TIM2,
    INTERRUPT_SOURCE_TIM3,
    INTERRUPT_SOURCE_TIM4,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_TIM6,
    INTERRUPT_SOURCE_TIM7,
    INTERRUPT_SOURCE_TIM8,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_NONE,
    INTERRUPT_SOURCE_TIM15,
    INTERRUPT_SOURCE_TIM1_TIM16,
    INTERRUPT_SOURCE_TIM17,
};

static struct {
    callbackfn_t callback[TIMER_IRQ_MAX];
    void *aux_data;
} timer_handlers[NUM_TIMERINTERRUPTS] = {0};

void timer_init(const tim_id_t timer_id) {
    //ASSERT(timer_id > 0);
    assert(timer_id > 0);
    peripheral_tim_enable(timer_id);
}

void timer_start(const tim_id_t timer_id)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    tim->SR = 0;
    tim->CR1 |= BIT(0);
}

void timer_stop(const tim_id_t timer_id)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    tim->CR1 &= ~BIT(0);
    tim->CNT = 0;
}

uint32_t timer_get_count(const tim_id_t timer_id)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    return tim->CNT;
}

uint32_t timer_get_arr(const tim_id_t timer_id) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    return tim->ARR;
}

uint32_t timer_get_compare(const tim_id_t timer_id, timer_channel_t channel)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint32_t val = 0;
    switch(channel) {
        case 1: val = tim->CCR1; break;
        case 2: val = tim->CCR2; break;
        case 3: val = tim->CCR3; break;
        case 4: val = tim->CCR4; break;
        default: break;
    }
    return val;
}

void timer_setcount(const tim_id_t timer_id, uint32_t cnt)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    assert(cnt <= tim->ARR);

    tim->CNT = cnt;
}

void timer_set_period(const tim_id_t timer_id, uint16_t prescaler, uint32_t period)
{
    //assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    tim->PSC = prescaler - 1; //Set prescaler
    tim->ARR = period - 1; //Set period
    tim->CNT = 0;
    tim->EGR |= BIT(0);  // Load new ARR immediately
}

void timer_set_mode_opm(const tim_id_t timer_id) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    tim->CR1 |= BIT(3);
}

void timer_wait_for_update_flag(const tim_id_t timer_id) {
    //assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    while (!(tim->SR & BIT(0)));
    tim->SR = 0; // Clear flag
}

void timer_set_mode_pwm(const tim_id_t timer_id, timer_channel_t channel)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    uint8_t shift = (channel & 1) ? 0 : 8;
    uint8_t mode = 6; //0110 = PWM Mode 1, add mode 2 later
    //set mode
    if (channel == 1 || channel == 2) {
        tim->CCMR1 |=  mode << (shift+4);       // OCM2 field = PWM
        tim->CCMR1 |= BIT(3) << shift; // Output compare 1 preload enable
    } else {
        tim->CCMR2 |=  mode << (shift + 4);       // OCM2 field = PWM
        tim->CCMR2 |= BIT(3) << shift; // Output compare 2 preload enable}
    }
}

void timer_set_mode_ic(const tim_id_t timer_id, timer_channel_t channel)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    tim->PSC = 0;
    tim->ARR = 0xFFFF;
    tim->CR1 |= BIT(7); // ARPE: enable shadow registers of ARR

    uint8_t shift = (channel & 1) ? 0 : 8;
    //set mode
    if (channel == 1 || channel == 2) {
        tim->CCMR1 &= ~(3 << shift);
        tim->CCMR1 |= 1 << shift; // CC1 or 2 configured as input, IC1,2 mapped on TI1,2
        tim->CCMR1 &= ~(0xF << (shift+4)); //reset input capture filter
        tim->CCMR1 |= (0x1 << (shift+4)); //0b0001: fSAMPLING=fCK_INT, N=2 consective event validate a transition
    } else {
        tim->CCMR2 &= ~(3 << shift);
        tim->CCMR2 |= 1 << shift; // CC3 or 4 configured as input, IC3,4 mapped on TI3,4
        tim->CCMR2 &= ~(0xF << (shift+4)); //reset input capture filter
        tim->CCMR2 |= (0x1 << (shift+4)); //0b0001: fSAMPLING=fCK_INT, N=2 consective event validate a transition
    }

}

void timer_set_compare(const tim_id_t timer_id, timer_channel_t channel, uint32_t duty)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];

    //set compare register
    switch(channel) {
        case 1:
            tim->CCR1 = duty;
            break;
        case 2:
            tim->CCR2 = duty;
            break;
        case 3:
            tim->CCR3 = duty;
            break;
        case 4:
            tim->CCR4 = duty;
            break;
        default:
            break;
        }
}

static void timer_set_cc_io(const tim_id_t timer_id, uint32_t channel, bool is_input, uint8_t shift) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    volatile uint8_t polarity = 1;
    if(is_input) {
        // CC1NP=0, CC1P=0: sensitive to rising edge
        tim->CCER &= ~(0x1 << (shift + 1)| 0x1  << (shift+3));
        // enable capture from counter in capture register
        tim->CCER |= BIT(shift); //enable input
        tim->DIER |= BIT(channel); // enable cc1 interrupt generation
    } else {
        tim->CCER |= polarity << (shift + 1);  // OC signal is active low
        tim->CCER |= BIT(shift); //enable output
        if(timer_id == 1 || timer_id == 8) {
            tim->BDTR |= BIT(15); //Master Output Enable
        }
    }
}

void timer_set_ic_edge(const tim_id_t timer_id, timer_channel_t channel, timer_edge_t edge) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint8_t shift = (channel -1) * 4;
        // CC1NP=0, CC1P=0: sensitive to rising edge
    if(edge == TIMER_EDGE_RISING) {
        tim->CCER &= ~(0x1 << (shift + 1)| 0x1  << (shift+3));
    } else if(edge == TIMER_EDGE_FALLING) {
        tim->CCER |= 0x1 << (shift + 1);
    }
}

void timer_set_oc_polarity(const tim_id_t timer_id, timer_channel_t channel, bool active_high) {
    assert(timer_id > 0);
    uint32_t bit = 1u << ((channel - 1u) * 4u + 1u);   // CCxP
    if (active_high) timers[timer_id]->CCER &= ~bit; else timers[timer_id]->CCER |= bit;
}

void timer_set_master_mode(const tim_id_t timer_id, timer_trgo_t trgo, bool sync_slaves) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    tim->CR2 = (tim->CR2 & ~(0x7u << 4)) | ((uint32_t) trgo << 4);   // MMS
    if (sync_slaves) tim->SMCR |= BIT(7); else tim->SMCR &= ~BIT(7); // MSM
}

void timer_set_slave_mode(const tim_id_t timer_id, timer_slave_mode_t mode, timer_trigger_t trigger) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    // SMS[2:0] at [2:0] (SMS[3] at [16] unused), TS[2:0] at [6:4]
    tim->SMCR = (tim->SMCR & ~(0x7u | (0x7u << 4) | BIT(16) | (0x3u << 20)))
              | ((uint32_t) trigger << 4) | (uint32_t) mode;
}

void timer_set_ic_source(const tim_id_t timer_id, timer_channel_t channel, bool indirect) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    volatile uint32_t *ccmr = (channel <= 2) ? &tim->CCMR1 : &tim->CCMR2;
    uint32_t shift = (channel & 1u) ? 0u : 8u;
    // CCxS: 01 = direct (TIx), 10 = indirect (neighbour TI) — only while CCxE = 0
    tim->CCER &= ~(1u << ((channel - 1u) * 4u));
    *ccmr = (*ccmr & ~(0x3u << shift)) | ((indirect ? 2u : 1u) << shift);
}

void timer_set_repetition(const tim_id_t timer_id, uint8_t n) {
    assert(timer_id > 0);
    timers[timer_id]->RCR = n;
}

volatile uint32_t *timer_ccr_address(const tim_id_t timer_id, timer_channel_t channel) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    switch (channel) {
        case 1: return &tim->CCR1;
        case 2: return &tim->CCR2;
        case 3: return &tim->CCR3;
        default: return &tim->CCR4;
    }
}

void timer_set_oc_mode(const tim_id_t timer_id, timer_channel_t channel, timer_oc_mode_t mode) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    volatile uint32_t *ccmr = (channel <= 2) ? &tim->CCMR1 : &tim->CCMR2;
    uint32_t shift = (channel & 1u) ? 0u : 8u;
    uint32_t v = *ccmr & ~((0xFFu | BIT(16)) << shift);          // CCxS = output, OCxM, OCxPE
    v |= ((uint32_t) mode << (shift + 4u));
    if (mode >= TIMER_OC_PWM1) v |= BIT(3) << shift;             // preload for PWM
    *ccmr = v;
}

void timer_enable_complementary(const tim_id_t timer_id, timer_channel_t channel, bool active_high) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint32_t sh = (channel - 1u) * 4u;
    tim->CCER = (tim->CCER & ~(1u << (sh + 3u))) | (active_high ? 0u : (1u << (sh + 3u))) | (1u << (sh + 2u));
    tim->BDTR |= BIT(15);                                        // MOE
}

void timer_cc_interrupt(const tim_id_t timer_id, timer_channel_t channel, bool enable) {
    assert(timer_id > 0);
    if (enable) timers[timer_id]->DIER |= BIT(channel); else timers[timer_id]->DIER &= ~BIT(channel);
}

volatile uint32_t *timer_dma_burst(const tim_id_t timer_id, uint8_t base, uint8_t count) {
    assert(timer_id > 0);
    // DCR/DMAR: L4 at 0x48/0x4C, G4 at 0x3DC/0x3E0
    uint32_t b = (uint32_t) timers[timer_id];
    bool g4 = mcu_is_g4();
    volatile uint32_t *dcr = (volatile uint32_t *) (b + (g4 ? 0x3DCu : 0x48u));
    *dcr = (uint32_t) (base & 0x1Fu) | ((uint32_t) ((count - 1u) & 0x1Fu) << 8);
    return (volatile uint32_t *) (b + (g4 ? 0x3E0u : 0x4Cu));
}

void timer_generate_update(const tim_id_t timer_id) {
    assert(timer_id > 0);
    timers[timer_id]->EGR |= BIT(0);
}

nvic_source_t timer_get_interrupt_source(const tim_id_t timer_id) {
    return timerid_to_irq[timer_id];
}

void timer_enable_dma_request(const tim_id_t timer_id, timer_dma_request_t request) {
    assert(timer_id > 0);
    timers[timer_id]->DIER |= BIT(request);
}

void timer_disable_dma_request(const tim_id_t timer_id, timer_dma_request_t request) {
    assert(timer_id > 0);
    timers[timer_id]->DIER &= ~BIT(request);
}

/*******Interrupts*****************/
void timer_update_interrupt_register_handler(const tim_id_t timer_id, callbackfn_t fn) {
    timer_handlers[timer_id].callback[TIMER_UPDATE_IRQ] = fn;
}

void timer_cc_interrupt_register_handler(const tim_id_t timer_id, timer_interrupt_t channel, callbackfn_t fn) {
    timer_handlers[timer_id].callback[channel] = fn;
}

static void timer_dispatch(const tim_id_t timer_id, const timer_interrupt_t irq_type)
{
    timer_handlers[timer_id].callback[irq_type](timer_handlers[timer_id].aux_data);
}

static inline void timer_clear_interruptflag(const tim_id_t timer_id) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint32_t interrupt_flag = tim->SR;
    if ((interrupt_flag & BIT(0)) && timer_handlers[timer_id].callback[TIMER_UPDATE_IRQ]) {
        tim->SR &= ~BIT(0);
    }
    if ((interrupt_flag & BIT(1)) && timer_handlers[timer_id].callback[TIMER_CC1_IRQ]) {
        tim->SR &= ~BIT(1);
    }
    if ((interrupt_flag & BIT(2)) && timer_handlers[timer_id].callback[TIMER_CC2_IRQ]) {
        tim->SR &= ~BIT(2);
    }
    if ((interrupt_flag & BIT(3)) && timer_handlers[timer_id].callback[TIMER_CC3_IRQ]) {
        tim->SR &= ~BIT(3);
    }
    if ((interrupt_flag & BIT(4)) && timer_handlers[timer_id].callback[TIMER_CC4_IRQ]) {
        tim->SR &= ~BIT(4);
    }
}

static inline void handle_timer_irq(const tim_id_t timer_id) {
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint32_t interrupt_flag = tim->SR;
    // An enabled flag nobody listens to must still be cleared, or the
    // interrupt re-enters forever (e.g. CC1IE set by timer_cc_enable() on
    // the second channel of a PWM-input pair).
    uint32_t orphan = 0;
    for (uint32_t i = 0; i < TIMER_IRQ_MAX; i++) {
        if (!timer_handlers[timer_id].callback[i]) orphan |= BIT(i);
    }
    orphan &= interrupt_flag & tim->DIER & 0x1Fu;
    if (orphan) tim->SR = ~orphan;
    if ((interrupt_flag & BIT(0)) && timer_handlers[timer_id].callback[TIMER_UPDATE_IRQ]) {
        tim->SR &= ~BIT(0);
        timer_dispatch(timer_id, 0);
    }
    if ((interrupt_flag & BIT(1)) && timer_handlers[timer_id].callback[TIMER_CC1_IRQ]) {
        tim->SR &= ~BIT(1);
        timer_dispatch(timer_id, 1);
    }
    if ((interrupt_flag & BIT(2)) && timer_handlers[timer_id].callback[TIMER_CC2_IRQ]) {
        tim->SR &= ~BIT(2);
        timer_dispatch(timer_id, 2);
    }
    if ((interrupt_flag & BIT(3)) && timer_handlers[timer_id].callback[TIMER_CC3_IRQ]) {
        tim->SR &= ~BIT(3);
        timer_dispatch(timer_id, 3);
    }
    if ((interrupt_flag & BIT(4)) && timer_handlers[timer_id].callback[TIMER_CC4_IRQ]) {
        tim->SR &= ~BIT(4);
        timer_dispatch(timer_id, 4);
    }
}

void timer_enable_interrupt(const tim_id_t timer_id)
{
    assert(timer_id > 0);

    TIM_t *tim = timers[timer_id];

    tim->DIER |= BIT(0); //enable interrupt
    tim->EGR |= BIT(0); //reset timer
    timer_clear_interruptflag(timer_id);
    interrupts_enable_source(timerid_to_irq[timer_id]);
}

void timer_cc_enable(const tim_id_t timer_id, timer_channel_t channel, bool is_input)
{
    assert(timer_id > 0);
    TIM_t *tim = timers[timer_id];
    uint8_t shift;

    switch(channel) {
        case 1:
            shift = 0;
        timer_set_cc_io(timer_id, channel, is_input, shift);
        break;
        case 2:
            shift = 4;
        timer_set_cc_io(timer_id, channel, is_input, shift);
        break;
        case 3:
            shift = 8;
        timer_set_cc_io(timer_id, channel, is_input, shift);
        break;
        case 4:
            shift = 12;
        timer_set_cc_io(timer_id, channel, is_input, shift);
        break;
        default:
            break;
    }

    tim->CR1 |= BIT(7); // ARPE: Enable auto-reload preload & UPDATE GENERATION
    tim->EGR |= BIT(0); //reset timer: preloads the shadow registers
    timer_clear_interruptflag(timer_id);
    interrupts_enable_source(timerid_to_irq[timer_id]);
#if defined(STEFI_MCU_STM32L4G4)
    // TIM1/TIM8 capture/compare interrupts have their own NVIC line
    // (TIM1_CC = 27, TIM8_CC = 46 on L4 and G4)
    if (is_input && timer_id == TIMER1) interrupts_enable_source((nvic_source_t) 27);
    if (is_input && timer_id == TIMER8) interrupts_enable_source((nvic_source_t) 46);
#endif
}
void TIM2_IRQHandler(void) {handle_timer_irq(TIMER2);}
void TIM3_IRQHandler(void) {handle_timer_irq(TIMER3);}
void TIM4_IRQHandler(void) {handle_timer_irq(TIMER4);}
void TIM6_IRQHandler(void) {handle_timer_irq(TIMER6);}
void TIM7_IRQHandler(void) {handle_timer_irq(TIMER7);}
void TIM8_IRQHandler(void) {handle_timer_irq(TIMER8);}
void TIM15_IRQHandler(void) {handle_timer_irq(TIMER15);}
void TIM16_IRQHandler(void) {
    timer_clear_interruptflag(TIMER1);
    handle_timer_irq(TIMER16);
}
void TIM17_IRQHandler(void) {handle_timer_irq(TIMER17);}
#if defined(STEFI_MCU_STM32L4G4)
void TIM1_CC_IRQHandler(void) {handle_timer_irq(TIMER1);}
void TIM8_CC_IRQHandler(void) {handle_timer_irq(TIMER8);}
#endif
#if defined(STEFI_MCU_STM32F7)
/* F7 vector 25 (TIM1_UP_TIM10): TIM1's update interrupt; TIMER16 aliases TIM10. */
void TIM1_UP_TIM10_IRQHandler(void) {
    handle_timer_irq(TIMER1);
    handle_timer_irq(TIMER16);
}
#endif
