#pragma once

#include <stdint.h>

#include "k210_kpu.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // NCHW
    typedef struct _k210_kpu_shape
    {
        int vaild;
        int num;
        int chn;
        int h;
        int w;
    } k210_kpu_shape_t;

    int maix_kpu_helper_probe_model_size(uint8_t *model_buffer, uint32_t buffer_size);
    int maix_kpu_helper_get_input_shape(kpu_model_context_t *ctx, k210_kpu_shape_t *shape);
    int maix_kpu_helper_get_output_shape(kpu_model_context_t *ctx, k210_kpu_shape_t *shape);
    int maix_kpu_helper_get_output_count(kpu_model_context_t *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
