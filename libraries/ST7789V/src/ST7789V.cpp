#define DBG_TAG "ST7789V"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "ST7789V.h"

#include "Bus8080.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

ST7789V::ST7789V(int16_t w, int16_t h)
:Adafruit_GFX(w, h)
,_buffer(NULL)
,_userFrmBuff(false)
,_frameBuffer(NULL)
,_pixelMap(NULL)
,_pixelMapSize(0)
,_scrrenDir(DIR_XY_RLUD)
{

}

ST7789V::~ST7789V(void)
{
    if(_buffer) {
        rt_free_align(_buffer);

        _buffer = NULL;
        _frameBuffer = NULL;
        _pixelMap = NULL;
    }
}

void ST7789V::begin(uint32_t freq)
{
    int16_t tw, th;

    if(NULL != _buffer) {
        LOG_E("maybe begind.");
        return;
    }

    tw = (WIDTH + 7) / 8;
    th = (HEIGHT + 7) / 8;
    _pixelMapSize = tw * th;

    _buffer = (uint8_t *)rt_malloc_align((sizeof(uint16_t) * WIDTH * HEIGHT) + _pixelMapSize, 8);
    if(NULL == _buffer) {
        LOG_E("Malloc Buffers Failed!");
        return;
    }

    _frameBuffer = (uint16_t *)_buffer;
    _pixelMap = (uint8_t *)(_buffer + sizeof(uint16_t) * WIDTH * HEIGHT);
    _userFrmBuff = false;

    memset(_pixelMap, 0, _pixelMapSize);

    // begin
    K210::Bus8080::begin(freq);

    K210::Bus8080::reset(HIGH, 10);

    _InitRegister();

    setRotation(0);
}

int ST7789V::setFrameBuffer(K210::Image *img)
{
    if((_width != img->w) || \
        (_height != img->h) || \
        (NULL == img->pixel) || \
        (K210::IMAGE_FORMAT_RGB565 != img->format))
    {
        LOG_E("%s failed", __func__);
        return -1;
    }

    return setFrameBuffer((uint16_t *)img->pixel, static_cast<int16_t>(img->w), static_cast<int16_t>(img->h));
}

int ST7789V::setFrameBuffer(uint16_t *buffer, int16_t w, int16_t h)
{
    if((NULL == buffer) || \
        (_width != w) || \
        (_height != h))
    {
        return -1;
    }

    if(_buffer) {
        rt_free_align(_buffer);

        _buffer = NULL;
        _frameBuffer = NULL;
        _pixelMap = NULL;
    }

    _buffer = (uint8_t *)rt_malloc_align(_pixelMapSize, 8);
    if(NULL == _buffer) {
        LOG_E("Malloc Buffers Failed!");
        return -1;
    }

    _frameBuffer = (uint16_t *)buffer;
    _pixelMap = _buffer;

    _userFrmBuff = true;

    return 0;
}

void ST7789V::_InitRegister(void)
{
    /*soft reset*/
    K210::Bus8080::writeCommand((uint8_t)SOFTWARE_RESET);
    delay(10);
    /*exit sleep*/
    K210::Bus8080::writeCommand((uint8_t)SLEEP_OFF);
    delay(10);
    /*pixel format*/
    K210::Bus8080::writeCommand((uint8_t)PIXEL_FORMAT_SET);
    K210::Bus8080::writeData((uint8_t)0x55);
    /*display on*/
    K210::Bus8080::writeCommand((uint8_t)DISPALY_ON);
}

void ST7789V::_setDirection(lcd_dir_t dir)
{
    K210::Bus8080::writeCommand((uint8_t)MEMORY_ACCESS_CTL);
    K210::Bus8080::writeData(uint8_t(dir));
}

void ST7789V::setArea(int x1, int y1, int x2, int y2)
{
    uint8_t data[4] = {0};

    data[0] = highByte(x1);
    data[1] = lowByte(x1);
    data[2] = highByte(x2);
    data[3] = lowByte(x2);
    K210::Bus8080::writeCommand((uint8_t)HORIZONTAL_ADDRESS_SET);
    K210::Bus8080::writeData(data, 4);

    data[0] = highByte(y1);
    data[1] = lowByte(y1);
    data[2] = highByte(y2);
    data[3] = lowByte(y2);
    K210::Bus8080::writeCommand((uint8_t)VERTICAL_ADDRESS_SET);
    K210::Bus8080::writeData(data, 4);

    K210::Bus8080::writeCommand((uint8_t)MEMORY_WRITE);
}

void ST7789V::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (_frameBuffer) {
        if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
            return;

        _frameBuffer[x + y * _width] = color;

        if(_userFrmBuff)
            return;

        int16_t tx= x / 8;
        int16_t ty= y / 8;

        _pixelMap[ty * (_width / 8) + tx]++;
    }
}

void ST7789V::startWrite(void)
{
    if(_userFrmBuff)
        return;

    memset(_pixelMap, 0, _pixelMapSize);
}

void ST7789V::endWrite(void)
{
    uint16_t tmp_pixel[8 * 8];

    if(_userFrmBuff) {
        LOG_D("Use user framebuffer, should manual refresh the screen\n");
        return;
    }

    uint32_t dirty_cnt = 0;
    for(int i = 0; i < _pixelMapSize; i++) {
        if(_pixelMap[i]) {
            dirty_cnt++;
        }
    }

    if(((_width * _height) / 2) <= dirty_cnt) {
        setArea(0, 0, _width - 1, _height - 1);

        // K210::Bus8080::writeData((uint32_t *)_frameBuffer, (_width * _height) / 2); // start x point can nont be odd.
        K210::Bus8080::writeData(_frameBuffer, (_width * _height)); // start x point can be odd, but will alloc memory for transfer
    } else {
        for(int i = 0; i < (_height / 8); i++) {
            for(int j = 0; j < (_width / 8); j++) {
                if(_pixelMap[i * (_width / 8) + j]) {
                    dirty_cnt--;

                    _pixelMap[i * (_width / 8) + j] = 0;

                    int pin_cnt = 0;
                    for(int y = (i * 8); y < ((i + 1) * 8); y++) {
                        for(int x = (j * 8); x < ((j + 1) * 8); x++) {
                            tmp_pixel[pin_cnt++] = _frameBuffer[y * _width + x];
                        }
                    }

                    setArea(j * 8, i * 8, j * 8 + 7, i * 8 + 7);

                    // K210::Bus8080::writeData((uint32_t *)tmp_pixel, 32); // start x point can nont be odd.
                    K210::Bus8080::writeData(tmp_pixel, 64); // start x point can be odd, but will alloc memory for transfer
                }

                if(0x00 == dirty_cnt) {
                    goto _end;
                }
            }
        }
    }

_end:

    return;
}

void ST7789V::setRotation(uint8_t r)
{
    lcd_dir_t dir = DIR_XY_RLUD;

    rotation = (r & 3);

    if(!(rotation <= 3)) {
        LOG_E("error rotation");
        return;
    }

    switch (rotation) {
        case 0: {
            _width = WIDTH;
            _height = HEIGHT;
            dir = DIR_XY_RLUD;
            break;
        } case 1: {
            _width = HEIGHT;
            _height = WIDTH;
            dir = DIR_YX_LRUD;
            break;
        } case 2: {
            _width = WIDTH;
            _height = HEIGHT;
            dir = DIR_XY_LRDU;
            break;
        } case 3: {
            _width = HEIGHT;
            _height = WIDTH;
            dir = DIR_YX_RLDU;
            break;
        } default:;
    }

    _scrrenDir = dir;

    _setDirection(dir);
}

/**************************************************************************/
/*!
    @brief      Invert the display (ideally using built-in hardware command)
    @param   i  True if you want to invert, false to make 'normal'
*/
/**************************************************************************/
void ST7789V::invertDisplay(bool i)
{
    if(i) {
        K210::Bus8080::writeCommand((uint8_t)INVERSION_DISPALY_ON);
    } else {
        K210::Bus8080::writeCommand((uint8_t)INVERSION_DISPALY_OFF);
    }
}

void ST7789V::fillScreen(uint16_t color)
{
    // fill framebuffer.
    memset(_pixelMap, 0xFF, _pixelMapSize);

    for(int i = 0; i < (_width * _height); i++) {
        _frameBuffer[i] = color;
    }

    setArea(0, 0, _width - 1, _height - 1);

    K210::Bus8080::fillScreen(color, _width, _height);
}

void ST7789V::drawFastRGBBitmap(int16_t rx, int16_t ry, int16_t rw, int16_t rh, uint16_t *bitmap)
{
    memset(_pixelMap, 0xFF, _pixelMapSize);

    if (((rx + rw) > _width) || ((ry + rh) > _height))
    {
        return;
    }

    if ((0x00 == rx) &&
        (0x00 == ry) &&
        (rw == _width) &&
        (rh == _height))
    {
        // just copy the image
        memcpy((uint8_t *)_frameBuffer, (uint8_t *)bitmap, _width * _height * 2);
    }
    else
    {
        for (int i = 0; i < rh; i++)
        {
            memcpy((uint8_t *)_frameBuffer + ((i + ry) * _width + rx) * 2, (uint8_t *)bitmap + (i * rw) * 2, rw * 2);
        }
    }
}

void ST7789V::drawImage(const K210::Image *img, int16_t x, int16_t y)
{
    if (K210::IMAGE_FORMAT_RGB565 != img->format)
    {
        return;
    }

    if (((img->w + x) > _width) || ((img->h + y) > _height))
    {
        return;
    }

    if (NULL == img->pixel)
    {
        return;
    }

    drawFastRGBBitmap(x, y, img->w, img->h, (uint16_t *)img->pixel);
}

void ST7789V::refresh(void)
{
    setArea(0, 0, _width - 1, _height - 1);

    K210::Bus8080::writeData(_frameBuffer, _width * _height);
}
