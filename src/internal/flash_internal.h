#pragma once
#include <stdint.h>

/*
 * STM32G431 embedded FLASH register block (RM0440 §3.7), base 0x40022000.
 * Offsets cross-checked against configs/stm32g431.svd.
 */
typedef struct {
    volatile uint32_t ACR;       // 0x00 access control
    volatile uint32_t PDKEYR;    // 0x04 power-down key
    volatile uint32_t KEYR;      // 0x08 unlock key
    volatile uint32_t OPTKEYR;   // 0x0C option-byte unlock key
    volatile uint32_t SR;        // 0x10 status
    volatile uint32_t CR;        // 0x14 control
    volatile uint32_t ECCR;      // 0x18 ECC
    volatile uint32_t _rsvd1C;   // 0x1C
    volatile uint32_t OPTR;      // 0x20 option
    volatile uint32_t PCROP1SR;  // 0x24
    volatile uint32_t PCROP1ER;  // 0x28
    volatile uint32_t WRP1AR;    // 0x2C write protection area A: STRT [6:0], END [22:16]
    volatile uint32_t WRP1BR;    // 0x30 write protection area B
} flash_t;

#define FLASH_REG ((flash_t*) 0x40022000)

/* Unlock keys (RM0440 §3.3.5). */
#define FLASH_KEY1 0x45670123u
#define FLASH_KEY2 0xCDEF89ABu
#define FLASH_OPTKEY1 0x08192A3Bu
#define FLASH_OPTKEY2 0x4C5D6E7Fu

/* CR bits. */
#define FLASH_CR_PG       BIT(0)
#define FLASH_CR_PER      BIT(1)
#define FLASH_CR_PNB_POS  3u
#define FLASH_CR_PNB_MSK  (0x7Fu << FLASH_CR_PNB_POS)   /* 7 bits: pages 0..63 */
#define FLASH_CR_MER1     BIT(2)
#define FLASH_CR_STRT     BIT(16)
#define FLASH_CR_OPTSTRT  BIT(17)
#define FLASH_CR_FSTPG    BIT(18)
#define FLASH_CR_OBL_LAUNCH BIT(27)
#define FLASH_CR_OPTLOCK  BIT(30)
#define FLASH_CR_LOCK     (1u << 31)

/* SR bits. */
#define FLASH_SR_EOP      BIT(0)
#define FLASH_SR_OPERR    BIT(1)
#define FLASH_SR_PROGERR  BIT(3)
#define FLASH_SR_WRPERR   BIT(4)
#define FLASH_SR_PGAERR   BIT(5)
#define FLASH_SR_SIZERR   BIT(6)
#define FLASH_SR_PGSERR   BIT(7)
#define FLASH_SR_MISERR   BIT(8)
#define FLASH_SR_FASTERR  BIT(9)
#define FLASH_SR_BSY      BIT(16)

#define FLASH_SR_ALL_ERR (FLASH_SR_OPERR | FLASH_SR_PROGERR | FLASH_SR_WRPERR | \
                          FLASH_SR_PGAERR | FLASH_SR_SIZERR | FLASH_SR_PGSERR | \
                          FLASH_SR_MISERR | FLASH_SR_FASTERR)
