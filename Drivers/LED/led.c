#include "led.h"

#include "main.h"

void LED_Init(void)
{
  LED_Off();
}

void LED_On(void)
{
  HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_RESET);
}

void LED_Off(void)
{
  HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
}

void LED_Toggle(void)
{
  HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
}
