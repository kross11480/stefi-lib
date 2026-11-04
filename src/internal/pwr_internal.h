#pragma once
typedef struct
{
  volatile uint32_t CR1;   /*!< PWR power control register 1,        Address offset: 0x00 */
  volatile uint32_t CR2;      /*!< PWR power control register 2,        Address offset: 0x04 */
  volatile uint32_t CR3;      /*!< PWR power control register 3,        Address offset: 0x08 */
  volatile uint32_t CR4;      /*!< PWR power control register 4,        Address offset: 0x0C */
  volatile uint32_t SR1;      /*!< PWR power status register 1,         Address offset: 0x10 */
  volatile uint32_t SR2;      /*!< PWR power status register 2,         Address offset: 0x14 */
  volatile uint32_t SCR;      /*!< PWR status clear register,           Address offset: 0x18 */
  uint32_t RESERVED[25];      /*!< PUCRx/PDCRx (unused here),           Address offset: 0x1C */
  volatile uint32_t CR5;      /*!< PWR power control register 5 (G4),   Address offset: 0x80 */

} PWR_typeDef;

#define PWR ((PWR_typeDef *) 0x40007000)

typedef struct
{
  volatile uint32_t ACR;              /*!< FLASH access control register,            Address offset: 0x00 */
} FLASH_typeDef;

#define FLASH ((FLASH_typeDef *) 0x40022000)


