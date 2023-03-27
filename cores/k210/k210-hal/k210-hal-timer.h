#pragma once

#include <stdint.h>

#include "k210_timer.h"
#include "k210_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

union HAL_TIMER_DEV {
    timer_device_number_t tim;
    pwm_device_number_t pwm;
};

union HAL_TIMER_CHN {
    timer_channel_number_t tim;
    pwm_channel_number_t pwm;
};

struct timer_dev_chn_map_t {
    int8_t pin;

    const union HAL_TIMER_DEV dev;
    const union HAL_TIMER_CHN chn;
};

struct timer_dev_chn_map_t *hal_timer_set_pin_func(int8_t pin);

void hal_timer_clr_pin_func(int8_t pin);

struct timer_dev_chn_map_t *hal_timer_get_free_chn(void);
void hal_timer_release_chn(struct timer_dev_chn_map_t * map);

#ifdef __cplusplus
}
#endif
