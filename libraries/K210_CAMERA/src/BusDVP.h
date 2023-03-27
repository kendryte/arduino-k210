#pragma once

#include "Arduino.h"

#include "k210-hal.h"

#include "Camera.h"
#include "Image.h"
namespace K210 {

class BusDVP
{
public:
    static int begin(int width, int height, camera_buffers_t *buff);
    static int begin(camera_pins_t &pins, uint32_t xclkFreqMhz, int width, int height, camera_buffers_t *buff);
    static void end();

    static int capture(uint32_t timeout_ms = 200);

    static void get_buffers(camera_buffers_t *buff) {
        memcpy((uint8_t *)buff, (uint8_t *)&_buff, sizeof(camera_buffers_t));
    }

private:
    static void reset(void);
    static void set_rest(int val);
    static void set_pwdn(int val);

    static void convert_display_buffer_order(void);

    static _lock_t _lock;

    static bool _begined;

    static int _imgWidth, _imgHeight;

    static bool _inner_buff_flag;
    static camera_buffers_t _buff;

    static camera_pins_t _pins;
};

} // namespace K210
