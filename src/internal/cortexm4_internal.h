#include <stdint.h>

typedef struct
{
    volatile uint32_t CPUID;                  /*!< Offset: 0x000 (R/ )  CPUID Base Register */
    volatile uint32_t ICSR;                   /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
    volatile uint32_t VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
    volatile uint32_t AIRCR;                  /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
    volatile uint32_t SCR;                    /*!< Offset: 0x010 (R/W)  System Control Register */
    volatile uint32_t CCR;                    /*!< Offset: 0x014 (R/W)  Configuration Control Register */
    volatile uint8_t  SHP[12U];               /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
    volatile uint32_t SHCSR;                  /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
    volatile uint32_t CFSR;                   /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register */
    volatile uint32_t HFSR;                   /*!< Offset: 0x02C (R/W)  HardFault Status Register */
    volatile uint32_t DFSR;                   /*!< Offset: 0x030 (R/W)  Debug Fault Status Register */
    volatile uint32_t MMFAR;                  /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register */
    volatile uint32_t BFAR;                   /*!< Offset: 0x038 (R/W)  BusFault Address Register */
    volatile uint32_t AFSR;                   /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
    volatile  uint32_t PFR[2U];                /*!< Offset: 0x040 (R/ )  Processor Feature Register */
    volatile  uint32_t DFR;                    /*!< Offset: 0x048 (R/ )  Debug Feature Register */
    volatile  uint32_t ADR;                    /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register */
    volatile  uint32_t MMFR[4U];               /*!< Offset: 0x050 (R/ )  Memory Model Feature Register */
    volatile  uint32_t ISAR[5U];               /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
    uint32_t RESERVED0[5U];
    volatile uint32_t CPACR;                  /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
} SCB_typeDef;

#define SCB ((SCB_typeDef *) 0xE000ED00)

/** Memory protection unit (ARMv7-M) */
typedef struct
{
    volatile uint32_t TYPE;                   /*!< Offset: 0x000 (R/ )  MPU Type Register */
    volatile uint32_t CTRL;                   /*!< Offset: 0x004 (R/W)  MPU Control Register */
    volatile uint32_t RNR;                    /*!< Offset: 0x008 (R/W)  MPU Region Number Register */
    volatile uint32_t RBAR;                   /*!< Offset: 0x00C (R/W)  MPU Region Base Address Register */
    volatile uint32_t RASR;                   /*!< Offset: 0x010 (R/W)  MPU Region Attribute and Size Register */
} MPU_typeDef;

#define MPU ((MPU_typeDef *) 0xE000ED90)
