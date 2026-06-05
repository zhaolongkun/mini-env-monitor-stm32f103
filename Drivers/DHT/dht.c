#include "dht.h"

#include "main.h"
#include "tim.h"

#define DHT_RELEASE_US 30U
#define DHT_TIMEOUT_US 120U
#define DHT_BIT_ONE_THRESHOLD_US 50U

static void DHT_SetPinOutput(void);
static void DHT_SetPinInput(void);
static void DHT_DelayUs(uint16_t us);
static DHT_StatusTypeDef DHT_WaitForPin(GPIO_PinState state, uint16_t timeout_us);

void DHT_Init(void)
{
  DHT_SetPinOutput();
  HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_SET);
}

DHT_StatusTypeDef DHT_Read(float *temp, float *hum)
{
  uint8_t data[5] = {0};
  uint8_t i;
  uint8_t bit;

  if ((temp == NULL) || (hum == NULL))
  {
    return DHT_ERROR_PARAM;
  }

  /*
   * Start signal:
   * DHT11 needs the host to pull DATA low for at least 18 ms.
   * DHT22 usually needs only about 1 ms, but DHT11 is the default target.
   */
  DHT_SetPinOutput();
  HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_RESET);
#if (DHT_SENSOR_TYPE == DHT_TYPE_DHT22)
  HAL_Delay(1);
#else
  HAL_Delay(18);
#endif
  HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_SET);
  DHT_DelayUs(DHT_RELEASE_US);
  DHT_SetPinInput();

  /* Sensor response: about 80 us low, then about 80 us high. */
  if (DHT_WaitForPin(GPIO_PIN_RESET, DHT_TIMEOUT_US) != DHT_OK)
  {
    return DHT_ERROR_TIMEOUT;
  }
  if (DHT_WaitForPin(GPIO_PIN_SET, DHT_TIMEOUT_US) != DHT_OK)
  {
    return DHT_ERROR_TIMEOUT;
  }
  if (DHT_WaitForPin(GPIO_PIN_RESET, DHT_TIMEOUT_US) != DHT_OK)
  {
    return DHT_ERROR_TIMEOUT;
  }

  /*
   * Each bit starts with about 50 us low. The following high pulse is
   * short for 0 and long for 1, so the high pulse width is measured.
   */
  for (i = 0; i < 5U; i++)
  {
    for (bit = 0; bit < 8U; bit++)
    {
      uint16_t start;
      uint16_t high_time;

      if (DHT_WaitForPin(GPIO_PIN_SET, DHT_TIMEOUT_US) != DHT_OK)
      {
        return DHT_ERROR_TIMEOUT;
      }

      start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);

      if (DHT_WaitForPin(GPIO_PIN_RESET, DHT_TIMEOUT_US) != DHT_OK)
      {
        return DHT_ERROR_TIMEOUT;
      }

      high_time = (uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) - start);
      data[i] <<= 1;
      if (high_time > DHT_BIT_ONE_THRESHOLD_US)
      {
        data[i] |= 0x01U;
      }
    }
  }

  if (((uint8_t)(data[0] + data[1] + data[2] + data[3])) != data[4])
  {
    return DHT_ERROR_CHECKSUM;
  }

#if (DHT_SENSOR_TYPE == DHT_TYPE_DHT22)
  {
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)(data[2] & 0x7FU) << 8) | data[3];

    *hum = (float)raw_hum / 10.0f;
    *temp = (float)raw_temp / 10.0f;
    if ((data[2] & 0x80U) != 0U)
    {
      *temp = -*temp;
    }
  }
#else
  *hum = (float)data[0] + ((float)data[1] / 10.0f);
  *temp = (float)data[2] + ((float)data[3] / 10.0f);
#endif

  return DHT_OK;
}

static void DHT_SetPinOutput(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT_DATA_GPIO_Port, &GPIO_InitStruct);
}

static void DHT_SetPinInput(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT_DATA_GPIO_Port, &GPIO_InitStruct);
}

static void DHT_DelayUs(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  while ((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) < us)
  {
  }
}

static DHT_StatusTypeDef DHT_WaitForPin(GPIO_PinState state, uint16_t timeout_us)
{
  uint16_t start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);

  while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) != state)
  {
    if ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim2) - start) > timeout_us)
    {
      return DHT_ERROR_TIMEOUT;
    }
  }

  return DHT_OK;
}
