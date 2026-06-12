#ifndef __FRAM_H
#define __FRAM_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Exported defines ----------------------------------------------------------*/
#define FRAM_ADDRESS_BASE   (0xA0) // Base I2C address (7-bit, shifted left by 1 for R/W bit)
#define FRAM_DEVICE_ADDRESS (FRAM_ADDRESS_BASE | (0x00)) // A0, A1, A2 grounded
#define FRAM_PAGE_SIZE      64    // Example page size, confirm with datasheet

/* Exported types ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void FRAM_Init(void);
HAL_StatusTypeDef FRAM_WriteByte(uint16_t address, uint8_t data);
HAL_StatusTypeDef FRAM_ReadByte(uint16_t address, uint8_t *data);
HAL_StatusTypeDef FRAM_WritePage(uint16_t address, uint8_t *data, uint16_t size);
HAL_StatusTypeDef FRAM_ReadPage(uint16_t address, uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* __FRAM_H */
