#include "Arduino.h"

#include "rthw.h"

#include "k210_bsp.h"

void yield()
{
    rt_thread_yield();
}

unsigned long millis(void)
{
    return rt_tick_get_millisecond();
}

unsigned long micros(void)
{
    return rt_tick_get() / (1000u / RT_TICK_PER_SECOND);
}

void delay(unsigned long ms)
{
    rt_thread_mdelay(ms);
}

void delayMicroseconds(unsigned int us)
{
    unsigned int _ms = us / 1000;
    unsigned int _us = us % 1000;

    if(_ms)
        rt_thread_mdelay(_ms);

    if(_us)
        rt_hw_us_delay(us);
}

void initVariant() __attribute__((weak));
void initVariant() {}

void init() __attribute__((weak));
void init() {}

//used by hal log
const char * pathToFileName(const char * path)
{
    size_t i = 0;
    size_t pos = 0;
    char * p = (char *)path;
    while(*p){
        i++;
        if(*p == '/' || *p == '\\'){
            pos = i;
        }
        p++;
    }
    return path+pos;
}

uint32_t prng_get(void)
{
    static bool seeded = false;
    static uint32_t pad = 0, n = 0, d = 0;
    static uint8_t dat = 0;

    if (!seeded) {
        seeded = true;
        pad = (uint32_t)(read_cycle() & 0xFFFFFFFF);
        n = (uint32_t)(read_cycle() & 0x12345678);
        d = (uint32_t)(read_cycle() & 0x87654321);
    }

    pad += dat + d * n;
    pad = (pad << 3) + (pad >> 29);
    n = pad | 2;
    d ^= (pad << 31) + (pad >> 1);
    dat ^= (char)pad ^ (d >> 8) ^ 1;

    return pad ^ (d << 5) ^ (pad >> 18) ^ (dat << 1);
}
