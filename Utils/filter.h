#ifndef FILTER_H
#define FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MOVING_AVG_WINDOW_SIZE 5U

typedef struct
{
  float buffer[MOVING_AVG_WINDOW_SIZE];
  float sum;
  uint8_t index;
  uint8_t count;
} MovingAvgFilter;

void MovingAvg_Init(MovingAvgFilter *filter);
float MovingAvg_Update(MovingAvgFilter *filter, float value);
float MovingAvg_Get(const MovingAvgFilter *filter);

#ifdef __cplusplus
}
#endif

#endif /* FILTER_H */
