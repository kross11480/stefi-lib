/**********************************************************************/
/* STM32F76x/77x startup: reset handler + vector table (RM0410 §11)   */
/**********************************************************************/
#include <stdint.h>

extern unsigned int _estack;  //Top of Stack
extern unsigned int _sdata;   //start .data in SRAM
extern unsigned int _edata;   //end .data in SRAM
extern unsigned int _sidata;  //start .data in FLASH
extern unsigned int _sbss;    //start .bss (allocated in RAM)
extern unsigned int _ebss;    //end .bss

extern void main();

/* Faults and unused vectors park here: break into an attached debugger. */
void Default_Handler(void)
{
    for (;;) __asm__ volatile("BKPT #0");
}

/* Handlers implemented by the HAL (gpio.c, timer.c, systick.c, dma.c,
 * usb_cdc.c). Declared weak-aliased so an app can still link without them. */
#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))
void HardFault_Handler(void) WEAK_ALIAS;
void MemManage_Handler(void) WEAK_ALIAS;
void BusFault_Handler(void) WEAK_ALIAS;
void UsageFault_Handler(void) WEAK_ALIAS;
void SysTick_IRQHandler(void) WEAK_ALIAS;
void EXTI0_IRQHandler(void) WEAK_ALIAS;
void EXTI1_IRQHandler(void) WEAK_ALIAS;
void EXTI2_IRQHandler(void) WEAK_ALIAS;
void EXTI3_IRQHandler(void) WEAK_ALIAS;
void EXTI4_IRQHandler(void) WEAK_ALIAS;
void EXTI5_9_IRQHandler(void) WEAK_ALIAS;
void EXTI10_15_IRQHandler(void) WEAK_ALIAS;
void TIM15_IRQHandler(void) WEAK_ALIAS;       /* TIM1_BRK_TIM9 */
void TIM1_UP_TIM10_IRQHandler(void) WEAK_ALIAS;
void TIM17_IRQHandler(void) WEAK_ALIAS;       /* TIM1_TRG_COM_TIM11 */
void TIM2_IRQHandler(void) WEAK_ALIAS;
void TIM3_IRQHandler(void) WEAK_ALIAS;
void TIM4_IRQHandler(void) WEAK_ALIAS;
void TIM6_IRQHandler(void) WEAK_ALIAS;
void TIM7_IRQHandler(void) WEAK_ALIAS;
void TIM8_IRQHandler(void) WEAK_ALIAS;        /* TIM8_UP_TIM13 */
void DMA1_Stream0_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream1_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream2_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream3_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream4_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream5_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream6_IRQHandler(void) WEAK_ALIAS;
void DMA1_Stream7_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream0_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream1_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream2_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream3_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream4_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream5_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream6_IRQHandler(void) WEAK_ALIAS;
void DMA2_Stream7_IRQHandler(void) WEAK_ALIAS;
void OTG_FS_IRQHandler(void) WEAK_ALIAS;

void Reset_Handler(void)
{
    unsigned int *init_data_flash = &_sidata;
    /*bss (better save stack) holds uninitialized global and static vars */
    for (unsigned int *data_bss = &_sbss; data_bss < &_ebss; data_bss++)
    {
        *data_bss = 0;
    }
    /* Copy init data from FLASH to RAM: otherwise initialized global var does not work */
    for (unsigned int *data_sram = &_sdata; data_sram < &_edata;) {
        *data_sram++ = *init_data_flash++;
    }

    /* Branch to main() function */
    main();

    /* infinite loop in case main() returns */
    while (1);
}

#define NUM_IRQS 110   /* STM32F76x/77x: IRQ 0..109 */

__attribute__ ((section (".isr_vector"), used))
void (* const paIsrFunc[16 + NUM_IRQS])(void) =
{
    /* --- core vectors --- */
    (void *)&_estack,          /* initial stack pointer */
    Reset_Handler,             /* initial program counter */
    Default_Handler,           /* NMI */
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    Default_Handler,           /* SVCall */
    Default_Handler,           /* DebugMonitor */
    0,
    Default_Handler,           /* PendSV */
    SysTick_IRQHandler,
    /* --- peripheral vectors (index = IRQ number) --- */
    [16 + 6]  = EXTI0_IRQHandler,
    [16 + 7]  = EXTI1_IRQHandler,
    [16 + 8]  = EXTI2_IRQHandler,
    [16 + 9]  = EXTI3_IRQHandler,
    [16 + 10] = EXTI4_IRQHandler,
    [16 + 11] = DMA1_Stream0_IRQHandler,
    [16 + 12] = DMA1_Stream1_IRQHandler,
    [16 + 13] = DMA1_Stream2_IRQHandler,
    [16 + 14] = DMA1_Stream3_IRQHandler,
    [16 + 15] = DMA1_Stream4_IRQHandler,
    [16 + 16] = DMA1_Stream5_IRQHandler,
    [16 + 17] = DMA1_Stream6_IRQHandler,
    [16 + 23] = EXTI5_9_IRQHandler,
    [16 + 24] = TIM15_IRQHandler,
    [16 + 25] = TIM1_UP_TIM10_IRQHandler,
    [16 + 26] = TIM17_IRQHandler,
    [16 + 28] = TIM2_IRQHandler,
    [16 + 29] = TIM3_IRQHandler,
    [16 + 30] = TIM4_IRQHandler,
    [16 + 40] = EXTI10_15_IRQHandler,
    [16 + 44] = TIM8_IRQHandler,
    [16 + 47] = DMA1_Stream7_IRQHandler,
    [16 + 54] = TIM6_IRQHandler,
    [16 + 55] = TIM7_IRQHandler,
    [16 + 56] = DMA2_Stream0_IRQHandler,
    [16 + 57] = DMA2_Stream1_IRQHandler,
    [16 + 58] = DMA2_Stream2_IRQHandler,
    [16 + 59] = DMA2_Stream3_IRQHandler,
    [16 + 60] = DMA2_Stream4_IRQHandler,
    [16 + 67] = OTG_FS_IRQHandler,
    [16 + 68] = DMA2_Stream5_IRQHandler,
    [16 + 69] = DMA2_Stream6_IRQHandler,
    [16 + 70] = DMA2_Stream7_IRQHandler,
};
