#define DBG_TAG "HAL_GPIO"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210_bsp.h"
#include "k210-hal.h"

#define GPIOHS_FUNC_SIZE        (32)

struct gpiohs_func_info_t {
    uint64_t last_time;
    fpioa_function_t func;
    uint8_t pin;
};

static _lock_t _hal_gpio_lock = 0;
static struct gpiohs_func_info_t gpiohs_func_table[ GPIOHS_FUNC_SIZE ];

int initPins(void)
{
    for(int i = 0; i < ( GPIOHS_FUNC_SIZE ); i++) {
        gpiohs_func_table[i].pin = 255;
        gpiohs_func_table[i].func = FUNC_GPIOHS0 + i;
        gpiohs_func_table[i].last_time = 0;
    }
    return 0;
}

static int comp_gpiohs_func_tab(const void *arg1, const void *arg2)
{
    struct gpiohs_func_info_t *i1 = (struct gpiohs_func_info_t *) arg1 ;
    struct gpiohs_func_info_t *i2 = (struct gpiohs_func_info_t *) arg2 ;

    return i1->last_time > i2->last_time ? 1 : -1;
}

fpioa_function_t hal_gpiohs_set_pin_func(uint8_t pinNumber)
{
    if(K210_IO_NUMS <= pinNumber) {
        return FUNC_MAX;
    }

    _lock_acquire(&_hal_gpio_lock);

    qsort(gpiohs_func_table, ( GPIOHS_FUNC_SIZE ), sizeof(struct gpiohs_func_info_t), comp_gpiohs_func_tab);

    if(255 != gpiohs_func_table[0].pin) {
        LOG_W("Rebind func %d from pin %d to %d\n", gpiohs_func_table[0].func, gpiohs_func_table[0].pin, pinNumber);
    }

    gpiohs_func_table[0].pin = pinNumber;
    gpiohs_func_table[0].last_time = sysctl_get_time_us();

    hal_fpioa_set_pin_func(pinNumber, gpiohs_func_table[0].func);

    _lock_release(&_hal_gpio_lock);

    return gpiohs_func_table[0].func;
}

fpioa_function_t hal_gpiohs_get_pin_func(uint8_t pinNumber)
{
    _lock_acquire(&_hal_gpio_lock);

    for(int i = 0; i < ( GPIOHS_FUNC_SIZE ); i++) {
        if(pinNumber == gpiohs_func_table[i].pin) {
            gpiohs_func_table[i].last_time = sysctl_get_time_us();

            _lock_release(&_hal_gpio_lock);

            return gpiohs_func_table[i].func;
        }
    }
    _lock_release(&_hal_gpio_lock);

    return FUNC_MAX;
}

void hal_gpiohs_clr_pin_func(uint8_t pinNumber)
{
    _lock_acquire(&_hal_gpio_lock);

    for(int i = 0; i < ( GPIOHS_FUNC_SIZE ); i++) {
        if(pinNumber == gpiohs_func_table[i].pin) {
            hal_fpioa_clr_pin_func(pinNumber);

            gpiohs_func_table[i].pin = 255;
            gpiohs_func_table[i].last_time = 0;

            _lock_release(&_hal_gpio_lock);

            return;
        }
    }

    _lock_release(&_hal_gpio_lock);
}
