#define DBG_TAG "IMAGE"
#define DBG_LVL DBG_INFO // DBG_WARNING
#include "rtdbg.h"
#include "rtthread.h"

#include <string.h>

#include "Image.h"
#include "image_cvt.h"
#include "jpege.h"

// Configure the image reader.
#define STBI_NO_JPEG
#define STBI_NO_PNG
//#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM   //(.ppm and .pgm)

#define STBI_NO_STDIO
#define STBI_NO_LINEAR

#define STBI_NO_SIMD

#define STBI_MALLOC(sz)             rt_malloc(sz)
#define STBI_REALLOC(p,newsz)       rt_realloc(p, newsz)
#define STBI_FREE(p)                rt_free(p)
#define STBI_ASSERT(x)              RT_ASSERT(x)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// configure the image writer
#define STBIW_MALLOC(sz)            rt_malloc(sz)
#define STBIW_REALLOC(p,newsz)      rt_realloc(p,newsz)
#define STBIW_FREE(p)               rt_free(p)
#define STBIW_ASSERT(x)             RT_ASSERT(x)

#define STBI_WRITE_NO_STDIO

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define CREATE_DEST_IMAGE(f)                                                    \
    if (false == create)                                                        \
    {                                                                           \
        if (NULL == dst->pixel)                                                 \
        {                                                                       \
            LOG_E("dst image no pixel buffer");                                 \
            return -1;                                                          \
        }                                                                       \
        if ((src->w != dst->w) ||                                               \
            (src->h != dst->h) ||                                               \
            (dst->bpp != format_to_bpp(f)) ||                                   \
            (dst->format != f))                                                 \
        {                                                                       \
            LOG_E("dst image format error");                                    \
            return -1;                                                          \
        }                                                                       \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        dst->w = src->w;                                                        \
        dst->h = src->h;                                                        \
        dst->bpp = format_to_bpp(f);                                            \
        dst->format = f;                                                        \
        if (dst->pixel)                                                         \
        {                                                                       \
            LOG_W("free old pixel buffer %p", dst->pixel);                      \
            rt_free_align(dst->pixel);                                          \
            dst->pixel = NULL;                                                  \
        }                                                                       \
        dst->pixel = (uint8_t *)rt_malloc_align(dst->w * dst->h * dst->bpp, 8); \
        if (NULL == dst->pixel)                                                 \
        {                                                                       \
            LOG_E("Malloc failed");                                             \
            return -1;                                                          \
        }                                                                       \
    }

namespace K210
{

    Image::Image()
    {
        w = 0;
        h = 0;
        bpp = 0;
        format = IMAGE_FORMAT_INVALID;
        pixel = NULL;
        user_buffer = false;
        buffer_not_align = false;
    }

    Image::Image(uint32_t width, uint32_t height, image_format_t f, bool create)
    {
        w = width;
        h = height;
        format = f;
        bpp = format_to_bpp(f);
        user_buffer = false;
        buffer_not_align = false;

        if(create)
        {
            pixel = (uint8_t *)rt_malloc_align(w * h * bpp, 8);

            if(NULL == pixel)
            {
                LOG_E("Malloc failed");
            }
        }
        else
        {
            pixel = NULL;
        }
    }

    Image::Image(uint32_t width, uint32_t height, image_format_t f, uint8_t *buffer)
    {
        w = width;
        h = height;
        format = f;
        bpp = format_to_bpp(f);
        pixel = buffer;
        user_buffer = true;
        buffer_not_align = false;
    }

    Image::~Image()
    {
        if(pixel && (false == user_buffer))
        {
            if(buffer_not_align)
            {
                rt_free(pixel);
            }
            else
            {
                rt_free_align(pixel);
            }
        }
    }

    int Image::cut(Image *src, Image *dst, rectangle_t &r, bool create)
    {
        uint32_t ow = src->w; /* origin image width */
        uint32_t oh = src->h; /* origin image height */

        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        if((false == create) && (NULL == dst->pixel))
        {
            LOG_E("dst image no pixel buffer");
            return -1;
        }

        if ((r.x > ow) || ((r.x + r.w) > ow) ||
            (r.y > oh) || ((r.y + r.h) > oh))
        {
            LOG_E("cut out image too large for origin image.");
            return -1;
        }

        dst->w = r.w;
        dst->h = r.h;
        dst->bpp = src->bpp;
        dst->format = src->format;

        if(true == create)
        {
            if (dst->pixel)
            {
                LOG_W("free old pixel buffer %p", dst->pixel);
                rt_free_align(dst->pixel);
                dst->pixel = NULL;
            }

            dst->pixel = (uint8_t *)rt_malloc_align(dst->w * dst->h * dst->bpp, 8);
            if (NULL == dst->pixel)
            {
                LOG_E("Malloc failed");
                return -1;
            }
        }

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        case IMAGE_FORMAT_RGB565:
        case IMAGE_FORMAT_RGB888:
        {
            for (uint32_t i = 0; i < r.h; i++)
            {
                memcpy(dst->pixel + i * r.w * dst->bpp, src->pixel + (ow * (i + r.y) + r.x) * src->bpp, r.w * dst->bpp);
            }
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            // copy r channel
            for (uint32_t i = 0; i < r.h; i++)
            {
                memcpy(dst->pixel + i * r.w + r.w * r.h * 0, src->pixel + ow * (i + r.y) + r.x + ow * oh * 0, r.w);
            }

            // copy g channel
            for (uint32_t i = 0; i < r.h; i++)
            {
                memcpy(dst->pixel + i * r.w + r.w * r.h * 1, src->pixel + ow * (i + r.y) + r.x + ow * oh * 1, r.w);
            }

            // copy b channel
            for (uint32_t i = 0; i < r.h; i++)
            {
                memcpy(dst->pixel + i * r.w + r.w * r.h * 2, src->pixel + ow * (i + r.y) + r.x + ow * oh * 2, r.w);
            }
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::cut(Image *dst, rectangle_t &r, bool create)
    {
        return cut(this, dst, r, create);
    }

    Image * Image::cut(rectangle_t &r)
    {
        Image *img = new Image();
        if(0x00 != cut(this, img, r, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    int Image::resize(Image *src, Image *dst, uint32_t width, uint32_t height, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        if((false == create) && (NULL == dst->pixel))
        {
            LOG_E("dst image no pixel buffer");
            return -1;
        }

        dst->w = width;
        dst->h = height;
        dst->bpp = src->bpp;
        dst->format = src->format;

        if(true == create)
        {
            if (dst->pixel)
            {
                LOG_W("free old pixel buffer %p", dst->pixel);
                rt_free_align(dst->pixel);
                dst->pixel = NULL;
            }

            dst->pixel = (uint8_t *)rt_malloc_align(dst->w * dst->h * dst->bpp, 8);
            if (NULL == dst->pixel)
            {
                LOG_E("Malloc failed");
                return -1;
            }
        }

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        {
            uint8_t *srcImg = src->pixel;
            uint8_t *dstImg = dst->pixel;

            float xf, sx = (float)(src->w) / dst->w;
            float yf, sy = (float)(src->h) / dst->h;
            int x, y, x0, y0, x1, y1, val_x1, val_y1;

            if ((src->w == dst->w) && (src->h == dst->h)) // just copy
            {
                memcpy(dstImg, srcImg, src->w * src->h * src->bpp);
            }
            else if ((src->w > dst->w) || (src->h > dst->h)) // scale down
            {
                for (y = 0; y < dst->h; y++)
                {
                    y0 = y * sy;
                    y1 = (y + 1) * sy;

                    for (x = 0; x < dst->w; x++)
                    {
                        int sum, xx, yy;

                        sum = 0;
                        x0 = x * sy;
                        x1 = (x + 1) * sy;

                        for (yy = y0; yy <= y1; yy++)
                        {
                            for (xx = x0; xx <= x1; xx++)
                            {
                                sum += srcImg[yy * src->w + xx];
                            }
                        }
                        dstImg[y * dst->w + x] = sum / ((y1 - y0 + 1) * (x1 - x0 + 1));
                    }
                }
            }
            else // scale up
            {
                for (y = 0; y < dst->h; y++)
                {
                    yf = (y + 0.5) * sy - 0.5;
                    y0 = (int)yf;
                    y1 = y0 + 1;
                    val_y1 = y0 < src->h - 1 ? y1 : y0;
                    for (x = 0; x < dst->w; x++)
                    {
                        xf = (x + 0.5) * sx - 0.5;
                        x0 = (int)xf;
                        x1 = x0 + 1;
                        val_x1 = x0 < src->w - 1 ? x1 : x0;
                        dstImg[y * dst->w + x] = (uint8_t)(srcImg[y0 * src->w + x0] * (x1 - xf) * (y1 - yf) +
                                                      srcImg[y0 * src->w + val_x1] * (xf - x0) * (y1 - yf) +
                                                      srcImg[val_y1 * src->w + x0] * (x1 - xf) * (yf - y0) +
                                                      srcImg[val_y1 * src->w + val_x1] * (xf - x0) * (yf - y0));
                    }
                }
            }
        }
        break;
        case IMAGE_FORMAT_RGB565:
        {
            uint16_t *srcImg = (uint16_t *)src->pixel;
            uint16_t *dstImg = (uint16_t *)dst->pixel;

            float sx = (float)(src->w) / dst->w;
            float sy = (float)(src->h) / dst->h;
            int x, y, x0, y0;
            uint16_t x1, x2, y1, y2;

            if ((src->w == dst->w) && (src->h == dst->h)) // just copy
            {
                memcpy(dstImg, srcImg, src->w * src->h * src->bpp);
            }
            else if ((src->w > dst->w) || (src->h > dst->h)) // scale down
            {
                for (y = 0; y < dst->h; y++)
                {
                    y0 = y * sy;
                    for (x = 0; x < dst->w; x++)
                    {
                        x0 = x * sx;
                        dstImg[y * dst->w + x] = srcImg[y0 * src->w + x0];
                    }
                }
            }
            else // scale up
            {
                float x_src, y_src;
                float temp1, temp2;
                uint8_t temp_r, temp_g, temp_b;
                for (y = 0; y < dst->h; y++)
                {
                    for (x = 0; x < dst->w; x++)
                    {
                        x_src = (x + 0.5f) * sx - 0.5f;
                        y_src = (y + 0.5f) * sy - 0.5f;
                        x1 = (uint16_t)x_src;
                        x2 = x1 + 1;
                        y1 = (uint16_t)y_src;
                        y2 = y1 + 1;

                        if (x2 >= src->w || y2 >= src->h)
                        {
                            dstImg[x + y * dst->w] = srcImg[x1 + y1 * src->w];
                            continue;
                        }

                        temp1 = (x2 - x_src) * COLOR_RGB565_TO_R5(srcImg[x1 + y1 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_R5(srcImg[x2 + y1 * src->w]);
                        temp2 = (x2 - x_src) * COLOR_RGB565_TO_R5(srcImg[x1 + y2 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_R5(srcImg[x2 + y2 * src->w]);
                        temp_r = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * COLOR_RGB565_TO_G6(srcImg[x1 + y1 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_G6(srcImg[x2 + y1 * src->w]);
                        temp2 = (x2 - x_src) * COLOR_RGB565_TO_G6(srcImg[x1 + y2 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_G6(srcImg[x2 + y2 * src->w]);
                        temp_g = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * COLOR_RGB565_TO_B5(srcImg[x1 + y1 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_B5(srcImg[x2 + y1 * src->w]);
                        temp2 = (x2 - x_src) * COLOR_RGB565_TO_B5(srcImg[x1 + y2 * src->w]) + (x_src - x1) * COLOR_RGB565_TO_B5(srcImg[x2 + y2 * src->w]);
                        temp_b = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        dstImg[x + y * dst->w] = COLOR_R5_G6_B5_TO_RGB565(temp_r, temp_g, temp_b);
                    }
                }
            }
        }
        break;
        case IMAGE_FORMAT_RGB888:
        {
            uint8_t *srcImg = src->pixel;
            uint8_t *dstImg = dst->pixel;

            float sx = (float)(src->w) / dst->w;
            float sy = (float)(src->h) / dst->h;
            int x, y, x0, y0;
            uint16_t x1, x2, y1, y2;

            if ((src->w == dst->w) && (src->h == dst->h)) // just copy
            {
                memcpy(dstImg, srcImg, src->w * src->h * src->bpp);
            }
            else if ((src->w > dst->w) || (src->h > dst->h)) // scale down
            {
                for (y = 0; y < dst->h; y++)
                {
                    y0 = y * sy;
                    for (x = 0; x < dst->w; x++)
                    {
                        x0 = x * sx;
                        dstImg[(y * dst->w + x) * src->bpp + 0] = srcImg[(y0 * src->w + x0) * src->bpp + 0];
                        dstImg[(y * dst->w + x) * src->bpp + 1] = srcImg[(y0 * src->w + x0) * src->bpp + 1];
                        dstImg[(y * dst->w + x) * src->bpp + 2] = srcImg[(y0 * src->w + x0) * src->bpp + 2];
                    }
                }
            }
            else // scale up
            {
                float x_src, y_src;
                float temp1, temp2;
                uint8_t temp_r, temp_g, temp_b;
                for (y = 0; y < dst->h; y++)
                {
                    for (x = 0; x < dst->w; x++)
                    {
                        x_src = (x + 0.5f) * sx - 0.5f;
                        y_src = (y + 0.5f) * sy - 0.5f;
                        x1 = (uint16_t)x_src;
                        x2 = x1 + 1;
                        y1 = (uint16_t)y_src;
                        y2 = y1 + 1;

                        if (x2 >= src->w || y2 >= src->h)
                        {
                            dstImg[x + y * dst->w] = srcImg[x1 + y1 * src->w];
                            continue;
                        }

                        temp1 = (x2 - x_src) * srcImg[(x1 + y1 * src->w) * src->bpp + 0] + (x_src - x1) * srcImg[(x2 + y1 * src->w) * src->bpp + 0];
                        temp2 = (x2 - x_src) * srcImg[(x1 + y2 * src->w) * src->bpp + 0] + (x_src - x1) * srcImg[(x2 + y2 * src->w) * src->bpp + 0];
                        temp_r = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * srcImg[(x1 + y1 * src->w) * src->bpp + 1] + (x_src - x1) * srcImg[(x2 + y1 * src->w) * src->bpp + 1];
                        temp2 = (x2 - x_src) * srcImg[(x1 + y2 * src->w) * src->bpp + 1] + (x_src - x1) * srcImg[(x2 + y2 * src->w) * src->bpp + 1];
                        temp_g = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * srcImg[(x1 + y1 * src->w) * src->bpp + 2] + (x_src - x1) * srcImg[(x2 + y1 * src->w) * src->bpp + 2];
                        temp2 = (x2 - x_src) * srcImg[(x1 + y2 * src->w) * src->bpp + 2] + (x_src - x1) * srcImg[(x2 + y2 * src->w) * src->bpp + 2];
                        temp_b = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        dstImg[(x + y * dst->w) * src->bpp + 0] = temp_r;
                        dstImg[(x + y * dst->w) * src->bpp + 1] = temp_g;
                        dstImg[(x + y * dst->w) * src->bpp + 2] = temp_b;
                    }
                }
            }
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            uint16_t x1, x2, y1, y2;
            float w_scale, h_scale;
            float temp1, temp2;
            float x_src, y_src;

            uint8_t *r_src, *g_src, *b_src, *r_dst, *g_dst, *b_dst;
            uint16_t w_src, h_src, w_dst, h_dst;

            if ((src->w == dst->w) && (src->h == dst->h)) // just copy
            {
                memcpy(dst->pixel, src->pixel, src->w * src->h * src->bpp);
            }
            else
            {
                w_src = src->w;
                h_src = src->h;
                r_src = src->pixel;
                g_src = r_src + w_src * h_src;
                b_src = g_src + w_src * h_src;

                w_dst = dst->w;
                h_dst = dst->h;
                r_dst = dst->pixel;
                g_dst = r_dst + w_dst * h_dst;
                b_dst = g_dst + w_dst * h_dst;

                w_scale = (float)w_src / w_dst;
                h_scale = (float)h_src / h_dst;

                for (uint16_t y = 0; y < h_dst; y++)
                {
                    for (uint16_t x = 0; x < w_dst; x++)
                    {
                        x_src = (x + 0.5f) * w_scale - 0.5f;
                        x1 = (uint16_t)x_src;
                        x2 = x1 + 1;
                        y_src = (y + 0.5f) * h_scale - 0.5f;
                        y1 = (uint16_t)y_src;
                        y2 = y1 + 1;

                        if (x2 >= w_src || y2 >= h_src)
                        {
                            r_dst[x + y * w_dst] = r_src[x1 + y1 * w_src];
                            g_dst[x + y * w_dst] = g_src[x1 + y1 * w_src];
                            b_dst[x + y * w_dst] = b_src[x1 + y1 * w_src];
                            continue;
                        }

                        temp1 = (x2 - x_src) * r_src[x1 + y1 * w_src] + (x_src - x1) * r_src[x2 + y1 * w_src];
                        temp2 = (x2 - x_src) * r_src[x1 + y2 * w_src] + (x_src - x1) * r_src[x2 + y2 * w_src];
                        r_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * g_src[x1 + y1 * w_src] + (x_src - x1) * g_src[x2 + y1 * w_src];
                        temp2 = (x2 - x_src) * g_src[x1 + y2 * w_src] + (x_src - x1) * g_src[x2 + y2 * w_src];
                        g_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);

                        temp1 = (x2 - x_src) * b_src[x1 + y1 * w_src] + (x_src - x1) * b_src[x2 + y1 * w_src];
                        temp2 = (x2 - x_src) * b_src[x1 + y2 * w_src] + (x_src - x1) * b_src[x2 + y2 * w_src];
                        b_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
                    }
                }
            }
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::resize(Image *dst, uint32_t width, uint32_t height, bool create)
    {
        return resize(this, dst, width, height, create);
    }

    Image * Image::resize(uint32_t width, uint32_t height)
    {
        Image *img = new Image();
        if(0x00 != resize(this, img, width, height, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    int Image::to_grayscale(Image *src, Image *dst, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        CREATE_DEST_IMAGE(IMAGE_FORMAT_GRAYSCALE)

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        {
            LOG_W("Convert grayscale to grayscale, copy the image");

            memcpy(dst->pixel, src->pixel, src->w * src->h * src->bpp);
        }
        break;
        case IMAGE_FORMAT_RGB565:
        {
            uint8_t *grayscale = dst->pixel;
            uint8_t *end = grayscale + dst->w * dst->h * dst->bpp;

            uint16_t pixel, *rgb565 = (uint16_t *)src->pixel;

            while(end != grayscale)
            {
                pixel = *rgb565++;
                *grayscale++ = COLOR_RGB565_TO_GRAYSCALE(pixel);
            }
        }
        break;
        case IMAGE_FORMAT_RGB888:
        {
            uint8_t *grayscale = dst->pixel;
            uint8_t *end = grayscale + dst->w * dst->h * dst->bpp;

            uint8_t r, g, b, *rgb888 = src->pixel;

            while (end != grayscale)
            {
                r = rgb888[0];
                g = rgb888[1];
                b = rgb888[2];
                rgb888 += 3;

                *grayscale++ = COLOR_RGB888_TO_GRAYSCALE(r, g, b);
            }
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            uint8_t *grayscale = dst->pixel;
            uint8_t *end = grayscale + dst->w * dst->h * dst->bpp;

            uint8_t r, *sr = src->pixel;
            uint8_t g, *sg = sr + src->w * src->h;
            uint8_t b, *sb = sg + src->w * src->h;

            while (end != grayscale)
            {
                r = *sr++;
                g = *sg++;
                b = *sb++;

                *grayscale++ = COLOR_RGB888_TO_GRAYSCALE(r, g, b);
            }
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::to_grayscale(Image *dst, bool create)
    {
        return to_grayscale(this, dst, create);
    }

    Image * Image::to_grayscale(void)
    {
        Image *img = new Image();
        if(0x00 != to_grayscale(this, img, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    int Image::to_rgb565(Image *src, Image *dst, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        CREATE_DEST_IMAGE(IMAGE_FORMAT_RGB565)

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        {
            uint16_t *rgb565 = (uint16_t *)dst->pixel;
            uint16_t *end = rgb565 + dst->w * dst->h;

            uint8_t gray, *grayscale = src->pixel;

            while (end != rgb565)
            {
                gray = *grayscale++;
                *rgb565++ = yuv_to_rgb565(gray, 0, 0);
            }
        }
        break;
        case IMAGE_FORMAT_RGB565:
        {
            LOG_W("Convert rgb565 to rgb565, copy the image");

            memcpy(dst->pixel, src->pixel, src->w * src->h * src->bpp);
        }
        break;
        case IMAGE_FORMAT_RGB888:
        {
            uint16_t *rgb565 = (uint16_t *)dst->pixel;
            uint16_t *end = rgb565 + dst->w * dst->h;

            uint8_t r, g, b, *rgb888 = src->pixel;

            while (end != rgb565)
            {
                r = (uint8_t)rgb888[0];
                g = (uint8_t)rgb888[1];
                b = (uint8_t)rgb888[2];
                rgb888 += 3;

                *rgb565++ = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
            }
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            uint16_t *rgb565 = (uint16_t *)dst->pixel;
            uint16_t *end = rgb565 + dst->w * dst->h;

            uint8_t r, *sr = src->pixel;
            uint8_t g, *sg = sr + src->w * src->h;
            uint8_t b, *sb = sg + src->w * src->h;

            while (end != rgb565)
            {
                r = *sr++;
                g = *sg++;
                b = *sb++;

                *rgb565++ = COLOR_R8_G8_B8_TO_RGB565(r, g, b);
            }
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::to_rgb565(Image *dst, bool create)
    {
        return to_rgb565(this, dst, create);
    }

    Image * Image::to_rgb565(void)
    {
        Image *img = new Image();
        if(0x00 != to_rgb565(this, img, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    int Image::to_rgb888(Image *src, Image *dst, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        CREATE_DEST_IMAGE(IMAGE_FORMAT_RGB888)

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        {
            uint8_t *rgb888 = dst->pixel;
            uint8_t *end = rgb888 + dst->w * dst->h * dst->bpp;

            uint8_t gray, *grayscale = src->pixel;

            while(end != rgb888)
            {
                gray = *grayscale++;

                yuv_to_rgb888(gray, 0, 0, rgb888, rgb888 + 1, rgb888 + 2);
                rgb888 += 3;
            }
        }
        break;
        case IMAGE_FORMAT_RGB565:
        {
            uint8_t *rgb888 = dst->pixel;
            uint8_t *end = rgb888 + dst->w * dst->h * dst->bpp;

            uint16_t pixel, *rgb565 = (uint16_t *)src->pixel;

            while(end != rgb888)
            {
                pixel = *rgb565++;

                rgb888[0] = COLOR_RGB565_TO_R8(pixel);
                rgb888[1] = COLOR_RGB565_TO_G8(pixel);
                rgb888[2] = COLOR_RGB565_TO_B8(pixel);

                rgb888 += 3;
            }
        }
        break;
        case IMAGE_FORMAT_RGB888:
        {
            LOG_W("Convert rgb888 to rgb888, copy the image");

            memcpy(dst->pixel, src->pixel, src->w * src->h * src->bpp);
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            uint8_t *rgb888 = dst->pixel;
            uint8_t *end = rgb888 + dst->w * dst->h * dst->bpp;

            uint8_t *sr = src->pixel;
            uint8_t *sg = sr + src->w * src->h;
            uint8_t *sb = sg + src->w * src->h;

            while(end != rgb888)
            {
                rgb888[0] = *sr++;
                rgb888[1] = *sg++;
                rgb888[2] = *sb++;

                rgb888 += 3;
            }
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::to_rgb888(Image *dst, bool create)
    {
        return to_rgb888(this, dst, create);
    }

    Image * Image::to_rgb888(void)
    {
        Image *img = new Image();
        if(0x00 != to_rgb888(this, img, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    int Image::to_r8g8b8(Image *src, Image *dst, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        CREATE_DEST_IMAGE(IMAGE_FORMAT_R8G8B8)

        switch (src->format)
        {
        case IMAGE_FORMAT_GRAYSCALE:
        {
            uint8_t *dr = dst->pixel;
            uint8_t *dg = dr + dst->w * dst->h;
            uint8_t *db = dg + dst->w * dst->h;

            uint8_t *end = dg;

            uint8_t gray, *grayscale = src->pixel;

            while(end != dr)
            {
                gray = *grayscale++;

                yuv_to_rgb888(gray, 0, 0, dr++, dg++, db++);
            }
        }
        break;
        case IMAGE_FORMAT_RGB565:
        {
            uint8_t *dr = dst->pixel;
            uint8_t *dg = dr + dst->w * dst->h;
            uint8_t *db = dg + dst->w * dst->h;

            uint8_t *end = dg;

            uint16_t pixel, *rgb565 = (uint16_t *)src->pixel;

            while(end != dr)
            {
                pixel = *rgb565++;

                *dr++ = COLOR_RGB565_TO_R8(pixel);
                *dg++ = COLOR_RGB565_TO_G8(pixel);
                *db++ = COLOR_RGB565_TO_B8(pixel);
            }
        }
        break;
        case IMAGE_FORMAT_RGB888:
        {
            uint8_t *dr = dst->pixel;
            uint8_t *dg = dr + dst->w * dst->h;
            uint8_t *db = dg + dst->w * dst->h;

            uint8_t *end = dg;

            uint8_t *rgb888 = src->pixel;

            while(end != dr)
            {
                *dr++ = rgb888[0];
                *dg++ = rgb888[1];
                *db++ = rgb888[2];

                rgb888 += 3;
            }
        }
        break;
        case IMAGE_FORMAT_R8G8B8:
        {
            LOG_W("Convert r8g8b8 to r8g8b8, copy the image");

            memcpy(dst->pixel, src->pixel, src->w * src->h * src->bpp);
        }
        break;
        default:
        {
            LOG_E("Unsupport format %d", dst->format);
            return -1;
        }
        break;
        }

        return 0;
    }

    int Image::to_r8g8b8(Image *dst, bool create)
    {
        return to_r8g8b8(this, dst, create);
    }

    Image * Image::to_r8g8b8(void)
    {
        Image *img = new Image();
        if(0x00 != to_r8g8b8(this, img, true))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    #define GET_PIXEL_PTR(img, x, y) \
        ((img)->pixel + ((y) * (img)->w + (x)) * (img)->bpp)

    int Image::cut_to_new_format(Image *src, Image *dst, rectangle_t &r, image_format_t new_format, bool create)
    {
        if (NULL == src->pixel)
        {
            LOG_E("Origin image no pixels");
            return -1;
        }

        uint32_t ow = src->w; /* origin image width */
        uint32_t oh = src->h; /* origin image height */

        uint32_t dst_w = r.w;
        uint32_t dst_h = r.h;
        image_format_t dst_format = new_format;
        uint32_t dst_bpp = format_to_bpp(dst_format);

        if ((r.x > ow) || ((r.x + r.w) > ow) ||
            (r.y > oh) || ((r.y + r.h) > oh))
        {
            LOG_E("cut out image too large for origin image.");
            return -1;
        }

        // --- Allocation and initialization block (Unchanged) ---
        if (false == create)
        {
            if (NULL == dst->pixel)
            {
                LOG_E("dst image no pixel buffer");
                return -1;
            }
            if ((dst_w != dst->w) ||
                (dst_h != dst->h) ||
                (dst_bpp != dst->bpp) ||
                (dst_format != dst->format))
            {
                LOG_E("dst image format or size mismatch in non-create mode.");
                return -1;
            }
        }
        else // create == true
        {
            // 1. Set new properties based on cut area and new format
            dst->w = dst_w;
            dst->h = dst_h;
            dst->bpp = dst_bpp;
            dst->format = dst_format;
            dst->user_buffer = false; 

            // 2. Free old buffer if present and not user-provided
            if (dst->pixel && !dst->user_buffer)
            {
                LOG_W("free old pixel buffer %p", dst->pixel);
                rt_free_align(dst->pixel);
                dst->pixel = NULL;
            }

            // 3. Allocate new buffer
            uint32_t size = dst_w * dst_h * dst_bpp;
            dst->pixel = (uint8_t *)rt_malloc_align(size, 8);

            if (NULL == dst->pixel)
            {
                LOG_E("Malloc failed for dst buffer");
                return -1;
            }
        }
        // --- End of Allocation block ---

        uint32_t x, y;

        // Handle direct cut (same format)
        if (src->format == dst_format)
        {
            if (dst_format == IMAGE_FORMAT_R8G8B8)
            {
                uint32_t s_plane_size = src->w * src->h;
                uint32_t d_plane_size = dst_w * dst_h;
                uint32_t bytes_to_copy = dst_w;

                for (uint32_t plane = 0; plane < src->bpp; plane++) 
                {
                    uint8_t *s_base = src->pixel + s_plane_size * plane;
                    uint8_t *d_base = dst->pixel + d_plane_size * plane;

                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *s_ptr = s_base + (r.y + y) * src->w + r.x;
                        uint8_t *d_ptr = d_base + y * dst_w;
                        memcpy(d_ptr, s_ptr, bytes_to_copy);
                    }
                }
            }
            else // Standard case for packed/contiguous formats (RGB565, RGB888, GRAYSCALE)
            {
                // This original logic works for packed formats (RGB565, RGB888) and single-channel (GRAYSCALE)
                for (y = 0; y < dst_h; y++)
                {
                    uint8_t *src_ptr = GET_PIXEL_PTR(src, r.x, r.y + y);
                    uint8_t *dst_ptr = GET_PIXEL_PTR(dst, 0, y);
                    memcpy(dst_ptr, src_ptr, dst_w * dst_bpp);
                }
            }
            return 0;
        }
        // ----------------------------------------------------------------------
        // --- FIX ENDS HERE ---


        // ----------------------------------------------------------------------
        // --- Conversion from IMAGE_FORMAT_RGB565 (2 BPP, Packed Source) ---
        // ----------------------------------------------------------------------
        if (src->format == IMAGE_FORMAT_RGB565)
        {
            switch (dst_format)
            {
                case IMAGE_FORMAT_R8G8B8: // Planar Destination
                {
                    uint32_t d_plane_size = dst_w * dst_h;
                    uint8_t *d_r = dst->pixel;
                    uint8_t *d_g = d_r + d_plane_size;
                    uint8_t *d_b = d_g + d_plane_size;

                    for (y = 0; y < dst_h; y++)
                    {
                        uint16_t *src_ptr_row = (uint16_t *)GET_PIXEL_PTR(src, r.x, r.y + y);

                        for (x = 0; x < dst_w; x++)
                        {
                            uint16_t pixel565 = src_ptr_row[x];
                            uint32_t d_offset = y * dst_w + x; // Planar offset

                            d_r[d_offset] = COLOR_RGB565_TO_R8(pixel565);
                            d_g[d_offset] = COLOR_RGB565_TO_G8(pixel565);
                            d_b[d_offset] = COLOR_RGB565_TO_B8(pixel565);
                        }
                    }
                    return 0;
                }

                case IMAGE_FORMAT_RGB888: // Packed Destination
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint16_t *src_ptr_row = (uint16_t *)GET_PIXEL_PTR(src, r.x, r.y + y);
                        uint8_t *dst_ptr_row = GET_PIXEL_PTR(dst, 0, y);

                        for (x = 0; x < dst_w; x++)
                        {
                            uint16_t pixel565 = src_ptr_row[x];
                            
                            dst_ptr_row[0] = COLOR_RGB565_TO_R8(pixel565);
                            dst_ptr_row[1] = COLOR_RGB565_TO_G8(pixel565);
                            dst_ptr_row[2] = COLOR_RGB565_TO_B8(pixel565);

                            dst_ptr_row += 3;
                        }
                    }
                    return 0;
                }

                case IMAGE_FORMAT_GRAYSCALE:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint16_t *src_ptr_row = (uint16_t *)GET_PIXEL_PTR(src, r.x, r.y + y);
                        uint8_t *dst_ptr_row = GET_PIXEL_PTR(dst, 0, y);
                        
                        for (x = 0; x < dst_w; x++)
                        {
                            uint16_t pixel565 = src_ptr_row[x];
                            
                            #ifdef COLOR_RGB565_TO_GRAYSCALE
                            dst_ptr_row[x] = COLOR_RGB565_TO_GRAYSCALE(pixel565);
                            #else
                            uint8_t r8 = COLOR_RGB565_TO_R8(pixel565);
                            uint8_t g8 = COLOR_RGB565_TO_G8(pixel565);
                            uint8_t b8 = COLOR_RGB565_TO_B8(pixel565);
                            dst_ptr_row[x] = (uint8_t)((299 * r8 + 587 * g8 + 114 * b8) / 1000);
                            #endif
                        }
                    }
                    return 0;
                }
                default: 
                {
                    LOG_E("Unsupported RGB565 source to Dst %d", dst_format);
                    return -1;
                }
            }
        }

        // ----------------------------------------------------------------------
        // --- Conversion from IMAGE_FORMAT_RGB888 (3 BPP, Packed Source) ---
        // ----------------------------------------------------------------------
        if (src->format == IMAGE_FORMAT_RGB888)
        {
            switch (dst_format)
            {
                case IMAGE_FORMAT_RGB565:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *src_ptr_row = GET_PIXEL_PTR(src, r.x, r.y + y);
                        uint16_t *dst_ptr_row = (uint16_t *)GET_PIXEL_PTR(dst, 0, y);

                        for (x = 0; x < dst_w; x++)
                        {
                            uint8_t r8 = src_ptr_row[0];
                            uint8_t g8 = src_ptr_row[1];
                            uint8_t b8 = src_ptr_row[2];

                            dst_ptr_row[x] = COLOR_R8_G8_B8_TO_RGB565(r8, g8, b8);

                            src_ptr_row += 3;
                        }
                    }
                    return 0;
                }

                case IMAGE_FORMAT_R8G8B8: // Planar Destination
                {
                    uint32_t d_plane_size = dst_w * dst_h;
                    uint8_t *d_r = dst->pixel;
                    uint8_t *d_g = d_r + d_plane_size;
                    uint8_t *d_b = d_g + d_plane_size;

                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *src_ptr_row = GET_PIXEL_PTR(src, r.x, r.y + y);
                        
                        for (x = 0; x < dst_w; x++)
                        {
                            uint32_t d_offset = y * dst_w + x; // Planar offset

                            d_r[d_offset] = src_ptr_row[0];
                            d_g[d_offset] = src_ptr_row[1];
                            d_b[d_offset] = src_ptr_row[2];

                            src_ptr_row += 3; // Source is packed
                        }
                    }
                    return 0;
                }

                case IMAGE_FORMAT_GRAYSCALE:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *src_ptr_row = GET_PIXEL_PTR(src, r.x, r.y + y);
                        uint8_t *dst_ptr_row = GET_PIXEL_PTR(dst, 0, y);

                        for (x = 0; x < dst_w; x++)
                        {
                            uint8_t r8 = src_ptr_row[0];
                            uint8_t g8 = src_ptr_row[1];
                            uint8_t b8 = src_ptr_row[2];
                            
                            dst_ptr_row[x] = (uint8_t)((299 * r8 + 587 * g8 + 114 * b8) / 1000);
                            
                            src_ptr_row += 3;
                        }
                    }
                    return 0;
                }
                default: 
                {
                    LOG_E("Unsupported RGB888 source to Dst %d", dst_format);
                    return -1;
                }
            }
        }

        // ----------------------------------------------------------------------
        // --- Conversion from IMAGE_FORMAT_R8G8B8 (3 BPP, Planar Source) ---
        // ----------------------------------------------------------------------
        if (src->format == IMAGE_FORMAT_R8G8B8)
        {
            // Calculate source plane pointers once
            uint32_t s_plane_size = src->w * src->h;
            uint8_t *s_r_base = src->pixel;
            uint8_t *s_g_base = s_r_base + s_plane_size;
            uint8_t *s_b_base = s_g_base + s_plane_size;

            switch (dst_format)
            {
                case IMAGE_FORMAT_RGB565:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint16_t *dst_ptr_row = (uint16_t *)GET_PIXEL_PTR(dst, 0, y);
                        
                        for (x = 0; x < dst_w; x++)
                        {
                            uint32_t s_offset = (r.y + y) * src->w + r.x + x;
                            
                            uint8_t r8 = s_r_base[s_offset];
                            uint8_t g8 = s_g_base[s_offset];
                            uint8_t b8 = s_b_base[s_offset];
                            
                            dst_ptr_row[x] = COLOR_R8_G8_B8_TO_RGB565(r8, g8, b8);
                        }
                    }
                    return 0;
                }
                case IMAGE_FORMAT_RGB888:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *dst_ptr_row = GET_PIXEL_PTR(dst, 0, y);
                        
                        for (x = 0; x < dst_w; x++)
                        {
                            uint32_t s_offset = (r.y + y) * src->w + r.x + x;
                            
                            uint8_t r8 = s_r_base[s_offset];
                            uint8_t g8 = s_g_base[s_offset];
                            uint8_t b8 = s_b_base[s_offset];
                            
                            dst_ptr_row[0] = r8;
                            dst_ptr_row[1] = g8;
                            dst_ptr_row[2] = b8;
                            
                            dst_ptr_row += 3;
                        }
                    }
                    return 0;
                }
                case IMAGE_FORMAT_GRAYSCALE:
                {
                    for (y = 0; y < dst_h; y++)
                    {
                        uint8_t *dst_ptr_row = GET_PIXEL_PTR(dst, 0, y);
                        
                        for (x = 0; x < dst_w; x++)
                        {
                            uint32_t s_offset = (r.y + y) * src->w + r.x + x;
                            
                            uint8_t r8 = s_r_base[s_offset];
                            uint8_t g8 = s_g_base[s_offset];
                            uint8_t b8 = s_b_base[s_offset];
                            
                            dst_ptr_row[x] = (uint8_t)((299 * r8 + 587 * g8 + 114 * b8) / 1000);
                        }
                    }
                    return 0;
                }
                default: 
                {
                    LOG_E("Unsupported R8G8B8 source to Dst %d", dst_format);
                    return -1;
                }
            }
        }

        // ----------------------------------------------------------------------
        // --- Conversion from IMAGE_FORMAT_GRAYSCALE (1 BPP Source) ---
        // ----------------------------------------------------------------------
        if (src->format == IMAGE_FORMAT_GRAYSCALE)
        {
            LOG_E("Unsupported GRAYSCALE source to Dst %d in cut_to_new_format", dst_format);
            return -1;
        }

        LOG_E("Unsupported format combination: Src %d to Dst %d", src->format, dst_format);

        return -1;
    }

    int Image::cut_to_new_format(Image *dst, rectangle_t &r, image_format_t new_format, bool create)
    {
        return cut_to_new_format(this, dst, r, new_format, create);
    }

    Image * Image::cut_to_new_format(rectangle_t &r, image_format_t new_format)
    {
        Image *img = new Image();

        if (0x00 != cut_to_new_format(this, img, r, new_format, true))
        {
            delete img;
            return NULL;
        }

        return img;
    }

    int Image::cut_to_r8g8b8(Image *dst, rectangle_t &r, bool create)
    {
        return cut_to_new_format(this, dst, r, IMAGE_FORMAT_R8G8B8, create);
    }

    Image * Image::cut_to_r8g8b8(rectangle_t &r)
    {
        return cut_to_new_format(r, IMAGE_FORMAT_R8G8B8);
    }

    int Image::cut_to_rgb565(Image *dst, rectangle_t &r, bool create)
    {
        return cut_to_new_format(this, dst, r, IMAGE_FORMAT_RGB565, create);
    }

    Image * Image::cut_to_rgb565(rectangle_t &r)
    {
        return cut_to_new_format(r, IMAGE_FORMAT_RGB565);
    }

    int Image::cut_to_rgb888(Image *dst, rectangle_t &r, bool create)
    {
        return cut_to_new_format(this, dst, r, IMAGE_FORMAT_RGB888, create);
    }

    Image * Image::cut_to_rgb888(rectangle_t &r)
    {
        return cut_to_new_format(r, IMAGE_FORMAT_RGB888);
    }

    int Image::cut_to_grayscale(Image *dst, rectangle_t &r, bool create)
    {
        return cut_to_new_format(this, dst, r, IMAGE_FORMAT_GRAYSCALE, create);
    }

    Image * Image::cut_to_grayscale(rectangle_t &r)
    {
        return cut_to_new_format(r, IMAGE_FORMAT_GRAYSCALE);
    }

    int Image::load_bmp(Image *dst, fs::FS &fs, const char *name)
    {
        fs::File file;
        uint8_t hdr[256];
        int x, y, comp;

        file = fs.open(name);
        if(!file || file.isDirectory())
        {
            LOG_E("open file %s failed.", name);
            file.close();
            return -1;
        }

        if(sizeof(hdr) != file.read(hdr, sizeof(hdr)))
        {
            LOG_E("read file hdr failed");
            file.close();
            return -1;
        }

        if(0x01 != stbi_info_from_memory(hdr, sizeof(hdr), &x, &y, &comp))
        {
            LOG_E("parse file info failed with error \'%s\'", stbi_failure_reason());
            file.close();
            return -1;
        }

        size_t bmp_size = file.size();

        uint8_t *temp = (uint8_t *)rt_malloc_align(bmp_size, 8);
        if(NULL == temp)
        {
            file.close();
            LOG_E("malloc failed");
            return -1;
        }

        file.seek(0, fs::SeekSet);
        if(bmp_size != file.read(temp, bmp_size))
        {
            LOG_E("read file failed");
            rt_free_align(temp);
            file.close();
            return -1;
        }
        file.close();

        uint8_t *img = stbi_load_from_memory(temp, bmp_size, &x, &y, &comp, STBI_rgb);
        rt_free_align(temp);

        if((NULL == img) || (0x03 != comp))
        {
            rt_kprintf("load img failed, %s RGB888", (0x03 != comp) ? "NOT" : "IS");
            return -1;
        }

        dst->w = x;
        dst->h = y;
        dst->pixel = img;
        dst->format = IMAGE_FORMAT_RGB888;
        dst->bpp = format_to_bpp(IMAGE_FORMAT_RGB888);
        dst->buffer_not_align = true;

        return 0;
    }

    Image * Image::load_bmp(fs::FS &fs, const char *name)
    {
        Image *img = new Image();
        if(0x00 != load_bmp(img, fs, name))
        {
            delete img;
            return NULL;
        }
        return img;
    }

    struct save_bmp_temp_file {
        uint8_t *buffer;
        size_t size;

        size_t offset;
    };

    /// @brief Write bmp callback
    /// @param context struct save_bmp_temp_file *_buff
    /// @param data to write data pointer
    /// @param size to write data size
    extern "C" void stbi_write_func(void *context, void *data, int size)
    {
        size_t _size = static_cast<size_t>(size);
        uint8_t *_data = reinterpret_cast<uint8_t *>(data);
        struct save_bmp_temp_file *_buff = reinterpret_cast<struct save_bmp_temp_file*>(context);

        if(!_size)
        {
            return;
        }

        if(!_buff || !_data)
        {
            LOG_W("write image faild: !_buff %d, !_data %d", !_buff, !_data);
            return;
        }

        if((_buff->offset + _size) > _buff->size)
        {
            LOG_E("write outof the buffer");
            return;
        }

        memcpy(_buff->buffer + _buff->offset, data, _size);
        _buff->offset += _size;
    }

    int Image::save_bmp(Image *img, fs::FS &fs, const char *name)
    {
        if (IMAGE_FORMAT_RGB888 != img->format)
        {
            LOG_E("write bmp only support rgb888");
            return -1;
        }

        if ((NULL == img->pixel) ||
            (0x00 == img->w) ||
            (0x00 == img->h) ||
            (0x00 == img->bpp))
        {
            LOG_E("Invalid image.");
            return -1;
        }

        struct save_bmp_temp_file _buff;

        _buff.offset = 0;
        _buff.size = img->w * img->h * img->bpp + 512;
        _buff.buffer = (uint8_t *)rt_malloc_align(_buff.size, 8);
        if(NULL == _buff.buffer)
        {
            LOG_E("malloc failed");
            return -1;
        }

        LOG_I("save bmp width %d, height %d, bpp %d, pixel %p", img->w, img->h, img->bpp, img->pixel);

        if(0x00 == stbi_write_bmp_to_func(stbi_write_func, (void *)&_buff, img->w, img->h, img->bpp, (void *)img->pixel))
        {
            rt_free_align(_buff.buffer);

            LOG_E("write bmp failed.");
            return -1;
        }

        LOG_I("Bmp size is %ld", _buff.offset);

        fs::File file = fs.open(name, FILE_WRITE);
        if (!file)
        {
            rt_free_align(_buff.buffer);

            LOG_E("open file %s failed.", name);
            return -1;
        }

        if(_buff.offset != file.write(_buff.buffer, _buff.offset))
        {
            file.close();
            rt_free_align(_buff.buffer);

            LOG_E("write file failed");
            return -1;
        }

        file.close();
        rt_free_align(_buff.buffer);

        LOG_I("write bmp success");

        return 0;
    }

    int Image::save_bmp(fs::FS &fs, const char *name)
    {
        return save_bmp(this, fs, name);
    }

    int Image::save_jpeg(Image *img, fs::FS &fs, const char *name, int quality)
    {
        if ((NULL == img->pixel) ||
            (0x00 == img->w) ||
            (0x00 == img->h) ||
            (0x00 == img->bpp))
        {
            LOG_E("Invalid image.");
            return -1;
        }

        // Check if format is supported for JPEG compression
        if ((IMAGE_FORMAT_GRAYSCALE != img->format) && (IMAGE_FORMAT_RGB565 != img->format))
        {
            LOG_E("JPEG compression only supports GRAYSCALE and RGB565 formats.");
            return -1;
        }

        // Validate quality parameter
        if (quality < 1 || quality > 100)
        {
            LOG_W("JPEG quality should be between 1-100, using default 80");
            quality = 80;
        }

        // Create image_t structure for JPEG compression
        image_t src_img;
        src_img.w = img->w;
        src_img.h = img->h;
        src_img.data = img->pixel;
        src_img.size = img->w * img->h * img->bpp;
        src_img.pixfmt = img->format;

        // Allocate buffer for JPEG compression
        // Estimate JPEG size (typically 10-20% of original for quality 80)
        size_t estimated_size = (src_img.size * 20) / 100;
        if (estimated_size < 1024) estimated_size = 1024; // Minimum 1KB
        
        uint8_t *jpeg_buffer = (uint8_t *)rt_malloc(estimated_size);
        if (NULL == jpeg_buffer)
        {
            LOG_E("Failed to allocate memory for JPEG buffer");
            return -1;
        }

        size_t jpeg_size = estimated_size;
        int result = jpeg_compress(&src_img, jpeg_buffer, &jpeg_size, quality, JPEG_SUBSAMPLING_AUTO);

        if (result != 0)
        {
            LOG_E("JPEG compression failed with error code: %d", result);
            rt_free(jpeg_buffer);
            return -1;
        }

        // Write JPEG data to file
        fs::File file = fs.open(name, FILE_WRITE);
        if (!file)
        {
            LOG_E("Failed to open file %s for writing", name);
            rt_free(jpeg_buffer);
            return -1;
        }

        if (jpeg_size != file.write(jpeg_buffer, jpeg_size))
        {
            LOG_E("Failed to write JPEG data to file");
            file.close();
            rt_free(jpeg_buffer);
            return -1;
        }

        file.close();
        rt_free(jpeg_buffer);

        LOG_I("JPEG saved successfully to %s, size: %d bytes", name, jpeg_size);
        return 0;
    }

    int Image::save_jpeg(fs::FS &fs, const char *name, int quality)
    {
        return save_jpeg(this, fs, name, quality);
    }

    int Image::compress_jpeg(Image *img, uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality)
    {
        if ((NULL == img->pixel) ||
            (0x00 == img->w) ||
            (0x00 == img->h) ||
            (0x00 == img->bpp) ||
            (NULL == jpeg_buffer) ||
            (NULL == jpeg_size))
        {
            LOG_E("Invalid parameters for JPEG compression");
            return -1;
        }

        // Validate quality parameter
        if (quality < 1 || quality > 100)
        {
            LOG_W("JPEG quality should be between 1-100, using default 80");
            quality = 80;
        }

        // Create image_t structure for JPEG compression
        image_t src_img;
        src_img.w = img->w;
        src_img.h = img->h;
        src_img.data = img->pixel;
        src_img.size = img->w * img->h * img->bpp;
        src_img.pixfmt = img->format;

        // Check if provided buffer has reasonable minimum size
        if (buffer_capacity < 1024)
        {
            LOG_E("Provided buffer capacity (%d bytes) is too small, minimum 1024 bytes required", buffer_capacity);
            return -1;
        }
 
        // Compress directly into user-provided buffer
        size_t temp_jpeg_size = buffer_capacity;
        int result = jpeg_compress(&src_img, jpeg_buffer, &temp_jpeg_size, quality, JPEG_SUBSAMPLING_AUTO);

        if (result != 0)
        {
            LOG_E("JPEG compression failed with error code: %d", result);
            return -1;
        }

        // Return actual compressed size
        *jpeg_size = temp_jpeg_size;

        LOG_D("JPEG compressed successfully, size: %d bytes", *jpeg_size);
        return 0;
    }

    int Image::compress_jpeg(uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality)
    {
        return compress_jpeg(this, jpeg_buffer, buffer_capacity, jpeg_size, quality);
    }

}


#undef CREATE_DEST_IMAGE
