#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IM_MAX(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define IM_MIN(a,b)     ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

#define COLOR_R8_MIN 0
#define COLOR_R8_MAX 255
#define COLOR_G8_MIN 0
#define COLOR_G8_MAX 255
#define COLOR_B8_MIN 0
#define COLOR_B8_MAX 255

#define COLOR_RGB565_TO_R5(pixel) (((pixel) >> 11) & 0x1F)
#define COLOR_RGB565_TO_R8(pixel) \
({ \
    __typeof__ (pixel) __pixel = (pixel); \
    __pixel = (__pixel >> 8) & 0xF8; \
    __pixel | (__pixel >> 5); \
})

#define COLOR_RGB565_TO_G6(pixel) (((pixel) >> 5) & 0x3F)
#define COLOR_RGB565_TO_G8(pixel) \
({ \
    __typeof__ (pixel) __pixel = (pixel); \
    __pixel = (__pixel >> 3) & 0xFC; \
    __pixel | (__pixel >> 6); \
})

#define COLOR_RGB565_TO_B5(pixel) ((pixel) & 0x1F)
#define COLOR_RGB565_TO_B8(pixel) \
({ \
    __typeof__ (pixel) __pixel = (pixel); \
    __pixel = (__pixel << 3) & 0xF8; \
    __pixel | (__pixel >> 5); \
})

#define COLOR_RGB888_TO_Y(r8, g8, b8) ((((r8) * 38) + ((g8) * 75) + ((b8) * 15)) >> 7) // 0.299R + 0.587G + 0.114B
#define COLOR_RGB565_TO_Y(rgb565) \
({ \
    __typeof__ (rgb565) __rgb565 = (rgb565); \
    int r = COLOR_RGB565_TO_R8(__rgb565); \
    int g = COLOR_RGB565_TO_G8(__rgb565); \
    int b = COLOR_RGB565_TO_B8(__rgb565); \
    COLOR_RGB888_TO_Y(r, g, b); \
})

#define COLOR_Y_TO_RGB888(pixel) ((pixel) * 0x010101)
#define COLOR_Y_TO_RGB565(pixel) \
({ \
    __typeof__ (pixel) __pixel = (pixel); \
    int __rb_pixel = (__pixel >> 3) & 0x1F; \
    (__rb_pixel * 0x0801) + ((__pixel << 3) & 0x7E0); \
})

#define COLOR_RGB888_TO_U(r8, g8, b8) ((((r8) * -21) - ((g8) * 43) + ((b8) * 64)) >> 7) // -0.168736R - 0.331264G + 0.5B
#define COLOR_RGB565_TO_U(rgb565) \
({ \
    __typeof__ (rgb565) __rgb565 = (rgb565); \
    int r = COLOR_RGB565_TO_R8(__rgb565); \
    int g = COLOR_RGB565_TO_G8(__rgb565); \
    int b = COLOR_RGB565_TO_B8(__rgb565); \
    COLOR_RGB888_TO_U(r, g, b); \
})

#define COLOR_RGB888_TO_V(r8, g8, b8) ((((r8) * 64) - ((g8) * 54) - ((b8) * 10)) >> 7) // 0.5R - 0.418688G - 0.081312B
#define COLOR_RGB565_TO_V(rgb565) \
({ \
    __typeof__ (rgb565) __rgb565 = (rgb565); \
    int r = COLOR_RGB565_TO_R8(__rgb565); \
    int g = COLOR_RGB565_TO_G8(__rgb565); \
    int b = COLOR_RGB565_TO_B8(__rgb565); \
    COLOR_RGB888_TO_V(r, g, b); \
})

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define COLOR_YUV_TO_RGB565(y, u, v)                yuv_to_rgb565((y) + 128, u, v)
#define COLOR_R5_G6_B5_TO_RGB565(r5, g6, b5)        (((r5) << 11) | ((g6) << 5) | (b5))
#define COLOR_R8_G8_B8_TO_RGB565(r8, g8, b8)        ((((r8) & 0xF8) << 8) | (((g8) & 0xFC) << 3) | ((b8) >> 3))

#define COLOR_RGB565_TO_GRAYSCALE(pixel)            COLOR_RGB565_TO_Y(pixel)
#define COLOR_RGB888_TO_GRAYSCALE(r8, g8, b8)       COLOR_RGB888_TO_Y(r8, g8, b8)

static inline void yuv_to_rgb888(uint8_t y, int8_t u, int8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = IM_MAX(IM_MIN(y + ((91881 * v) >> 16), COLOR_R8_MAX), COLOR_R8_MIN);
    *g = IM_MAX(IM_MIN(y - (((22554 * u) + (46802 * v)) >> 16), COLOR_G8_MAX), COLOR_G8_MIN);
    *b = IM_MAX(IM_MIN(y + ((116130 * u) >> 16), COLOR_B8_MAX), COLOR_B8_MIN);
}

// https://en.wikipedia.org/wiki/YCbCr -> JPEG Conversion
static inline uint16_t yuv_to_rgb565(uint8_t y, int8_t u, int8_t v)
{
    uint8_t r, g, b;

    yuv_to_rgb888(y, u, v, &r, &g, &b);

    return COLOR_R8_G8_B8_TO_RGB565(r, g, b);
}

#ifdef __cplusplus
}
#endif
