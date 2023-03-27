#define DBG_TAG "BUS8080"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "Bus8080.h"

namespace K210 {

lcd_pins_t Bus8080::_pins = {-1, -1, -1, -1};

void Bus8080::begin(uint32_t freq)
{
    lcd_pins_t pin = {
        .clk = TFT_CLK_PIN,
        .cs = TFT_CSX_PIN,
        .dc = TFT_DCX_PIN,
        .rst = TFT_RST_PIN,
    };
    begin(pin, freq);
}

void Bus8080::begin(lcd_pins_t &pins, uint32_t freq)
{
    _pins = pins;

    hal_tft_begin(_pins.clk, _pins.cs, _pins.dc, freq);
}

void Bus8080::end(void)
{
    hal_tft_end();
}

void Bus8080::reset(PinStatus valid, int rst_ms)
{
    if(0 <= _pins.rst) {
        pinMode(_pins.rst, OUTPUT);

        digitalWrite(_pins.rst, (HIGH == valid) ? LOW : HIGH);
        delay(rst_ms);
        digitalWrite(_pins.rst, (HIGH == valid) ? HIGH : LOW);

        pinModeClear(_pins.rst);
    }
}

void Bus8080::writeCommand(uint8_t command)
{
    uint8_t c[1];

    c[0] = command;

    hal_tft_write_command(c, 1);
}

void Bus8080::writeCommand(uint16_t command)
{
    uint8_t c[2];

    c[0] = highByte(command);
    c[1] = lowByte(command);

    hal_tft_write_command(c, 2);
}

void Bus8080::writeData(uint8_t data)
{
    uint8_t d[1];

    d[0] = data;

    hal_tft_write_byte(d, 1);
}

void Bus8080::writeData(uint8_t *data, uint32_t len)
{
    hal_tft_write_byte(data, len);
}

static void cvt_u16_to_u32(uint16_t *data, uint32_t len)
{
    uint16_t t;
    uint16_t *img = data;
    uint16_t *end = img + len;

    while(img != end) {
        t = img[0];
        img[0] = img[1];
        img[1] = t;
        img += 2;
    }
}

void Bus8080::writeData(uint16_t *data, uint32_t len)
{
    // hal_tft_write_half(data, len);

    cvt_u16_to_u32(data, len);
    hal_tft_write_word((uint32_t *)data, len / 2);
}

void Bus8080::writeData(uint32_t *data, uint32_t len)
{
    hal_tft_write_word(data, len);
}

void Bus8080::fillScreen(uint16_t color, int16_t w, int16_t h)
{
    uint32_t c = ((uint32_t)color << 16) | (uint32_t)color;

    hal_tft_fill_data(&c, w * h);
}

} // namespace K210

