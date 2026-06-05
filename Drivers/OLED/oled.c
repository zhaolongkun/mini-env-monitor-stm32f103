#include "oled.h"

#include "i2c.h"
#include "oled_font.h"
#include "stm32f1xx_hal.h"

#include <string.h>

#define OLED_PAGES (OLED_HEIGHT / 8U)
#define OLED_I2C_TIMEOUT_MS 10U
#define OLED_CONTROL_CMD 0x00U
#define OLED_CONTROL_DATA 0x40U

static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t oled_ready;

static HAL_StatusTypeDef OLED_WriteCommand(uint8_t command);
static HAL_StatusTypeDef OLED_WriteData(const uint8_t *data, uint16_t size);

void OLED_Init(void)
{
  static const uint8_t init_commands[] = {
    0xAE,       /* display off */
    0x20, 0x00, /* horizontal addressing */
    0xB0,
    0xC8,
    0x00,
    0x10,
    0x40,
    0x81, 0x7F,
    0xA1,
    0xA6,
    0xA8, 0x3F,
    0xA4,
    0xD3, 0x00,
    0xD5, 0x80,
    0xD9, 0xF1,
    0xDA, 0x12,
    0xDB, 0x40,
    0x8D, 0x14,
    0xAF        /* display on */
  };
  uint32_t i;

  oled_ready = 1U;
  HAL_Delay(100U);

  for (i = 0U; i < (sizeof(init_commands) / sizeof(init_commands[0])); i++)
  {
    if (OLED_WriteCommand(init_commands[i]) != HAL_OK)
    {
      oled_ready = 0U;
      return;
    }
  }

  OLED_Clear();
  OLED_Update();
}

void OLED_Clear(void)
{
  (void)memset(oled_buffer, 0, sizeof(oled_buffer));
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str)
{
  uint16_t cursor_x = x;
  uint8_t page = y;

  if ((str == 0) || (page >= OLED_PAGES))
  {
    return;
  }

  while ((*str != '\0') && ((cursor_x + OLED_FONT_WIDTH) < OLED_WIDTH))
  {
    uint8_t col;
    uint16_t offset = ((uint16_t)page * OLED_WIDTH) + cursor_x;

    for (col = 0U; col < OLED_FONT_WIDTH; col++)
    {
      oled_buffer[offset + col] = OLED_Font_GetColumn(*str, col);
    }

    if ((cursor_x + OLED_FONT_WIDTH) < OLED_WIDTH)
    {
      oled_buffer[offset + OLED_FONT_WIDTH] = 0x00U;
    }

    cursor_x = (uint16_t)(cursor_x + OLED_FONT_WIDTH + OLED_FONT_SPACING);
    str++;
  }
}

void OLED_Update(void)
{
  uint8_t page;

  if (oled_ready == 0U)
  {
    return;
  }

  for (page = 0U; page < OLED_PAGES; page++)
  {
    if ((OLED_WriteCommand((uint8_t)(0xB0U + page)) != HAL_OK) ||
        (OLED_WriteCommand(0x00U) != HAL_OK) ||
        (OLED_WriteCommand(0x10U) != HAL_OK) ||
        (OLED_WriteData(&oled_buffer[(uint16_t)page * OLED_WIDTH], OLED_WIDTH) != HAL_OK))
    {
      oled_ready = 0U;
      return;
    }
  }
}

static HAL_StatusTypeDef OLED_WriteCommand(uint8_t command)
{
  uint8_t tx[2];

  tx[0] = OLED_CONTROL_CMD;
  tx[1] = command;

  return HAL_I2C_Master_Transmit(&hi2c1,
                                 (uint16_t)(OLED_I2C_ADDRESS << 1),
                                 tx,
                                 (uint16_t)sizeof(tx),
                                 OLED_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef OLED_WriteData(const uint8_t *data, uint16_t size)
{
  uint8_t tx[17];
  uint16_t sent = 0U;

  if (data == 0)
  {
    return HAL_ERROR;
  }

  tx[0] = OLED_CONTROL_DATA;

  while (sent < size)
  {
    uint16_t chunk = (uint16_t)(size - sent);
    if (chunk > 16U)
    {
      chunk = 16U;
    }

    (void)memcpy(&tx[1], &data[sent], chunk);

    if (HAL_I2C_Master_Transmit(&hi2c1,
                                (uint16_t)(OLED_I2C_ADDRESS << 1),
                                tx,
                                (uint16_t)(chunk + 1U),
                                OLED_I2C_TIMEOUT_MS) != HAL_OK)
    {
      return HAL_ERROR;
    }

    sent = (uint16_t)(sent + chunk);
  }

  return HAL_OK;
}
