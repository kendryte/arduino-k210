#pragma once

#include "Arduino.h"

#include "Wire.h"
#include "ST7789V.h"

#include "ts_lib/ts_lib.h"

class TSPoint
{
public:
    TSPoint(void);
    TSPoint(int x0, int y0, enum event_type type0);

    int x, y;
    enum event_type type;
};

class TouchScreen
{
public:
    TouchScreen(TwoWire *wire, uint16_t dev_addr);
    ~TouchScreen();

    TSPoint poll();

    virtual int ts_read(int cmd, int *val);
    virtual int ts_write(int cmd, int *val);

    virtual int do_tscal(ST7789V &lcd, int result[7]);

    virtual int _poll() = 0;

protected:
    TwoWire *_wire;
    uint16_t _dev_addr;
    struct ts_pdata_t ts_pdata;

private:
    virtual int ts_set_calibration(enum ts_ioctl_type cmd, int cal[7]);
};
