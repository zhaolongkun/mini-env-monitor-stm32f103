#include "app.h"

#include "button.h"
#include "dht.h"
#include "filter.h"
#include "led.h"
#include "main.h"
#include "oled.h"
#include "tim.h"
#include "uart_json.h"
#include "usart.h"

#include <stdio.h>

#define APP_KEY_PERIOD_MS 10U
#define APP_OLED_PERIOD_MS 500U
#define APP_DHT_PERIOD_MS 1000U
#define APP_UART_PERIOD_MS 1000U
#define APP_LED_PERIOD_MS 1000U
#define APP_ALARM_TEMP_C 30.0f

static uint8_t current_page;
static float temperature;
static float humidity;
static uint32_t run_time_s;
static uint8_t alarm_flag;
static uint8_t dht_ok_flag;
static uint32_t dht_error_count;

static MovingAvgFilter temp_filter;
static MovingAvgFilter hum_filter;

static uint32_t last_key_ms;
static uint32_t last_oled_ms;
static uint32_t last_dht_ms;
static uint32_t last_uart_ms;
static uint32_t last_led_ms;
static uint8_t oled_dirty;

static uint8_t App_IsDue(uint32_t now_ms, uint32_t *last_ms, uint32_t period_ms);
static void App_ReadDhtTask(void);
static void App_LedTask(uint32_t now_ms);
static void App_RenderOled(void);
static void App_RenderPage0(void);
static void App_RenderPage1(void);
static void App_FormatTenth(float value, char *buffer, uint32_t buffer_len);

void App_Init(void)
{
  uint32_t now = HAL_GetTick();

  current_page = 0U;
  temperature = 0.0f;
  humidity = 0.0f;
  run_time_s = 0U;
  alarm_flag = 0U;
  dht_ok_flag = 0U;
  dht_error_count = 0U;
  oled_dirty = 1U;

  last_key_ms = now;
  last_oled_ms = now;
  last_dht_ms = now;
  last_uart_ms = now;
  last_led_ms = now;

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  LED_Init();
  Button_Init();
  DHT_Init();
  OLED_Init();
  MovingAvg_Init(&temp_filter);
  MovingAvg_Init(&hum_filter);

  OLED_Clear();
  OLED_ShowString(0U, 0U, "Mini Env Monitor");
  OLED_ShowString(0U, 2U, "Sensor Init");
  OLED_ShowString(0U, 4U, "UART1 115200");
  OLED_Update();
}

void App_Run(void)
{
  uint32_t now = HAL_GetTick();

  run_time_s = now / 1000U;

  if (App_IsDue(now, &last_key_ms, APP_KEY_PERIOD_MS) != 0U)
  {
    Button_Update(now);
    if (Button_GetShortPressEvent() != 0U)
    {
      current_page = (uint8_t)((current_page + 1U) % 2U);
      oled_dirty = 1U;
    }
  }

  if (App_IsDue(now, &last_dht_ms, APP_DHT_PERIOD_MS) != 0U)
  {
    App_ReadDhtTask();
  }

  alarm_flag = (temperature > APP_ALARM_TEMP_C) ? 1U : 0U;
  App_LedTask(now);

  if (App_IsDue(now, &last_uart_ms, APP_UART_PERIOD_MS) != 0U)
  {
    UART_SendEnvJson(&huart1, temperature, humidity, run_time_s, alarm_flag);
  }

  if ((oled_dirty != 0U) || (App_IsDue(now, &last_oled_ms, APP_OLED_PERIOD_MS) != 0U))
  {
    App_RenderOled();
    oled_dirty = 0U;
  }
}

static uint8_t App_IsDue(uint32_t now_ms, uint32_t *last_ms, uint32_t period_ms)
{
  if ((uint32_t)(now_ms - *last_ms) >= period_ms)
  {
    *last_ms = now_ms;
    return 1U;
  }

  return 0U;
}

static void App_ReadDhtTask(void)
{
  float raw_temp;
  float raw_hum;

  if (DHT_Read(&raw_temp, &raw_hum) == DHT_OK)
  {
    dht_ok_flag = 1U;
    temperature = MovingAvg_Update(&temp_filter, raw_temp);
    humidity = MovingAvg_Update(&hum_filter, raw_hum);
  }
  else
  {
    dht_ok_flag = 0U;
    dht_error_count++;
  }

  oled_dirty = 1U;
}

static void App_LedTask(uint32_t now_ms)
{
  if (alarm_flag != 0U)
  {
    LED_On();
    return;
  }

  if (App_IsDue(now_ms, &last_led_ms, APP_LED_PERIOD_MS) != 0U)
  {
    LED_Toggle();
  }
}

static void App_RenderOled(void)
{
  OLED_Clear();

  if (current_page == 0U)
  {
    App_RenderPage0();
  }
  else
  {
    App_RenderPage1();
  }

  OLED_Update();
}

static void App_RenderPage0(void)
{
  char line[24];
  char temp_str[16];
  char hum_str[16];

  App_FormatTenth(temperature, temp_str, sizeof(temp_str));
  App_FormatTenth(humidity, hum_str, sizeof(hum_str));

  OLED_ShowString(0U, 0U, "Mini Env Monitor");
  (void)snprintf(line, sizeof(line), "Temp: %s C", temp_str);
  OLED_ShowString(0U, 1U, line);
  (void)snprintf(line, sizeof(line), "Hum : %s %%", hum_str);
  OLED_ShowString(0U, 2U, line);
  (void)snprintf(line, sizeof(line), "Time: %lu s", (unsigned long)run_time_s);
  OLED_ShowString(0U, 3U, line);
  (void)snprintf(line, sizeof(line), "Alarm: %s", (alarm_flag != 0U) ? "ON" : "OFF");
  OLED_ShowString(0U, 4U, line);
  (void)snprintf(line, sizeof(line), "DHT: %s", (dht_ok_flag != 0U) ? "OK" : "WAIT");
  OLED_ShowString(0U, 5U, line);
}

static void App_RenderPage1(void)
{
  char line[24];

  OLED_ShowString(0U, 0U, "STM32F103C8T6");
  OLED_ShowString(0U, 1U, "UART1 115200");
  OLED_ShowString(0U, 2U, "I2C1 OLED");
  (void)snprintf(line, sizeof(line), "DHT Err: %lu", (unsigned long)dht_error_count);
  OLED_ShowString(0U, 3U, line);
  (void)snprintf(line, sizeof(line), "Run: %lu s", (unsigned long)run_time_s);
  OLED_ShowString(0U, 4U, line);
}

static void App_FormatTenth(float value, char *buffer, uint32_t buffer_len)
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
