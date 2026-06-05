#ifndef OLED_FONT_H
#define OLED_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OLED_FONT_WIDTH 5U
#define OLED_FONT_SPACING 1U

uint8_t OLED_Font_GetColumn(char ch, uint8_t column);

#ifdef __cplusplus
}
#endif

#endif /* OLED_FONT_H */
