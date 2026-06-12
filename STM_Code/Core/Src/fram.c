#include "fram.h"
#include "i2c.h" // For hi2c3

// External I2C handle from i2c.c
extern I2C_HandleTypeDef hi2c3;

void FRAM_Init(void)
{
  // No specific initialization needed for FRAM beyond I2C init
  // I2C is initialized in MX_I2C3_Init() called from main.
}

HAL_StatusTypeDef FRAM_WriteByte(uint16_t address, uint8_t data)
{
  uint8_t tx_buffer[3];
  tx_buffer[0] = (uint8_t)((address >> 8) & 0xFF); // High byte of address
  tx_buffer[1] = (uint8_t)(address & 0xFF);         // Low byte of address
  tx_buffer[2] = data;

  // The FRAM_DEVICE_ADDRESS is already left-shifted by 1 in fram.h, so use it directly.
  return HAL_I2C_Master_Transmit(&hi2c3, FRAM_DEVICE_ADDRESS, tx_buffer, 3, HAL_MAX_DELAY);
}

HAL_StatusTypeDef FRAM_ReadByte(uint16_t address, uint8_t *data)
{
  HAL_StatusTypeDef status;
  uint8_t tx_buffer[2];
  tx_buffer[0] = (uint8_t)((address >> 8) & 0xFF); // High byte of address
  tx_buffer[1] = (uint8_t)(address & 0xFF);         // Low byte of address

  // Send the address
  status = HAL_I2C_Master_Transmit(&hi2c3, FRAM_DEVICE_ADDRESS, tx_buffer, 2, HAL_MAX_DELAY);
  if (status != HAL_OK)
  {
    return status;
  }

  // Receive the data
  return HAL_I2C_Master_Receive(&hi2c3, FRAM_DEVICE_ADDRESS, data, 1, HAL_MAX_DELAY);
}

HAL_StatusTypeDef FRAM_WritePage(uint16_t address, uint8_t *data, uint16_t size)
{
  if (size == 0 || size > FRAM_PAGE_SIZE) // Assuming a write buffer size limit
  {
    return HAL_ERROR;
  }

  uint8_t tx_buffer[2 + FRAM_PAGE_SIZE]; // 2 for address, up to FRAM_PAGE_SIZE for data
  tx_buffer[0] = (uint8_t)((address >> 8) & 0xFF);
  tx_buffer[1] = (uint8_t)(address & 0xFF);

  for (uint16_t i = 0; i < size; i++)
  {
    tx_buffer[2 + i] = data[i];
  }

  return HAL_I2C_Master_Transmit(&hi2c3, FRAM_DEVICE_ADDRESS, tx_buffer, 2 + size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef FRAM_ReadPage(uint16_t address, uint8_t *data, uint16_t size)
{
  if (size == 0) return HAL_ERROR;

  HAL_StatusTypeDef status;
  uint8_t tx_buffer[2];
  tx_buffer[0] = (uint8_t)((address >> 8) & 0xFF); // High byte of address
  tx_buffer[1] = (uint8_t)(address & 0xFF);         // Low byte of address

  // Send the address
  status = HAL_I2C_Master_Transmit(&hi2c3, FRAM_DEVICE_ADDRESS, tx_buffer, 2, HAL_MAX_DELAY);
  if (status != HAL_OK)
  {
    return status;
  }

  // Receive the data
  return HAL_I2C_Master_Receive(&hi2c3, FRAM_DEVICE_ADDRESS, data, size, HAL_MAX_DELAY);
}
