#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>
typedef struct {
	uint8_t r, g, b, a;
} Color;

#define COLOR_LIGHT_GRAY ((Color){.r = 105, 	.g = 105, 	.b = 105, 	.a = 255 })
#define COLOR_WHITE ((Color)	{ .r = 255,		.g = 255,	.b = 255,	.a = 255 })
#define COLOR_GRAY ((Color)		{ .r = 0x18,	.g = 0x18,	.b = 0x18,	.a = 255 })
#define COLOR_BLACK ((Color)	{ .r = 0,		.g = 0,		.b = 0,		.a = 255 })
#define COLOR_YELLOW ((Color)	{ .r = 0xFC,	.g = 0xE8,	.b = 0x03,	.a = 255 })
#define COLOR_ORANGE ((Color)	{ .r = 0xFC,	.g = 0xB1,	.b = 0x03,	.a = 255 })
#define COLOR_PURPLE ((Color)	{ .r = 0xAA,	.g = 0x00,	.b = 0xE3,	.a = 255 })
#define COLOR_BLUE ((Color)		{ .r = 0x00,	.g = 0x00,	.b = 0xE3,	.a = 255 })
#define COLOR_LIGHT_BLUE ((Color){.r = 0x07,	.g = 0xB8,	.b = 0xDB,	.a = 255 })
#define COLOR_LIME ((Color)		{ .r = 0x2A,	.g = 0xE3,	.b = 0x00,	.a = 255 })
#define COLOR_GREEN ((Color)	{ .r = 0x24,	.g = 0x9C,	.b = 0x09,	.a = 255 })
#define COLOR_PINK ((Color)		{ .r = 0xE3,	.g = 0x44,	.b = 0x7E,	.a = 255 })
#define COLOR_RED ((Color)		{ .r = 0xDB,	.g = 0x07,	.b = 0x07,	.a = 255 })
#define COLOR_NONE ((Color)		{ .r = 0,		.g = 0,		.b = 0, 	.a = 0 })

#define COLOR_UNPACK_F(clr) ((clr).r / 255.0f), ((clr).g / 255.0f), ((clr).b / 255.0f), ((clr).a / 255.0f)

#endif // __COLOR_H__
