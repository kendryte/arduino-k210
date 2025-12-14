#pragma once

#include <stdint.h>

#include "FS.h"
#include "image_cvt.h"

namespace K210
{
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

        // Generic cut and convert function
        static int cut_to_new_format(Image *src, Image *dst, rectangle_t &r, image_format_t new_format, bool create);
        int cut_to_new_format(Image *dst, rectangle_t &r, image_format_t new_format, bool create = true);
        Image * cut_to_new_format(rectangle_t &r, image_format_t new_format);

        // resize origin image to new size
        static int resize(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize(uint32_t width, uint32_t height);

        // resize and convert image to new format
        static int resize_to_new_format(Image *src, Image *dst, uint32_t width, uint32_t height, image_format_t new_format, bool create);
        int resize_to_new_format(Image *dst, uint32_t width, uint32_t height, image_format_t new_format, bool create = true);
        Image * resize_to_new_format(uint32_t width, uint32_t height, image_format_t new_format);

        // convert image to grayscale
        static int to_grayscale(Image *src, Image *dst, bool create);
        int to_grayscale(Image *dst, bool create = true);
        Image * to_grayscale(void);

        // cut and convert image to grayscale
        int cut_to_grayscale(Image *dst, rectangle_t &r, bool create = true);
        Image * cut_to_grayscale(rectangle_t &r);

        // resize and convert image to grayscale
        static int resize_to_grayscale(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize_to_grayscale(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize_to_grayscale(uint32_t width, uint32_t height);

        // convert image to rgb565
        static int to_rgb565(Image *src, Image *dst, bool create);
        int to_rgb565(Image *dst, bool create = true);
        Image * to_rgb565(void);

        // cut and convert image to rgb565
        int cut_to_rgb565(Image *dst, rectangle_t &r, bool create = true);
        Image * cut_to_rgb565(rectangle_t &r);

        // resize and convert image to rgb565
        static int resize_to_rgb565(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize_to_rgb565(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize_to_rgb565(uint32_t width, uint32_t height);

        // convert image to rgb888
        static int to_rgb888(Image *src, Image *dst, bool create);
        int to_rgb888(Image *dst, bool create = true);
        Image * to_rgb888(void);

        // cut and convert image to rgb888
        int cut_to_rgb888(Image *dst, rectangle_t &r, bool create = true);
        Image * cut_to_rgb888(rectangle_t &r);

        // resize and convert image to rgb888
        static int resize_to_rgb888(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize_to_rgb888(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize_to_rgb888(uint32_t width, uint32_t height);

        // convert image to rgbp888 for ai inference
        static int to_rgbp888(Image *src, Image *dst, bool create);
        int to_rgbp888(Image *dst, bool create = true);
        Image * to_rgbp888(void);

        // cut and convert image to rgbp888
        int cut_to_rgbp888(Image *dst, rectangle_t &r, bool create = true);
        Image * cut_to_rgbp888(rectangle_t &r);

        // resize and convert image to rgbp888
        static int resize_to_rgbp888(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);
        int resize_to_rgbp888(Image *dst, uint32_t width, uint32_t height, bool create = true);
        Image * resize_to_rgbp888(uint32_t width, uint32_t height);

        static int load_bmp(Image *dst, fs::FS &fs, const char *name);
        static Image * load_bmp(fs::FS &fs, const char *name);

        static int save_bmp(Image *img, fs::FS &fs, const char *name);
        int save_bmp(fs::FS &fs, const char *name);

        // JPEG save functions
        static int save_jpeg(Image *img, fs::FS &fs, const char *name, int quality = 80);
        int save_jpeg(fs::FS &fs, const char *name, int quality = 80);

        static int compress_jpeg(Image *img, uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality = 80);
        int compress_jpeg(uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality = 80);

        // Getter functions
        uint32_t width() const { return mWidth; }
        uint32_t height() const { return mHeight; }
        uint32_t bpp() const { return mBpp; }
        image_format_t format() const { return mFormat; }
        uint8_t* data() const { return mPixel; }
        uint8_t* pixel() const { return mPixel; }

        // Function to calculate image size
        uint32_t size() const { return mWidth * mHeight * mBpp; }

    private:
        uint32_t mWidth, mHeight, mBpp;
        image_format_t mFormat;
        uint8_t *mPixel;

        bool mUserBuffer;
        bool mBufferNotAlign;

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
            case IMAGE_FORMAT_RGBP888:
                bpp = 3;
                break;
            default:
                break;
            }
            return bpp;
        }
    };

}
