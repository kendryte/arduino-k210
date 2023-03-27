#define DBG_TAG "HAL_TIMER"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"

#define K210_TIMER_DEC_CHN_NUMS     (12)

static struct timer_dev_chn_map_t timer_dev_chn_map[ K210_TIMER_DEC_CHN_NUMS ] = {
    {-1, TIMER_DEVICE_0, TIMER_CHANNEL_0},
    {-1, TIMER_DEVICE_0, TIMER_CHANNEL_1},
    {-1, TIMER_DEVICE_0, TIMER_CHANNEL_2},
    {-1, TIMER_DEVICE_0, TIMER_CHANNEL_3},

    {-1, TIMER_DEVICE_1, TIMER_CHANNEL_0},
    {-1, TIMER_DEVICE_1, TIMER_CHANNEL_1},
    {-1, TIMER_DEVICE_1, TIMER_CHANNEL_2},
    {-1, TIMER_DEVICE_1, TIMER_CHANNEL_3},

    {-1, TIMER_DEVICE_2, TIMER_CHANNEL_0},
    {-1, TIMER_DEVICE_2, TIMER_CHANNEL_1},
    {-1, TIMER_DEVICE_2, TIMER_CHANNEL_2},
    {-1, TIMER_DEVICE_2, TIMER_CHANNEL_3},
};

static int get_free_timer_chn(int8_t pin)
{
    // first check if already assigned.
    for(int i = 0; i < ( K210_TIMER_DEC_CHN_NUMS ); i++) {
        if(pin == timer_dev_chn_map[i].pin) {
            return i;
        }
    }

    for(int i = 0; i < ( K210_TIMER_DEC_CHN_NUMS ); i++) {
        if((-1) == timer_dev_chn_map[i].pin) {
            return i;
        }
    }

    return -1;
}

struct timer_dev_chn_map_t *hal_timer_set_pin_func(int8_t pin)
{
    int chn = get_free_timer_chn(pin);
    if((-1) != chn) {
        timer_dev_chn_map[chn].pin = pin;
        hal_fpioa_set_pin_func(pin, (fpioa_function_t)(FUNC_TIMER0_TOGGLE1 + chn));
        return &timer_dev_chn_map[chn];
    }
    return NULL;
}

void hal_timer_clr_pin_func(int8_t pin)
{
    for(int i = 0; i < ( K210_TIMER_DEC_CHN_NUMS ); i++) {
        if(pin == timer_dev_chn_map[i].pin) {
            hal_fpioa_clr_pin_func(pin);

            timer_dev_chn_map[i].pin = -1;
        }
    }
}

/*
    0 - Pin
    1 - Sts
*/
static int8_t _pin_place_holder[K210_TIMER_DEC_CHN_NUMS][2] = {
    {42, 0},
    {43, 0},
    {44, 0},
    {45, 0},

    {46, 0},
    {47, 0},
    {48, 0},
    {49, 0},

    {50, 0},
    {51, 0},
    {52, 0},
    {53, 0},
};

struct timer_dev_chn_map_t *hal_timer_get_free_chn(void)
{
    int chn = -1, pin_idx = -1;
    int8_t pin = 127;
    
    for(int i = 0; i < K210_TIMER_DEC_CHN_NUMS; i++) {
        if(0x00 == _pin_place_holder[i][1]) {
            pin_idx = i;
            pin = _pin_place_holder[i][0];
            break;
        }
    }

    chn = get_free_timer_chn(pin);
    if(((-1) != chn) && ((-1) != pin_idx) && (127 != pin)) {
        _pin_place_holder[pin_idx][1] = 1;
        timer_dev_chn_map[chn].pin = pin;
        return &timer_dev_chn_map[chn];
    }

    return NULL;
}

void hal_timer_release_chn(struct timer_dev_chn_map_t * map)
{
    for(int i = 0; i < ( K210_TIMER_DEC_CHN_NUMS ); i++) {
        if(map->pin == timer_dev_chn_map[i].pin) {
            timer_dev_chn_map[i].pin = -1;
            break;
        }
    }

    for(int i = 0; i < ( K210_TIMER_DEC_CHN_NUMS ); i++) {
        if(map->pin == _pin_place_holder[i][0]) {
            _pin_place_holder[i][1] = 0;
            break;
        }
    }
}
