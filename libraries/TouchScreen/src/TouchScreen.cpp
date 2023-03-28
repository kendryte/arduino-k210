#include "TouchScreen.h"

TSPoint::TSPoint(void) { x = y = 0; type = TOUCH_NONE; }

TSPoint::TSPoint(int x0, int y0, enum event_type type0)
{
    x = x0;
    y = y0;
    type = type0;
}

TouchScreen::TouchScreen(TwoWire *wire, uint16_t dev_addr)
    : _wire(wire), _dev_addr(dev_addr)
{
}

TouchScreen::~TouchScreen()
{
    _wire = NULL;
}

int TouchScreen::ts_read(int cmd, int *val)
{
    uint8_t buf[2];

    if (_wire)
    {
        _wire->beginTransmission(_dev_addr);
        _wire->write(static_cast<uint8_t>(cmd));
        _wire->endTransmission(false);

        if (0x02 != _wire->requestFrom(_dev_addr, size_t(2)))
        {
            return -1;
        }

        buf[0] = _wire->read();
        buf[1] = _wire->read();

        if(val)
        {
            *val = (buf[0] << 4) | (buf[1] >> 4);
        }

        return 0;
    }

    return -1;
}

int TouchScreen::ts_write(int cmd, int *val)
{
    return -1;
}

TSPoint TouchScreen::poll()
{
    _poll();

    return TSPoint(ts_pdata.event->x, ts_pdata.event->y, ts_pdata.event->type);
}

int TouchScreen::ts_set_calibration(enum ts_ioctl_type cmd, int cal[7])
{
    int _cal[7];

    if (cmd == TS_IOCTL_SET_CALBRATION)
    {
        if (!cal)
        {
            return -1;
        }

        memcpy(_cal, cal, sizeof(int) * 7);

        tsfilter_setcal(ts_pdata.filter, _cal);

        return 0;
    }

    return -1;
}

static void lcd_draw_cross(ST7789V &lcd, int x, int y, uint16_t color)
{
    lcd.drawLine(x - 12, y, x + 13, y, color); //横线
    lcd.drawLine(x, y - 12, x, y + 13, color); //竖线

    lcd.drawPixel(x + 1, y + 1, color);
    lcd.drawPixel(x - 1, y + 1, color);
    lcd.drawPixel(x + 1, y - 1, color);
    lcd.drawPixel(x - 1, y - 1, color);

    lcd.drawCircle(x, y, 6, color); //画中心圈
}

int TouchScreen::do_tscal(ST7789V &lcd, int result[7])
{
    int index = 0;

    struct tscal_t cal;

    result[0] = 1;
    result[1] = 0;
    result[2] = 0;
    result[3] = 0;
    result[4] = 1;
    result[5] = 0;
    result[6] = 1;

    ts_set_calibration(TS_IOCTL_SET_CALBRATION, result);

    cal.xfb[0] = 50;
    cal.yfb[0] = 50;

    cal.xfb[1] = lcd.width() - 50;
    cal.yfb[1] = 50;

    cal.xfb[2] = lcd.width() - 50;
    cal.yfb[2] = lcd.height() - 50;

    cal.xfb[3] = 50;
    cal.yfb[3] = lcd.height() - 50;

    cal.xfb[4] = lcd.width() / 2;
    cal.yfb[4] = lcd.height() / 2;

    index = 0;
    lcd.fillScreen(0x00);
    lcd_draw_cross(lcd, cal.xfb[index], cal.yfb[index], 0xF800);

    while (1)
    {
        _poll();

        if (ts_pdata.event->type == TOUCH_END)
        {
            cal.x[index] = ts_pdata.event->x;
            cal.y[index] = ts_pdata.event->y;

            if (++index >= 5)
            {
                if (perform_calibration(&cal))
                {
                    ts_set_calibration(TS_IOCTL_SET_CALBRATION, &cal.a[0]);
                }

                lcd.fillScreen(0x00);

                memcpy(result, cal.a, sizeof(int) * 7);

                ts_pdata.event->type = TOUCH_NONE;
                break;
            }

            lcd.fillScreen(0x00);
            lcd_draw_cross(lcd, cal.xfb[index], cal.yfb[index], 0xF800);
        }

        ts_pdata.event->type = TOUCH_NONE;
    }

    return 0;
}
