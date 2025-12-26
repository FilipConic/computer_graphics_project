#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>
typedef struct {
	uint8_t r, g, b, a;
} Color;

#define COLOR_WHITE ((Color){ .r = 255, .g = 255, .b = 255, .a = 255 })
#define COLOR_GRAY ((Color) { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0x18 })
#define COLOR_BLACK ((Color){ .r = 0, .g = 0, .b = 0, .a = 255 })
#define COLOR_NONE ((Color){ .r = 0, .g = 0, .b = 0, .a = 0 })

#define COLOR_UNPACK_F(clr) ((clr).r / 255.0f), ((clr).g / 255.0f), ((clr).b / 255.0f), ((clr).a / 255.0f)

#endif // __COLOR_H__
