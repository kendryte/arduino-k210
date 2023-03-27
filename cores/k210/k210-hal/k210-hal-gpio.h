#pragma once

#include <stdint.h>

#include "k210_fpioa.h"

#ifdef __cplusplus
extern "C" {
#endif

fpioa_function_t hal_gpiohs_set_pin_func(uint8_t pinNumber);

fpioa_function_t hal_gpiohs_get_pin_func(uint8_t pinNumber);

void hal_gpiohs_clr_pin_func(uint8_t pinNumber);

#ifdef __cplusplus
}
#endif
