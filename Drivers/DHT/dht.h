#ifndef DHT_H
#define DHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#define DHT_TYPE_DHT11 11U
#define DHT_TYPE_DHT22 22U

#ifndef DHT_SENSOR_TYPE
#define DHT_SENSOR_TYPE DHT_TYPE_DHT11
#endif

typedef enum
{
  DHT_OK = 0,
  DHT_ERROR_TIMEOUT,
  DHT_ERROR_CHECKSUM,
  DHT_ERROR_PARAM
} DHT_StatusTypeDef;

void DHT_Init(void);
DHT_StatusTypeDef DHT_Read(float *temp, float *hum);

#ifdef __cplusplus
}
#endif

#endif /* DHT_H */
