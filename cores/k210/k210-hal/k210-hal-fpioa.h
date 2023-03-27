#pragma once

#include <stdint.h>

#include "k210_fpioa.h"

#define K210_IO_NUMS    (48)

#ifdef __cplusplus
extern "C" {
#endif

void hal_fpioa_init(void);

void hal_fpioa_set_pin_func(uint8_t pin, fpioa_function_t func);

fpioa_function_t hal_fpioa_get_pin_func(uint8_t pin);

void hal_fpioa_clr_pin_func(uint8_t pin);

#ifdef __cplusplus
}
#endif
