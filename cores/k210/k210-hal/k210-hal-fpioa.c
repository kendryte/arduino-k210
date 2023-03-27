#define DBG_TAG "HAL_FPIOA"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"

static fpioa_function_t _k210_pin_func_map[ K210_IO_NUMS ];

void hal_fpioa_init(void)
{
    for(int i = 0; i < ( K210_IO_NUMS ); i++) {
        _k210_pin_func_map[i] = FUNC_MAX;
    }
}

void hal_fpioa_set_pin_func(uint8_t pin, fpioa_function_t func)
{
    if( K210_IO_NUMS > pin) {
        if((FUNC_MAX != _k210_pin_func_map[pin]) && (func != _k210_pin_func_map[pin])) {
            LOG_W("Set Pin(%d) func From %d to %d\n", pin, _k210_pin_func_map[pin], func);
        }

        _k210_pin_func_map[pin] = func;

        fpioa_set_function(pin, func);
    }
}

fpioa_function_t hal_fpioa_get_pin_func(uint8_t pin)
{
    if( K210_IO_NUMS > pin) {
        return _k210_pin_func_map[pin];
    }
    return FUNC_MAX;
}

void hal_fpioa_clr_pin_func(uint8_t pin)
{
    if( K210_IO_NUMS > pin) {
        _k210_pin_func_map[pin] = FUNC_MAX;
    }
}
