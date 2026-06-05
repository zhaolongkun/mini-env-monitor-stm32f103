#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Button_Init(void);
void Button_Update(uint32_t now_ms);
uint8_t Button_GetShortPressEvent(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
