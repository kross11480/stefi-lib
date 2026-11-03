//
extern unsigned int _estack;  //Top of Stack
extern unsigned int _sdata;  //start .data in SRAM
extern unsigned int _edata; //end .data in SRAM
extern unsigned int _sidata; //start .data in FLASH
extern unsigned int _sbss; // start .bss (allocated in RAM)
extern unsigned int _ebss; // end .bss

extern void SysTick_IRQHandler(void);
extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);
extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);
extern void EXTI5_9_IRQHandler(void);
extern void EXTI10_15_IRQHandler(void);

extern void TIM2_IRQHandler(void);
extern void TIM3_IRQHandler(void);
extern void TIM4_IRQHandler(void);
extern void TIM6_IRQHandler(void);
extern void TIM7_IRQHandler(void);
extern void TIM8_IRQHandler(void);
extern void TIM15_IRQHandler(void);
extern void TIM16_IRQHandler(void);
extern void TIM17_IRQHandler(void);

extern void DMA1_Channel1_IRQHandler(void);
extern void DMA1_Channel2_IRQHandler(void);
extern void DMA1_Channel3_IRQHandler(void);
extern void DMA1_Channel4_IRQHandler(void);
extern void DMA1_Channel5_IRQHandler(void);
extern void DMA1_Channel6_IRQHandler(void);
extern void DMA1_Channel7_IRQHandler(void);

/* Weak: only linked in when the app uses libstefi/usb_cdc.h (L4 OTG_FS).
 * Unresolved, the slot stays 0 — the IRQ is never enabled then. */
extern void OTG_FS_IRQHandler(void) __attribute__((weak));

/* Optional drivers: weak references, so their code is only linked into apps
 * that use them (unresolved = 0, those IRQs are then never enabled). */
extern void WWDG_IRQHandler(void) __attribute__((weak));        /* wwdg.c   */
extern void RTC_WKUP_IRQHandler(void) __attribute__((weak));    /* rtc.c    */
extern void TIM1_CC_IRQHandler(void) __attribute__((weak));     /* timer.c  */
extern void TIM8_CC_IRQHandler(void) __attribute__((weak));
extern void USART1_IRQHandler(void) __attribute__((weak));      /* usart.c  */
extern void USART2_IRQHandler(void) __attribute__((weak));

/* Faults: park in a loop (debugger: halt and inspect; a running watchdog
 * resets the MCU — the WWDG/IWDG examples rely on that). */
void Fault_Handler(void)
{
    for (;;);
}

extern void main();

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
__attribute__ ((section (".isr_vector")))
void (* const paIsrFunc[16+102])(void) =
{/* --- core vectors --- */
    (void *)&_estack,       /*  00 (0x00000000)  initial stack pointer */
    (void *)&Reset_Handler, /*  01 (0x00000004)  initial program counter (leads to first command to execute) */
    Fault_Handler,          /*  NMI */
    Fault_Handler,          /*  HardFault */
    Fault_Handler,          /*  MemManage */
    Fault_Handler,          /*  BusFault */
    Fault_Handler,          /*  UsageFault */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    SysTick_IRQHandler,
    /* --- non-core vectors --- */
	WWDG_IRQHandler,        /*   0 (0x00000040)  window watchdog interrupt */
	0,            /*   1 (0x00000044)  PVD through EXTI line 16 interrupt */
	0,            /*   2 (0x00000048)  RTC/TAMP/CSS on LSE through EXTI line 19 interrupt */
	RTC_WKUP_IRQHandler,    /*   3 (0x0000004C)  RTC Wakeup timer through EXTI line 20 interrupt */
	0,            /*   4 (0x00000050)  Flash global interrupt */
	0,            /*   5 (0x00000054)  RCC global interrupt */
	EXTI0_IRQHandler,       /*   6 (0x00000058)  EXTI Line 0 interrupt */
	EXTI1_IRQHandler,            /*   7 (0x0000005C)  EXTI Line 1 interrupt */
	EXTI2_IRQHandler,            /*   8 (0x00000060)  EXTI Line 2 interrupt */
	EXTI3_IRQHandler,            /*   9 (0x00000064)  EXTI Line 3 interrupt */
	EXTI4_IRQHandler,            /*  10 (0x00000068)  EXTI Line 4 interrupt */
	DMA1_Channel1_IRQHandler,   /*  11 (0x0000006C)  DMA1 channel 1 interrupt */
	DMA1_Channel2_IRQHandler,   /*  12 (0x00000070)  DMA1 channel 2 interrupt */
	DMA1_Channel3_IRQHandler,   /*  13 (0x00000074)  DMA1 channel 3 interrupt */
	DMA1_Channel4_IRQHandler,   /*  14 (0x00000078)  DMA1 channel 4 interrupt */
	DMA1_Channel5_IRQHandler,   /*  15 (0x0000007C)  DMA1 channel 5 interrupt */
	DMA1_Channel6_IRQHandler,   /*  16 (0x00000080)  DMA1 channel 6 interrupt */
	DMA1_Channel7_IRQHandler,   /*  17 (0x00000084)  DMA1 channel 7 interrupt */
	0,            /*  18 (0x00000088)  ADC1 + ADC2 global interrupt */
	0,            /*  19 (0x0000008C)  USB high priority interrupts */
	0,            /*  20 (0x00000090)  USB low priority interrupts */
	0,            /*  21 (0x00000094)  FDCAN1 interrupt 0 */
	0,            /*  22 (0x00000098)  FDCAN1 interrupt 1 */
	EXTI5_9_IRQHandler,            /*  23 (0x0000009C)  EXTI line [9:5] interrupts */
	TIM15_IRQHandler,            /*  24 (0x000000A0)  TIM1 break + TIM15 global interrupts */
	TIM16_IRQHandler,            /*  25 (0x000000A4)  TIM1 update + TIM16 global interrupts */
	TIM17_IRQHandler,            /*  26 (0x000000A8)  TIM1 trigger, commutation, direction change, index  + TIM17 interrupts */
	TIM1_CC_IRQHandler,     /*  27 (0x000000AC)  TIM1 capture compare interrupt */
	TIM2_IRQHandler,            /*  28 (0x000000B0)  TIM2 global interrupt */
	TIM3_IRQHandler,            /*  29 (0x000000B0)  TIM3 global interrupt */
	TIM4_IRQHandler,            /*  30 (0x000000B0)  TIM4 global interrupt */
	0, 0, 0, 0, 0, 0,           /*  31..36 I2C1/2, SPI1/2 */
	USART1_IRQHandler,          /*  37 */
	USART2_IRQHandler,          /*  38 */
	0,                          /*  39 USART3 */
	EXTI10_15_IRQHandler,       /*  40 */
	0, 0, 0,                    /*  41..43 */
	TIM8_IRQHandler,            /*  44 TIM8 update */
	0,                          /*  45 */
	TIM8_CC_IRQHandler,         /*  46 TIM8 capture compare */
	0, 0, 0, 0, 0, 0, 0,        /*  47..53 */
	TIM6_IRQHandler,            /*  54 (0x00000118)  TIM6 + DAC1/3 underrun global interrupts */
	TIM7_IRQHandler,            /*  55 (0x0000011C)  TIM7 + DAC2/4 underrun global interrupts */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /*  56..66 */
	OTG_FS_IRQHandler,          /*  67 (0x0000014C)  L4: USB OTG FS (G4: unused here) */
};