#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OLED_WIDTH 128U
#define OLED_HEIGHT 64U
#define OLED_I2C_ADDRESS 0x3CU

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* OLED_H */
