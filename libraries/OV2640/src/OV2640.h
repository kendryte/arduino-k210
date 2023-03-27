#pragma once

#include "Arduino.h"

#include "Camera.h"

using namespace K210;

class OV2640 : public Camera
{
public:
    OV2640(int8_t sda = CAMERA_SDA_PIN, int8_t scl = CAMERA_SCL_PIN, int i2cNum = 2);

    ~OV2640(void);

    virtual int         reset               (framesize_t framesize = FRAMESIZE_QVGA, camera_buffers_t *buff = NULL);
    virtual int         read_id             (void);

    virtual int         set_hmirror         (int enable);
    virtual int         set_vflip           (int enable);

    virtual int         set_windowing       (framesize_t framesize, int x, int y, int w, int h);
    virtual int         set_contrast        (int level);
    virtual int         set_brightness      (int level);
    virtual int         set_saturation      (int level);
    virtual int         set_gainceiling     (gainceiling_t gainceiling);
    virtual int         set_colorbar        (int enable);
    virtual int         set_auto_gain       (int enable, float gain_db, float gain_db_ceiling);
    virtual int         get_gain_db         (float *gain_db);
    virtual int         set_auto_exposure   (int enable, int exposure_us);
    virtual int         get_exposure_us     (int *exposure_us);
    virtual int         set_auto_whitebal   (int enable, uint8_t r_gain_db, uint8_t g_gain_db, uint8_t b_gain_db);
    virtual int         get_rgb_gain_db     (uint8_t *r_gain_db, uint8_t *g_gain_db, uint8_t *b_gain_db);
private:
    int8_t _sdaPin, _sclPin;

    int set_framesize(framesize_t framesize);
};
