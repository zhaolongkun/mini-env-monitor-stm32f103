#include "uart_json.h"

#include <stdio.h>
#include <string.h>

static void UART_FormatTenth(float value, char *buffer, uint32_t buffer_len)
{
  int32_t scaled;
  uint32_t magnitude;

  if ((buffer == 0) || (buffer_len == 0U))
  {
    return;
  }

  scaled = (int32_t)((value >= 0.0f) ? ((value * 10.0f) + 0.5f) : ((value * 10.0f) - 0.5f));

  if (scaled < 0)
  {
    magnitude = (uint32_t)(-scaled);
    (void)snprintf(buffer, buffer_len, "-%lu.%01lu",
                   (unsigned long)(magnitude / 10U),
                   (unsigned long)(magnitude % 10U));
  }
  else
  {
    magnitude = (uint32_t)scaled;
    (void)snprintf(buffer, buffer_len, "%lu.%01lu",
                   (unsigned long)(magnitude / 10U),
                   (unsigned long)(magnitude % 10U));
  }
}

void UART_SendEnvJson(UART_HandleTypeDef *huart,
                      float temp,
                      float hum,
                      uint32_t time_s,
                      uint8_t alarm)
{
  char temp_str[16];
  char hum_str[16];
  char json[96];
  int len;

  if (huart == 0)
  {
    return;
  }

  UART_FormatTenth(temp, temp_str, sizeof(temp_str));
  UART_FormatTenth(hum, hum_str, sizeof(hum_str));

  len = snprintf(json, sizeof(json),
                 "{\"temp\":%s,\"hum\":%s,\"time\":%lu,\"alarm\":%u}\r\n",
                 temp_str,
                 hum_str,
                 (unsigned long)time_s,
                 (unsigned int)(alarm != 0U));

  if ((len > 0) && ((uint32_t)len < sizeof(json)))
  {
    (void)HAL_UART_Transmit(huart, (uint8_t *)json, (uint16_t)len, 50U);
  }
}
