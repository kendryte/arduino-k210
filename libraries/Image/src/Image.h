#pragma once

#include <stdint.h>

#include "FS.h"

namespace K210
{

    enum image_format_t : uint32_t
    {
        IMAGE_FORMAT_GRAYSCALE = 0, // bpp 1
        IMAGE_FORMAT_RGB565,        // bpp 2
        IMAGE_FORMAT_RGB888,        // bpp 3
        IMAGE_FORMAT_R8G8B8,        // bpp 3
        IMAGE_FORMAT_INVAILD = 4,
    };

    typedef struct rectangle
    {
        uint32_t x;
        uint32_t y;
        uint32_t w;
        uint32_t h;
    } rectangle_t;

    class Image
    {

    public:
        Image();
        Image(uint32_t width, uint32_t height, image_format_t f, bool create = false);
        Image(uint32_t width, uint32_t height, image_format_t f, uint8_t *buffer);
        ~Image();

        // cut from origin image
        static int cut(Image *src, Image *dst, rectangle_t &r, bool create);
        int cut(Image *dst, rectangle_t &r, bool create = true);
        Image * cut(rectangle_t &r);

        // resize origin image to new size
        static int resize(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize(uint32_t width, uint32_t height);

        // convert image to grayscale
        static int to_grayscale(Image *src, Image *dst, bool create);
        int to_grayscale(Image *dst, bool create = true);
        Image * to_grayscale(void);

        // convert image to rgb565
        static int to_rgb565(Image *src, Image *dst, bool create);
        int to_rgb565(Image *dst, bool create = true);
        Image * to_rgb565(void);

        // convert image to rgb888
        static int to_rgb888(Image *src, Image *dst, bool create);
        int to_rgb888(Image *dst, bool create = true);
        Image * to_rgb888(void);

        // convert image to r8g8b8 for ai inference
        static int to_r8g8b8(Image *src, Image *dst, bool create);
        int to_r8g8b8(Image *dst, bool create = true);
        Image * to_r8g8b8(void);

        static int load_bmp(Image *dst, fs::FS &fs, const char *name);
        static Image * load_bmp(fs::FS &fs, const char *name);

        static int save_bmp(Image *img, fs::FS &fs, const char *name);
        int save_bmp(fs::FS &fs, const char *name);

        uint32_t w, h, bpp;
        image_format_t format;

        uint8_t *pixel;

    private:
        bool user_buffer;
        bool buffer_not_align;

        static int format_to_bpp(image_format_t f)
        {
            int bpp = 0;

            switch (f)
            {
            case IMAGE_FORMAT_GRAYSCALE:
                bpp = 1;
                break;
            case IMAGE_FORMAT_RGB565:
                bpp = 2;
                break;
            case IMAGE_FORMAT_RGB888:
            case IMAGE_FORMAT_R8G8B8:
                bpp = 3;
                break;
            default:
                break;
            }
            return bpp;
        }
    };

}
