#define DBG_TAG "DVP"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "BusDVP.h"

#include "pins_arduino.h"

namespace K210 {

/* static */ _lock_t BusDVP::_lock = 0;

/* static */ bool BusDVP::_begined = false;
/* static */ int BusDVP::_imgWidth = 0, BusDVP::_imgHeight = 0;
/* static */ bool BusDVP::_inner_buff_flag = false;
/* static */ camera_buffers_t BusDVP::_buff = {NULL, {NULL, NULL, NULL}};
/* static */ camera_pins_t BusDVP::_pins = {-1, -1, -1, -1, -1, -1};;

static volatile int _irq_done_flag = -1;

static int _bus_dvp_irq(void *ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        _irq_done_flag = 1;
    }
    else
    {
        if (0x00 == _irq_done_flag)
        {
            dvp_start_convert();
        }
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

/* static */ int BusDVP::begin(int width, int height, camera_buffers_t *buff)
{
    camera_pins_t pin = {
        .pclk = CAMERA_PCLK_PIN,
        .xclk = CAMERA_XCLK_PIN,
        .hsync = CAMERA_HSYNC_PIN,
        .vsync = CAMERA_VSYNC_PIN,
        .rst = CAMERA_RST_PIN,
        .pwdn = CAMERA_PWDN_PIN,
    };

    return begin(pin, 24, width, height, buff);
}

/* static */ int BusDVP::begin(camera_pins_t &pins, uint32_t xclkFreqMhz, int width, int height, camera_buffers_t *buff)
{
#define CHK_PINS(x) do { if(0 > x) return -1; } while(0)

    CHK_PINS(pins.pclk);
    CHK_PINS(pins.xclk);
    CHK_PINS(pins.hsync);
    CHK_PINS(pins.vsync);
    CHK_PINS(pins.rst);
    CHK_PINS(pins.pwdn);

    if ((0x00 == width) || (0x00 == height) || (true == _begined))
    {
        return -1;
    }

    lock_lock(&_lock);

    _pins = pins;
    _imgWidth = width;
    _imgHeight = height;

    if(NULL != buff)
    {
        _inner_buff_flag = false;

        _buff.disply = buff->disply;
        _buff.ai.r = buff->ai.r;
        _buff.ai.g = buff->ai.g;
        _buff.ai.b = buff->ai.b;
    }
    else
    {
        _inner_buff_flag = true;

        _buff.disply = (uint8_t *)rt_malloc_align(_imgWidth * _imgHeight * 5 /* 2: display; 3: ai */, 8);
        if(NULL == _buff.disply)
        {
            LOG_E("malloc dvp memory failed.");
            return -1;
        }

        _buff.ai.r = _buff.disply + _imgWidth * _imgHeight * 2;
        _buff.ai.g = _buff.ai.r + _imgWidth * _imgHeight * 1;
        _buff.ai.b = _buff.ai.g + _imgWidth * _imgHeight * 1;
    }

    // init pins
    hal_fpioa_set_pin_func(_pins.pclk,   FUNC_CMOS_PCLK);
    hal_fpioa_set_pin_func(_pins.xclk,   FUNC_CMOS_XCLK);
    hal_fpioa_set_pin_func(_pins.hsync,  FUNC_CMOS_HREF);
    hal_fpioa_set_pin_func(_pins.vsync,  FUNC_CMOS_VSYNC);

    hal_fpioa_set_pin_func(_pins.rst,    FUNC_CMOS_RST);
    hal_fpioa_set_pin_func(_pins.pwdn,   FUNC_CMOS_PWDN);

    sysctl_set_spi0_dvp_data(1);

    dvp_init();
    dvp_set_xclk_rate(xclkFreqMhz * 1000 * 1000);
    reset(); // ctrl rest and pwdn

    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    plic_irq_disable(IRQN_DVP_INTERRUPT);
    plic_irq_unregister(IRQN_DVP_INTERRUPT);

    dvp_enable_burst();
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(_imgWidth, _imgHeight);
    dvp_set_output_enable(DVP_OUTPUT_DISPLAY, 1);
    dvp_set_output_enable(DVP_OUTPUT_AI, 1);
    dvp_set_display_addr(_buff.disply);
    dvp_set_ai_addr(_buff.ai.r, _buff.ai.g, _buff.ai.b);
	dvp_disable_auto();

    plic_set_priority(IRQN_DVP_INTERRUPT, 2);
    plic_irq_register(IRQN_DVP_INTERRUPT, _bus_dvp_irq, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

    _begined = true;
    _irq_done_flag = 1;

    lock_unlock(&_lock);

    return 0;

#undef CHK_PINS
}

/* static */ void BusDVP::end()
{
    lock_lock(&_lock);

    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    plic_irq_disable(IRQN_DVP_INTERRUPT);
    plic_irq_unregister(IRQN_DVP_INTERRUPT);

    sysctl_clock_disable(SYSCTL_CLOCK_DVP);

    hal_fpioa_clr_pin_func(_pins.pclk);
    hal_fpioa_clr_pin_func(_pins.xclk);
    hal_fpioa_clr_pin_func(_pins.hsync);
    hal_fpioa_clr_pin_func(_pins.vsync);
    hal_fpioa_clr_pin_func(_pins.rst);
    hal_fpioa_clr_pin_func(_pins.pwdn);

    _pins.pclk = -1;
    _pins.xclk = -1;
    _pins.hsync = -1;
    _pins.vsync = -1;
    _pins.rst = -1;
    _pins.pwdn = -1;

    _begined = false;

    _imgWidth = 0;
    _imgHeight = 0;

    if(_inner_buff_flag)
    {
        rt_free_align(_buff.disply);
        rt_free_align(_buff.disply);
    }
    _inner_buff_flag = false;
    memset(&_buff, 0, sizeof(_buff));

    _irq_done_flag = 1;

    lock_unlock(&_lock);
}

/* static */ int BusDVP::capture(uint32_t timeout_ms)
{
    uint64_t start = sysctl_get_time_us();
    uint64_t end = start + timeout_ms * 1000;

    lock_lock(&_lock);

    _irq_done_flag = 0;

    while (0x00 == _irq_done_flag)
    {
        if (sysctl_get_time_us() >= end)
        {
            lock_unlock(&_lock);
            return -1;
        }
    }

    convert_display_buffer_order();

    lock_unlock(&_lock);

    return 0;
}

/* static */ void BusDVP::convert_display_buffer_order(void)
{
    // 760 us at 320*240 @ 600M
    //1130 us at 320*240 @ 400M
    uint16_t t;
    uint16_t *img = (uint16_t *)_buff.disply;
    uint16_t *end = img + _imgWidth * _imgHeight;

    while(img != end) {
        t = img[0];
        img[0] = img[1];
        img[1] = t;
        img += 2;
    }
}

/* static */ void BusDVP::reset(void)
{
    set_pwdn(0);

    set_rest(0);
    delay(50);
    set_rest(1);
    delay(50);
}

/* static */ void BusDVP::set_rest(int val)
{
    dvp_set_rest(val);
}

/* static */ void BusDVP::set_pwdn(int val)
{
    dvp_set_pwdn(val);
}

} // namespace K210
