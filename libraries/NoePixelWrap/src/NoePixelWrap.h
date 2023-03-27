#pragma once

#include <Adafruit_NeoPixel.h>

#include "k210-hal.h"
#include "rtthread.h"

class Adafruit_NeoPixel_Warp : public Adafruit_NeoPixel
{
public:
    Adafruit_NeoPixel_Warp(uint16_t n, int16_t pin = 6, neoPixelType type = NEO_GRB + NEO_KHZ800);
    Adafruit_NeoPixel_Warp(void);
    void show(void);

private:
    void _k210Show(uint8_t pin, uint8_t *pixels, uint32_t numBytes, boolean is800KHz);
};
