#include "NoePixelWrap.h"

Adafruit_NeoPixel_Warp::Adafruit_NeoPixel_Warp(uint16_t n, int16_t pin, neoPixelType type) : Adafruit_NeoPixel(n, pin, type)
{
}

Adafruit_NeoPixel_Warp::Adafruit_NeoPixel_Warp(void) : Adafruit_NeoPixel()
{
}

void Adafruit_NeoPixel_Warp::show(void)
{
    if (!pixels)
        return;
    while (!canShow())
        ;
    _k210Show(pin, pixels, numBytes, is800KHz);
    endTime = micros(); // Save EOD time for latch on next call
}

void Adafruit_NeoPixel_Warp::_k210Show(uint8_t pin, uint8_t *pixels, uint32_t numBytes, boolean is800KHz)
{

#define CYCLES_800_T0H (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 2500000) // 0.4us
#define CYCLES_800_T1H (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1250000) // 0.8us
#define CYCLES_800 (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 800000)      // 1.25us per bit
#define CYCLES_400_T0H (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 2000000) // 0.5uS
#define CYCLES_400_T1H (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 833333)  // 1.2us
#define CYCLES_400 (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 400000)      // 2.5us per bit

    uint8_t *p, *end, pix, mask;
    uint64_t t, time0, time1, period, c, startTime;

    p = pixels;
    end = p + numBytes;
    pix = *p++;
    mask = 0x80;
    startTime = 0;

#ifdef NEO_KHZ400
    if (is800KHz)
    {
#endif
        time0 = CYCLES_800_T0H;
        time1 = CYCLES_800_T1H;
        period = CYCLES_800;
#ifdef NEO_KHZ400
    }
    else
    { // 400 KHz bitstream
        time0 = CYCLES_400_T0H;
        time1 = CYCLES_400_T1H;
        period = CYCLES_400;
    }
#endif

    fpioa_function_t func = hal_gpiohs_get_pin_func(pin);

    noInterrupts();

    for (t = time0;; t = time0)
    {
        if (pix & mask)
            t = time1; // Bit high duration

        while (((c = read_cycle()) - startTime) < period)
            ;                                              // Wait for bit start
        gpiohs_set_pin(func - FUNC_GPIOHS0, GPIO_PV_HIGH); // digitalWrite(pin, HIGH);

        startTime = c; // Save start time
        while (((c = read_cycle()) - startTime) < t)
            ;                                             // Wait high duration
        gpiohs_set_pin(func - FUNC_GPIOHS0, GPIO_PV_LOW); // digitalWrite(pin, LOW);

        if (!(mask >>= 1))
        { // Next bit/byte
            if (p >= end)
                break;
            pix = *p++;
            mask = 0x80;
        }
    }
    while ((read_cycle() - startTime) < period)
        ; // Wait for last bit

    interrupts();
}
