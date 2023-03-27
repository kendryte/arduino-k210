#pragma once

#include "Arduino.h"

#include "Adafruit_GFX.h"

#include "Image.h"

/* clang-format off */
#define NO_OPERATION            0x00
#define SOFTWARE_RESET          0x01
#define READ_ID                 0x04
#define READ_STATUS             0x09
#define READ_POWER_MODE         0x0A
#define READ_MADCTL             0x0B
#define READ_PIXEL_FORMAT       0x0C
#define READ_IMAGE_FORMAT       0x0D
#define READ_SIGNAL_MODE        0x0E
#define READ_SELT_DIAG_RESULT   0x0F
#define SLEEP_ON                0x10
#define SLEEP_OFF               0x11
#define PARTIAL_DISPALY_ON      0x12
#define NORMAL_DISPALY_ON       0x13
#define INVERSION_DISPALY_OFF   0x20
#define INVERSION_DISPALY_ON    0x21
#define GAMMA_SET               0x26
#define DISPALY_OFF             0x28
#define DISPALY_ON              0x29
#define HORIZONTAL_ADDRESS_SET  0x2A
#define VERTICAL_ADDRESS_SET    0x2B
#define MEMORY_WRITE            0x2C
#define COLOR_SET               0x2D
#define MEMORY_READ             0x2E
#define PARTIAL_AREA            0x30
#define VERTICAL_SCROL_DEFINE   0x33
#define TEAR_EFFECT_LINE_OFF    0x34
#define TEAR_EFFECT_LINE_ON     0x35
#define MEMORY_ACCESS_CTL       0x36
#define VERTICAL_SCROL_S_ADD    0x37
#define IDLE_MODE_OFF           0x38
#define IDLE_MODE_ON            0x39
#define PIXEL_FORMAT_SET        0x3A
#define WRITE_MEMORY_CONTINUE   0x3C
#define READ_MEMORY_CONTINUE    0x3E
#define SET_TEAR_SCANLINE       0x44
#define GET_SCANLINE            0x45
#define WRITE_BRIGHTNESS        0x51
#define READ_BRIGHTNESS         0x52
#define WRITE_CTRL_DISPALY      0x53
#define READ_CTRL_DISPALY       0x54
#define WRITE_BRIGHTNESS_CTL    0x55
#define READ_BRIGHTNESS_CTL     0x56
#define WRITE_MIN_BRIGHTNESS    0x5E
#define READ_MIN_BRIGHTNESS     0x5F
#define READ_ID1                0xDA
#define READ_ID2                0xDB
#define READ_ID3                0xDC
#define RGB_IF_SIGNAL_CTL       0xB0
#define NORMAL_FRAME_CTL        0xB1
#define IDLE_FRAME_CTL          0xB2
#define PARTIAL_FRAME_CTL       0xB3
#define INVERSION_CTL           0xB4
#define BLANK_PORCH_CTL         0xB5
#define DISPALY_FUNCTION_CTL    0xB6
#define ENTRY_MODE_SET          0xB7
#define BACKLIGHT_CTL1          0xB8
#define BACKLIGHT_CTL2          0xB9
#define BACKLIGHT_CTL3          0xBA
#define BACKLIGHT_CTL4          0xBB
#define BACKLIGHT_CTL5          0xBC
#define BACKLIGHT_CTL7          0xBE
#define BACKLIGHT_CTL8          0xBF
#define POWER_CTL1              0xC0
#define POWER_CTL2              0xC1
#define VCOM_CTL1               0xC5
#define VCOM_CTL2               0xC7
#define NV_MEMORY_WRITE         0xD0
#define NV_MEMORY_PROTECT_KEY   0xD1
#define NV_MEMORY_STATUS_READ   0xD2
#define READ_ID4                0xD3
#define POSITIVE_GAMMA_CORRECT  0xE0
#define NEGATIVE_GAMMA_CORRECT  0xE1
#define DIGITAL_GAMMA_CTL1      0xE2
#define DIGITAL_GAMMA_CTL2      0xE3
#define INTERFACE_CTL           0xF6

/* clang-format on */

typedef enum _lcd_dir
{
    DIR_XY_RLUD = 0x00,
    DIR_YX_RLUD = 0x20,
    DIR_XY_LRUD = 0x40,
    DIR_YX_LRUD = 0x60,
    DIR_XY_RLDU = 0x80,
    DIR_YX_RLDU = 0xA0,
    DIR_XY_LRDU = 0xC0,
    DIR_YX_LRDU = 0xE0,
    DIR_XY_MASK = 0x20,
    DIR_RL_MASK = 0x40,
    DIR_UD_MASK = 0x80
} lcd_dir_t;

class ST7789V : public Adafruit_GFX
{
public:
    ST7789V(int16_t w, int16_t h);

    ~ST7789V(void);

    /// @brief St7789v begin
    /// @param canvas Lcd display canvas buffer, size MUST bigger than WIDTH * HEIGHT * 2
    /// @param freq Tft Bus clock frequency
    virtual void begin(uint32_t freq = 15 * 1000 * 1000);

    int setFrameBuffer(K210::Image *img);
    int setFrameBuffer(uint16_t *buffer, int16_t w, int16_t h);

    /// @brief Set St7789v register
    /// @param  None
    /// @note User can override in subclass for different LCD Controller.
    /// @warning This can only called from begin.
    virtual void _InitRegister(void);

    /// @brief St7789v set display direction
    /// @param dir Direction
    /// @note User can override in subclass for different LCD Controller.
    virtual void _setDirection(lcd_dir_t dir);

    /// @brief St7789v set display area
    /// @param x1 Start x point
    /// @param y1 Start y point
    /// @param x2 End x point
    /// @param y2 End y point
    /// @note User can override in subclass for different LCD Controller.
    virtual void setArea(int x1, int y1, int x2, int y2);

    /// @brief St7789v fast(direct) draw rgb565 to screen
    /// @param x start point x
    /// @param y start pont y
    /// @param w draw area width
    /// @param h draw area height
    /// @param bitmap picture pointer
    /// @note This is for display camera image or other need fast speed use.
    virtual void drawFastRGBBitmap(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *bitmap);

    /// @brief Display RGB565 image on lcd
    /// @param img Image
    /// @param x start point x
    /// @param y start point y
    virtual void drawImage(const K210::Image *img, int16_t x = 0, int16_t y = 0);

    /// @brief Manual refresh lcd display, when call setFrameBuffer
    /// @param  None
    virtual void refresh(void);

    /**********************************************************************/
    /*!
      @brief  Draw to the screen/framebuffer/etc.
      Must be overridden in subclass.
      @param  x    X coordinate in pixels
      @param  y    Y coordinate in pixels
      @param color  16-bit pixel color.
    */
    /**********************************************************************/
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

    // TRANSACTION API / CORE DRAW API
    // These MAY be overridden by the subclass to provide device-specific
    // optimized code.  Otherwise 'generic' versions are used.
    virtual void startWrite(void);
    // virtual void writePixel(int16_t x, int16_t y, uint16_t color);
    // virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    // virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    // virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    // virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    virtual void endWrite(void);

    // CONTROL API
    // These MAY be overridden by the subclass to provide device-specific
    // optimized code.  Otherwise 'generic' versions are used.
    virtual void setRotation(uint8_t r);
    virtual void invertDisplay(bool i);

    // BASIC DRAW API
    // These MAY be overridden by the subclass to provide device-specific
    // optimized code.  Otherwise 'generic' versions are used.

    // It's good to implement those, even if using transaction API
    // virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    // virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    // virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    virtual void fillScreen(uint16_t color);
    // Optional and probably not necessary to change
    // virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    // virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  protected:
    uint8_t *_buffer; // w * h * 2 + (((w + 7) / 8) * ((h + 7) / 8))

    bool _userFrmBuff;
    uint16_t *_frameBuffer; // _buffer
    uint8_t *_pixelMap; // _buffer + w * h * 2
    uint32_t _pixelMapSize;

    lcd_dir_t _scrrenDir;
};
