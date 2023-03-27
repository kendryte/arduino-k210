#pragma once

#include "Arduino.h"

#include "Wire.h"
#include "Image.h"
namespace K210 {

typedef enum {
    PIXFORMAT_INVLAID   = -1,
    PIXFORMAT_RGB565    = DVP_CFG_RGB_FORMAT,    // 2BPP/RGB565
    PIXFORMAT_YUV422    = DVP_CFG_YUV_FORMAT,    // 2BPP/YUV422
} pixformat_t;

typedef enum {
    FRAMESIZE_INVALID = 0,
    // C/SIF Resolutions
    FRAMESIZE_QQCIF,    // 88x72
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_CIF,      // 352x288
    FRAMESIZE_QQSIF,    // 88x60
    FRAMESIZE_QSIF,     // 176x120
    FRAMESIZE_SIF,      // 352x240
    // VGA Resolutions
    FRAMESIZE_QQQQVGA,  // 40x30
    FRAMESIZE_QQQVGA,   // 80x60
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_HQQQVGA,  // 60x40
    FRAMESIZE_HQQVGA,   // 120x80
    FRAMESIZE_HQVGA,    // 240x160
    // FFT Resolutions
    FRAMESIZE_64X32,    // 64x32
    FRAMESIZE_64X64,    // 64x64
    FRAMESIZE_128X64,   // 128x64
    FRAMESIZE_128X128,  // 128x128
    FRAMESIZE_240X240,  // 240x240
    // Other
    FRAMESIZE_LCD,      // 128x160
    FRAMESIZE_QQVGA2,   // 128x160
    FRAMESIZE_WVGA,     // 720x480
    FRAMESIZE_WVGA2,    // 752x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
} framesize_t;

typedef enum {
    GAINCEILING_2X,
    GAINCEILING_4X,
    GAINCEILING_8X,
    GAINCEILING_16X,
    GAINCEILING_32X,
    GAINCEILING_64X,
    GAINCEILING_128X,
} gainceiling_t;

const int _camera_resolution[][2] = {
    {0, 0},
    // C/SIF Resolutions
    {88, 72},   /* QQCIF     */
    {176, 144}, /* QCIF      */
    {352, 288}, /* CIF       */
    {88, 60},   /* QQSIF     */
    {176, 120}, /* QSIF      */
    {352, 240}, /* SIF       */
    // VGA Resolutions
    {40, 30},   /* QQQQVGA   */
    {80, 60},   /* QQQVGA    */
    {160, 120}, /* QQVGA     */
    {320, 240}, /* QVGA      */
    {640, 480}, /* VGA       */
    {60, 40},   /* HQQQVGA   */
    {120, 80},  /* HQQVGA    */
    {240, 160}, /* HQVGA     */
    // FFT Resolutions
    {64, 32},   /* 64x32     */
    {64, 64},   /* 64x64     */
    {128, 64},  /* 128x64    */
    {128, 128}, /* 128x128    */
    {240, 240}, /* 240x240    */
    // Other
    {128, 160},   /* LCD       */
    {128, 160},   /* QQVGA2    */
    {720, 480},   /* WVGA      */
    {752, 480},   /* WVGA2     */
    {800, 600},   /* SVGA      */
    {1280, 1024}, /* SXGA      */
    {1600, 1200}, /* UXGA      */
};

typedef struct image {
    uint32_t w, h, bpp;

    struct {
        uint8_t *ai;
        uint8_t *display;
    } buffer;
} image_t;

typedef struct _camera_buffers {
    uint8_t *disply;
    struct {
        uint8_t *r;
        uint8_t *g;
        uint8_t *b;
    } ai;
} camera_buffers_t;

typedef struct _camera_pins
{
    int8_t pclk;
    int8_t xclk;
    int8_t hsync;
    int8_t vsync;

    int8_t rst;
    int8_t pwdn;
} camera_pins_t;

class Camera
{
protected:
    TwoWire *_wire;

    int _i2cNum;
    uint16_t _slvAddr;

    int _imgWidth, _imgHeight;
    bool _sensorVflip, _sensorHmirror;
public:
    Camera(int i2cNum, uint16_t slvAddr);

    ~Camera(void);

    int begin(int width, int height, camera_buffers_t *buff);
    int begin(camera_pins_t &pins, uint32_t xclkFreqMhz, int width, int height, camera_buffers_t *buff);
    void end();

    void get_buffers(camera_buffers_t *buff);

    virtual int         read_reg            (uint8_t reg_addr, uint8_t *reg_data);
    virtual int         read_reg            (uint16_t reg_addr, uint16_t *reg_data);
    virtual int         read_reg            (int reg_addr, int *reg_data);
    virtual int         write_reg           (uint8_t reg_addr, uint8_t reg_data);
    virtual int         write_reg           (uint16_t reg_addr, uint16_t reg_data);
    virtual int         write_reg           (int reg_addr, int reg_data);
    virtual int         snapshot            (uint32_t timeout_ms = 200);

    virtual int         width               (void);
    virtual int         height              (void);
    virtual bool        flip                (void);
    virtual bool        mirror              (void);

    virtual int         reset               (framesize_t framesize, camera_buffers_t *buff) = 0;
    virtual int         read_id             (void) = 0;
    virtual int         set_hmirror         (int enable) = 0;
    virtual int         set_vflip           (int enable) = 0;

    virtual int         set_windowing       (framesize_t framesize, int x, int y, int w, int h) = 0;
    virtual int         set_contrast        (int level) = 0;
    virtual int         set_brightness      (int level) = 0;
    virtual int         set_saturation      (int level) = 0;
    virtual int         set_gainceiling     (gainceiling_t gainceiling) = 0;
    virtual int         set_colorbar        (int enable) = 0;
    virtual int         set_auto_gain       (int enable, float gain_db, float gain_db_ceiling) = 0;
    virtual int         get_gain_db         (float *gain_db) = 0;
    virtual int         set_auto_exposure   (int enable, int exposure_us) = 0;
    virtual int         get_exposure_us     (int *exposure_us) = 0;
    virtual int         set_auto_whitebal   (int enable, uint8_t r_gain_db, uint8_t g_gain_db, uint8_t b_gain_db) = 0;
    virtual int         get_rgb_gain_db     (uint8_t *r_gain_db, uint8_t *g_gain_db,uint8_t *b_gain_db) = 0;

    // virtual int         set_framesize       (framesize_t framesize) = 0;
    // virtual int      set_framerate       (framerate_t framerate) = 0;
    // virtual int      set_quality         (int quality) = 0;
    // virtual int      set_pixformat       (pixformat_t pixformat) = 0;
    // virtual int      set_special_effect  (sde_t sde) = 0;
    // virtual int      set_lens_correction (int enable, int radi, int coef) = 0;
};

}
