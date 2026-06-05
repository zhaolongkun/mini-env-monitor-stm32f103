#include "filter.h"

void MovingAvg_Init(MovingAvgFilter *filter)
{
  uint8_t i;

  if (filter == 0)
  {
    return;
  }

  filter->sum = 0.0f;
  filter->index = 0U;
  filter->count = 0U;

  for (i = 0U; i < MOVING_AVG_WINDOW_SIZE; i++)
  {
    filter->buffer[i] = 0.0f;
  }
}

float MovingAvg_Update(MovingAvgFilter *filter, float value)
{
  if (filter == 0)
  {
    return value;
  }

  if (filter->count < MOVING_AVG_WINDOW_SIZE)
  {
    filter->buffer[filter->index] = value;
    filter->sum += value;
    filter->count++;
  }
  else
  {
    filter->sum -= filter->buffer[filter->index];
    filter->buffer[filter->index] = value;
    filter->sum += value;
  }

  filter->index++;
  if (filter->index >= MOVING_AVG_WINDOW_SIZE)
  {
    filter->index = 0U;
  }

  return MovingAvg_Get(filter);
}

float MovingAvg_Get(const MovingAvgFilter *filter)
{
  if ((filter == 0) || (filter->count == 0U))
  {
    return 0.0f;
  }

  return filter->sum / (float)filter->count;
}
