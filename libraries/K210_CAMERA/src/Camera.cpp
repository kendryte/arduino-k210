#define DBG_TAG "Camera"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "Camera.h"
#include "BusDVP.h"

namespace K210 {

Camera::Camera(int i2cNum, uint16_t slvAddr)
:_i2cNum(i2cNum)
,_slvAddr(slvAddr)
,_imgWidth(0)
,_imgHeight(0)
,_sensorVflip(false)
,_sensorHmirror(false)
{
    _wire = new TwoWire(_i2cNum);
    if(NULL == _wire)
    {
        LOG_E("new wire failed\n");
    }
}

Camera::~Camera(void)
{
    end();

    if(_wire)
    {
        _wire->end();

        delete _wire;
        _wire = NULL;
    }
}

int Camera::begin(int width, int height, camera_buffers_t *buff)
{
    return BusDVP::begin(width, height, buff);
}

int Camera::begin(camera_pins_t &pins, uint32_t xclkFreqMhz, int width, int height, camera_buffers_t *buff)
{
    return BusDVP::begin(pins, xclkFreqMhz, width, height, buff);
}

void Camera::end()
{
    BusDVP::end();
}

void Camera::get_buffers(camera_buffers_t *buff)
{
    BusDVP::get_buffers(buff);
}

int Camera::read_reg(uint8_t reg_addr, uint8_t *reg_data)
{
    _wire->beginTransmission(_slvAddr);
    _wire->write(reg_addr);
    _wire->endTransmission(false);
    size_t rx = _wire->requestFrom(_slvAddr, size_t(1));

    if(0x01 != rx)
    {
        return -1;
    }

    *reg_data = _wire->read();

    return 0;
}

int Camera::write_reg(uint8_t reg_addr, uint8_t reg_data)
{
    uint8_t stat = 0;

    _wire->beginTransmission(_slvAddr);
    _wire->write(reg_addr);
    _wire->write(reg_data);
    stat = _wire->endTransmission(true);

    if(0x00 != stat)
    {
        rt_kprintf("Camera w_reg failed at %02X->%02X with ret %d\n", reg_addr, reg_data, stat);
    }

    return (0x00 == stat) ? 0: -1;
}

int Camera::read_reg(uint16_t reg_addr, uint16_t *reg_data)
{
    return read_reg(static_cast<uint8_t>(reg_addr), reinterpret_cast<uint8_t*>(reg_data));
}

int Camera::read_reg(int reg_addr, int *reg_data)
{
    return read_reg(static_cast<uint16_t>(reg_addr), reinterpret_cast<uint16_t*>(reg_data));
}

int Camera::write_reg(uint16_t reg_addr, uint16_t reg_data)
{
    return write_reg(static_cast<uint8_t>(reg_addr), static_cast<uint8_t>(reg_data));
}

int Camera::write_reg(int reg_addr, int reg_data)
{
    return write_reg(static_cast<uint16_t>(reg_addr), static_cast<uint16_t>(reg_data));
}

int Camera::snapshot(uint32_t timeout_ms)
{
    return BusDVP::capture(timeout_ms);
}

int Camera::width(void)
{
    return _imgWidth;
}

int Camera::height(void)
{
    return _imgHeight;
}

bool Camera::flip(void)
{
    return _sensorVflip;
}

bool Camera::mirror(void)
{
    return _sensorHmirror;
}

}
