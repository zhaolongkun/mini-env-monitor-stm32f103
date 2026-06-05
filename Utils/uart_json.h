#ifndef UART_JSON_H
#define UART_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void UART_SendEnvJson(UART_HandleTypeDef *huart,
                      float temp,
                      float hum,
                      uint32_t time_s,
                      uint8_t alarm);

#ifdef __cplusplus
}
#endif

#endif /* UART_JSON_H */
