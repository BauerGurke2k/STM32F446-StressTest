#include "oled.h"

#include <string.h>

#define OLED_CONTROL_COMMAND         0x00U
#define OLED_CONTROL_DATA            0x40U
#define OLED_DATA_CHUNK_SIZE         16U

static const uint8_t oled_font_5x7[95][5] =
{
  [0] = {0x00, 0x00, 0x00, 0x00, 0x00},
  [1] = {0x00, 0x00, 0x5F, 0x00, 0x00},
  [2] = {0x00, 0x07, 0x00, 0x07, 0x00},
  [3] = {0x14, 0x7F, 0x14, 0x7F, 0x14},
  [4] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},
  [5] = {0x23, 0x13, 0x08, 0x64, 0x62},
  [6] = {0x36, 0x49, 0x55, 0x22, 0x50},
  [7] = {0x00, 0x05, 0x03, 0x00, 0x00},
  [8] = {0x00, 0x1C, 0x22, 0x41, 0x00},
  [9] = {0x00, 0x41, 0x22, 0x1C, 0x00},
  [10] = {0x14, 0x08, 0x3E, 0x08, 0x14},
  [11] = {0x08, 0x08, 0x3E, 0x08, 0x08},
  [12] = {0x00, 0x50, 0x30, 0x00, 0x00},
  [13] = {0x08, 0x08, 0x08, 0x08, 0x08},
  [14] = {0x00, 0x60, 0x60, 0x00, 0x00},
  [15] = {0x20, 0x10, 0x08, 0x04, 0x02},
  ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
  ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
  ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
  ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
  ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
  ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
  ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
  ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
  ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
  ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},
  [26] = {0x00, 0x36, 0x36, 0x00, 0x00},
  [27] = {0x00, 0x56, 0x36, 0x00, 0x00},
  [28] = {0x08, 0x14, 0x22, 0x41, 0x00},
  [29] = {0x14, 0x14, 0x14, 0x14, 0x14},
  [30] = {0x00, 0x41, 0x22, 0x14, 0x08},
  [31] = {0x02, 0x01, 0x51, 0x09, 0x06},
  [32] = {0x32, 0x49, 0x79, 0x41, 0x3E},
  ['A' - 32] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
  ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
  ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
  ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
  ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
  ['F' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x01},
  ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x3A},
  ['H' - 32] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
  ['I' - 32] = {0x00, 0x41, 0x7F, 0x41, 0x00},
  ['J' - 32] = {0x20, 0x40, 0x41, 0x3F, 0x01},
  ['K' - 32] = {0x7F, 0x08, 0x14, 0x22, 0x41},
  ['L' - 32] = {0x7F, 0x40, 0x40, 0x40, 0x40},
  ['M' - 32] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
  ['N' - 32] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
  ['O' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
  ['P' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x06},
  ['Q' - 32] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
  ['R' - 32] = {0x7F, 0x09, 0x19, 0x29, 0x46},
  ['S' - 32] = {0x46, 0x49, 0x49, 0x49, 0x31},
  ['T' - 32] = {0x01, 0x01, 0x7F, 0x01, 0x01},
  ['U' - 32] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
  ['V' - 32] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
  ['W' - 32] = {0x3F, 0x40, 0x38, 0x40, 0x3F},
  ['X' - 32] = {0x63, 0x14, 0x08, 0x14, 0x63},
  ['Y' - 32] = {0x07, 0x08, 0x70, 0x08, 0x07},
  ['Z' - 32] = {0x61, 0x51, 0x49, 0x45, 0x43},
  [59] = {0x00, 0x7F, 0x41, 0x41, 0x00},
  [60] = {0x02, 0x04, 0x08, 0x10, 0x20},
  [61] = {0x00, 0x41, 0x41, 0x7F, 0x00},
  [62] = {0x04, 0x02, 0x01, 0x02, 0x04},
  [63] = {0x40, 0x40, 0x40, 0x40, 0x40},
  [64] = {0x00, 0x01, 0x02, 0x04, 0x00},
  ['a' - 32] = {0x20, 0x54, 0x54, 0x54, 0x78},
  ['b' - 32] = {0x7F, 0x48, 0x44, 0x44, 0x38},
  ['c' - 32] = {0x38, 0x44, 0x44, 0x44, 0x20},
  ['d' - 32] = {0x38, 0x44, 0x44, 0x48, 0x7F},
  ['e' - 32] = {0x38, 0x54, 0x54, 0x54, 0x18},
  ['f' - 32] = {0x08, 0x7E, 0x09, 0x01, 0x02},
  ['g' - 32] = {0x08, 0x14, 0x54, 0x54, 0x3C},
  ['h' - 32] = {0x7F, 0x08, 0x04, 0x04, 0x78},
  ['i' - 32] = {0x00, 0x44, 0x7D, 0x40, 0x00},
  ['j' - 32] = {0x20, 0x40, 0x44, 0x3D, 0x00},
  ['k' - 32] = {0x7F, 0x10, 0x28, 0x44, 0x00},
  ['l' - 32] = {0x00, 0x41, 0x7F, 0x40, 0x00},
  ['m' - 32] = {0x7C, 0x04, 0x18, 0x04, 0x78},
  ['n' - 32] = {0x7C, 0x08, 0x04, 0x04, 0x78},
  ['o' - 32] = {0x38, 0x44, 0x44, 0x44, 0x38},
  ['p' - 32] = {0x7C, 0x14, 0x14, 0x14, 0x08},
  ['q' - 32] = {0x08, 0x14, 0x14, 0x18, 0x7C},
  ['r' - 32] = {0x7C, 0x08, 0x04, 0x04, 0x08},
  ['s' - 32] = {0x48, 0x54, 0x54, 0x54, 0x20},
  ['t' - 32] = {0x04, 0x3F, 0x44, 0x40, 0x20},
  ['u' - 32] = {0x3C, 0x40, 0x40, 0x20, 0x7C},
  ['v' - 32] = {0x1C, 0x20, 0x40, 0x20, 0x1C},
  ['w' - 32] = {0x3C, 0x40, 0x30, 0x40, 0x3C},
  ['x' - 32] = {0x44, 0x28, 0x10, 0x28, 0x44},
  ['y' - 32] = {0x0C, 0x50, 0x50, 0x50, 0x3C},
  ['z' - 32] = {0x44, 0x64, 0x54, 0x4C, 0x44},
  [91] = {0x00, 0x08, 0x36, 0x41, 0x00},
  [92] = {0x00, 0x00, 0x7F, 0x00, 0x00},
  [93] = {0x00, 0x41, 0x36, 0x08, 0x00},
  [94] = {0x08, 0x08, 0x2A, 0x1C, 0x08}
};

static HAL_StatusTypeDef OLED_WriteCommand(OLED_HandleTypeDef *oled, uint8_t command)
{
  uint8_t data[2] = {OLED_CONTROL_COMMAND, command};
  return HAL_I2C_Master_Transmit(oled->hi2c, oled->address, data, sizeof(data), oled->timeout_ms);
}

static HAL_StatusTypeDef OLED_WriteData(OLED_HandleTypeDef *oled, const uint8_t *data, uint16_t size)
{
  uint8_t tx_buffer[OLED_DATA_CHUNK_SIZE + 1U];

  if ((data == NULL) || (size == 0U))
  {
    return HAL_OK;
  }

  tx_buffer[0] = OLED_CONTROL_DATA;
  memcpy(&tx_buffer[1], data, size);

  return HAL_I2C_Master_Transmit(oled->hi2c, oled->address, tx_buffer, size + 1U, oled->timeout_ms);
}

static HAL_StatusTypeDef OLED_WriteCommandSequence(OLED_HandleTypeDef *oled, const uint8_t *commands, uint16_t count)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint16_t index;

  for (index = 0U; index < count; index++)
  {
    status = OLED_WriteCommand(oled, commands[index]);
    if (status != HAL_OK)
    {
      return status;
    }
  }

  return status;
}

HAL_StatusTypeDef OLED_Init(OLED_HandleTypeDef *oled, I2C_HandleTypeDef *hi2c, uint16_t address)
{
  static const uint8_t init_sequence[] =
  {
    0xAE,
    0xD5, 0x80,
    0xA8, 0x3F,
    0xD3, 0x00,
    0x40,
    0xAD, 0x8B,
    0xA1,
    0xC8,
    0xDA, 0x12,
    0x81, 0x7F,
    0xD9, 0x22,
    0xDB, 0x20,
    0xA4,
    0xA6,
    0xAF
  };
  HAL_StatusTypeDef status;

  if ((oled == NULL) || (hi2c == NULL))
  {
    return HAL_ERROR;
  }

  memset(oled, 0, sizeof(*oled));
  oled->hi2c = hi2c;
  oled->address = address;
  oled->width = OLED_WIDTH;
  oled->height = OLED_HEIGHT;
  oled->column_offset = OLED_SH1106_COLUMN_OFFSET;
  oled->timeout_ms = OLED_DEFAULT_TIMEOUT_MS;

  HAL_Delay(100);

  status = HAL_I2C_IsDeviceReady(oled->hi2c, oled->address, 3U, oled->timeout_ms);
  if (status != HAL_OK)
  {
    return status;
  }

  status = OLED_WriteCommandSequence(oled, init_sequence, sizeof(init_sequence));
  if (status != HAL_OK)
  {
    return status;
  }

  OLED_Clear(oled);
  return OLED_UpdateScreen(oled);
}

HAL_StatusTypeDef OLED_UpdateScreen(OLED_HandleTypeDef *oled)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t page;
  uint16_t chunk_offset;
  uint8_t column_low;
  uint8_t column_high;

  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  column_low = oled->column_offset & 0x0FU;
  column_high = 0x10U | ((oled->column_offset >> 4) & 0x0FU);

  for (page = 0U; page < OLED_PAGES; page++)
  {
    status = OLED_WriteCommand(oled, 0xB0U + page);
    if (status != HAL_OK)
    {
      return status;
    }

    status = OLED_WriteCommand(oled, column_low);
    if (status != HAL_OK)
    {
      return status;
    }

    status = OLED_WriteCommand(oled, column_high);
    if (status != HAL_OK)
    {
      return status;
    }

    for (chunk_offset = 0U; chunk_offset < oled->width; chunk_offset += OLED_DATA_CHUNK_SIZE)
    {
      uint16_t remaining = (uint16_t)oled->width - chunk_offset;
      uint16_t chunk_size = (remaining > OLED_DATA_CHUNK_SIZE) ? OLED_DATA_CHUNK_SIZE : remaining;

      status = OLED_WriteData(oled, &oled->buffer[(page * oled->width) + chunk_offset], (uint16_t)chunk_size);
      if (status != HAL_OK)
      {
        return status;
      }
    }
  }

  return status;
}

HAL_StatusTypeDef OLED_SetContrast(OLED_HandleTypeDef *oled, uint8_t contrast)
{
  uint8_t commands[2] = {0x81, contrast};

  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  return OLED_WriteCommandSequence(oled, commands, sizeof(commands));
}

HAL_StatusTypeDef OLED_SetDisplayOn(OLED_HandleTypeDef *oled, bool is_on)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  return OLED_WriteCommand(oled, is_on ? 0xAFU : 0xAEU);
}

HAL_StatusTypeDef OLED_InvertDisplay(OLED_HandleTypeDef *oled, bool invert)
{
  if (oled == NULL)
  {
    return HAL_ERROR;
  }

  return OLED_WriteCommand(oled, invert ? 0xA7U : 0xA6U);
}

void OLED_Clear(OLED_HandleTypeDef *oled)
{
  OLED_Fill(oled, OLED_COLOR_BLACK);
  OLED_SetCursor(oled, 0U, 0U);
}

void OLED_Fill(OLED_HandleTypeDef *oled, OLED_Color color)
{
  if (oled == NULL)
  {
    return;
  }

  memset(oled->buffer, (color == OLED_COLOR_WHITE) ? 0xFF : 0x00, sizeof(oled->buffer));
}

void OLED_SetCursor(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y)
{
  if (oled == NULL)
  {
    return;
  }

  oled->cursor_x = x;
  oled->cursor_y = y;
}

void OLED_DrawPixel(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, OLED_Color color)
{
  uint16_t index;

  if ((oled == NULL) || (x >= oled->width) || (y >= oled->height))
  {
    return;
  }

  index = (uint16_t)x + ((uint16_t)(y / 8U) * oled->width);
  if (color == OLED_COLOR_WHITE)
  {
    oled->buffer[index] |= (uint8_t)(1U << (y % 8U));
  }
  else
  {
    oled->buffer[index] &= (uint8_t)~(1U << (y % 8U));
  }
}

void OLED_DrawLine(OLED_HandleTypeDef *oled, int x0, int y0, int x1, int y1, OLED_Color color)
{
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = (y0 < y1) ? -(y1 - y0) : -(y0 - y1);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;
  int e2;

  while (1)
  {
    if ((x0 >= 0) && (y0 >= 0))
    {
      OLED_DrawPixel(oled, (uint8_t)x0, (uint8_t)y0, color);
    }

    if ((x0 == x1) && (y0 == y1))
    {
      break;
    }

    e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void OLED_DrawRect(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, OLED_Color color)
{
  if ((oled == NULL) || (width == 0U) || (height == 0U))
  {
    return;
  }

  OLED_DrawLine(oled, x, y, (int)x + width - 1, y, color);
  OLED_DrawLine(oled, x, y, x, (int)y + height - 1, color);
  OLED_DrawLine(oled, (int)x + width - 1, y, (int)x + width - 1, (int)y + height - 1, color);
  OLED_DrawLine(oled, x, (int)y + height - 1, (int)x + width - 1, (int)y + height - 1, color);
}

void OLED_FillRect(OLED_HandleTypeDef *oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height, OLED_Color color)
{
  uint16_t x_end;
  uint16_t y_end;
  uint16_t x_pos;
  uint16_t y_pos;

  if ((oled == NULL) || (width == 0U) || (height == 0U))
  {
    return;
  }

  x_end = (uint16_t)x + width;
  y_end = (uint16_t)y + height;

  for (x_pos = x; (x_pos < x_end) && (x_pos < oled->width); x_pos++)
  {
    for (y_pos = y; (y_pos < y_end) && (y_pos < oled->height); y_pos++)
    {
      OLED_DrawPixel(oled, (uint8_t)x_pos, (uint8_t)y_pos, color);
    }
  }
}

bool OLED_WriteChar(OLED_HandleTypeDef *oled, char ch)
{
  const uint8_t *glyph;
  uint8_t column;
  uint8_t row;

  if ((oled == NULL) || (ch < 32) || (ch > 126))
  {
    return false;
  }

  if ((oled->cursor_x + 6U > oled->width) || (oled->cursor_y + 8U > oled->height))
  {
    return false;
  }

  glyph = oled_font_5x7[(uint8_t)ch - 32U];

  for (column = 0U; column < 5U; column++)
  {
    uint8_t line = glyph[column];
    for (row = 0U; row < 7U; row++)
    {
      OLED_DrawPixel(
        oled,
        oled->cursor_x + column,
        oled->cursor_y + row,
        ((line >> row) & 0x01U) ? OLED_COLOR_WHITE : OLED_COLOR_BLACK
      );
    }
  }

  for (row = 0U; row < 7U; row++)
  {
    OLED_DrawPixel(oled, oled->cursor_x + 5U, oled->cursor_y + row, OLED_COLOR_BLACK);
  }

  oled->cursor_x += 6U;
  return true;
}

void OLED_WriteString(OLED_HandleTypeDef *oled, const char *text)
{
  if ((oled == NULL) || (text == NULL))
  {
    return;
  }

  while (*text != '\0')
  {
    if (*text == '\n')
    {
      oled->cursor_x = 0U;
      oled->cursor_y = (uint8_t)(oled->cursor_y + 8U);
    }
    else if (!OLED_WriteChar(oled, *text))
    {
      break;
    }

    text++;
  }
}
