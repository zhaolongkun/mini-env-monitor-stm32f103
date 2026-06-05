#include "button.h"

#include "main.h"

#define BUTTON_DEBOUNCE_MS 20U

static uint8_t button_last_raw_pressed;
static uint8_t button_stable_pressed;
static uint8_t button_press_latched;
static uint8_t button_short_press_event;
static uint32_t button_last_change_ms;

static uint8_t Button_ReadPressed(void)
{
  return (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

void Button_Init(void)
{
  button_last_raw_pressed = Button_ReadPressed();
  button_stable_pressed = button_last_raw_pressed;
  button_press_latched = button_stable_pressed;
  button_short_press_event = 0U;
  button_last_change_ms = HAL_GetTick();
}

void Button_Update(uint32_t now_ms)
{
  uint8_t raw_pressed = Button_ReadPressed();

  if (raw_pressed != button_last_raw_pressed)
  {
    button_last_raw_pressed = raw_pressed;
    button_last_change_ms = now_ms;
  }

  if (((uint32_t)(now_ms - button_last_change_ms) >= BUTTON_DEBOUNCE_MS) &&
      (raw_pressed != button_stable_pressed))
  {
    button_stable_pressed = raw_pressed;

    if (button_stable_pressed != 0U)
    {
      button_press_latched = 1U;
    }
    else if (button_press_latched != 0U)
    {
      button_press_latched = 0U;
      button_short_press_event = 1U;
    }
  }
}

uint8_t Button_GetShortPressEvent(void)
{
  uint8_t event = button_short_press_event;
  button_short_press_event = 0U;
  return event;
}
