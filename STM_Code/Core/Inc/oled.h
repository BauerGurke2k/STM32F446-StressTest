#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdbool.h>

#define OLED_WIDTH                   128U
#define OLED_HEIGHT                  64U
#define OLED_PAGES                   (OLED_HEIGHT / 8U)
#define OLED_BUFFER_SIZE             (OLED_WIDTH * OLED_HEIGHT / 8U)
#define OLED_DEFAULT_TIMEOUT_MS      100U
#define OLED_SH1106_COLUMN_OFFSET    2U

#define OLED_I2C_ADDR_0X3C           (0x3CU << 1)
#define OLED_I2C_ADDR_0X3D           (0x3DU << 1)

typedef enum
{
  OLED_COLOR_BLACK = 0U,
  OLED_COLOR_WHITE = 1U
} OLED_Color;

typedef struct
{
  I2C_HandleTypeDef *hi2c;
  uint16_t address;
  uint8_t width;
  uint8_t height;
  uint8_t column_offset;
  uint32_t timeout_ms;
  uint8_t cursor_x;
  uint8_t cursor_y;
  uint8_t buffer[OLED_BUFFER_SIZE];
} OLED_HandleTypeDef;

HAL_StatusTypeDef OLED_Init(OLED_HandleTypeDef *oled, I2C_HandleTypeDef *hi2c, uint16_t address);
HAL_StatusTypeDef OLED_UpdateScreen(OLED_HandleTypeDef *oled);
HAL_StatusTypeDef OLED_SetContrast(OLED_HandleTypeDef *oled, uint8_t contrast);
HAL_StatusTypeDef OLED_SetDisplayOn(OLED_HandleTypeDef *oled, bool is_on);
HAL_StatusTypeDef OLED_InvertDisplay(OLED_HandleTypeDef *oled, bool invert);

void OLED_Clear(OLED_HandleTypeDef *oled);
void OLED_Fill(OLED_HandleTypeDef *oled, OLED_Color color);
void OLED_SetCursor(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y);
void OLED_DrawPixel(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, OLED_Color color);
void OLED_DrawLine(OLED_HandleTypeDef *oled, int x0, int y0, int x1, int y1, OLED_Color color);
void OLED_DrawRect(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, OLED_Color color);
void OLED_FillRect(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, OLED_Color color);
bool OLED_WriteChar(OLED_HandleTypeDef *oled, char ch);
void OLED_WriteString(OLED_HandleTypeDef *oled, const char *text);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
