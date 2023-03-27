#define DBG_TAG "IMAGE"
#define DBG_LVL DBG_INFO // DBG_WARNING
#include "rtdbg.h"
#include "rtthread.h"

#include <string.h>

#include "Image.h"
#include "image_cvt.h"

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
        format = IMAGE_FORMAT_INVAILD;
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
            LOG_E("Invaild image.");
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

}


#undef CREATE_DEST_IMAGE
