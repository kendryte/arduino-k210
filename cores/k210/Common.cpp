#define DBG_TAG "Common"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "Common.h"

void randomSeed(unsigned long seed)
{
    (void)seed;

    prng_get();
}

long random(long howbig)
{
    uint32_t x = prng_get();
    uint64_t m = uint64_t(x) * uint64_t(howbig);
    uint32_t l = uint32_t(m);
    if (l < howbig) {
        uint32_t t = -howbig;
        if (t >= howbig) {
            t -= howbig;
            if (t >= howbig)
                t %= howbig;
        }
        while (l < t) {
            x = prng_get();
            m = uint64_t(x) * uint64_t(howbig);
            l = uint32_t(m);
        }
    }
    return m >> 32;
}

long random(long howsmall, long howbig)
{
    if(howsmall >= howbig) {
        return howsmall;
    }
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long run = in_max - in_min;
    if(run == 0){
        LOG_E("map(): Invalid input range, min == max");
        return -1; // AVR returns -1, SAM returns 0
    }
    const long rise = out_max - out_min;
    const long delta = x - in_min;
    return (delta * rise) / run + out_min;
}

uint16_t makeWord(uint16_t w)
{
  return w;
}

uint16_t makeWord(uint8_t h, uint8_t l)
{
  return (h << 8) | l;
}
