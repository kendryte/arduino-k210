#include "kpu_helper.h"

#include "rtthread.h"

#if (0x00 == ONLY_KMODEL_V3)
#include "nncase.h"
#endif // ONLY_KMODEL_V3

int maix_kpu_helper_probe_model_size(uint8_t *model_buffer, uint32_t buffer_size)
{
    uint32_t body_size = 0;

    const kpu_kmodel_header_t *header = (const kpu_kmodel_header_t *)model_buffer;

    if (header->version == 3 && header->arch == 0)
    {
        kpu_model_context_t ctx;

        ctx.output_count = header->output_count;
        ctx.outputs = (const kpu_model_output_t *)(model_buffer + sizeof(kpu_kmodel_header_t));
        ctx.layer_headers = (const kpu_model_layer_header_t *)((uintptr_t)ctx.outputs + sizeof(kpu_model_output_t) * ctx.output_count);
        ctx.layers_length = header->layers_length;
        ctx.body_start = (const uint8_t *)((uintptr_t)ctx.layer_headers + sizeof(kpu_model_layer_header_t) * header->layers_length);

        body_size = (uint32_t)(ctx.body_start - (const uint8_t *)header);

        if(body_size > buffer_size)
        {
            return -1;
        }

        for(int i=0; i< ctx.layers_length; i++)
        {
            const kpu_model_layer_header_t *cnt_layer_header = ctx.layer_headers + i;
            body_size += cnt_layer_header->body_size;
        }

        return body_size;
    }
    else if(header->version == 'KMDL')
    {
#if ONLY_KMODEL_V3
        rt_kprintf("Only support Kmodel V3\n");
#else
        body_size = nncase_probe_model_buffer_size(model_buffer, buffer_size);

        return body_size;
#endif // ONLY_KMODEL_V3
    }

    return -1;
}

int maix_kpu_helper_get_input_shape(kpu_model_context_t *ctx, k210_kpu_shape_t *shape)
{
    if(!shape)
    {
        return -1;
    }

    shape->vaild = 0;

    if(ctx->is_nncase)
    {
#if ONLY_KMODEL_V3
        rt_kprintf("Only support Kmodel V3\n");
#else
        if(0x00 == nncase_get_input_shape(ctx, 0, &shape->chn, &shape->h, &shape->w))
        {
            shape->vaild = 1;
            return 0;
        }
#endif // ONLY_KMODEL_V3
        return -1;
    }
    else
    {
        const kpu_model_layer_header_t *first_layer_header = ctx->layer_headers;

        if(KL_K210_CONV == first_layer_header->type)
        {
            const kpu_model_conv_layer_argument_t *first_layer = (const kpu_model_conv_layer_argument_t *)ctx->body_start;
            kpu_layer_argument_t layer_arg = *(volatile kpu_layer_argument_t *)(ctx->model_buffer + first_layer->layer_offset);

            shape->chn = 1 + layer_arg.image_channel_num.data.i_ch_num;
            shape->h = 1 + layer_arg.image_size.data.i_col_high;
            shape->w = 1 + layer_arg.image_size.data.i_row_wid;
            shape->vaild = 1;

            return 0;
        }
        else if(KL_FULLY_CONNECTED == first_layer_header->type)
        {
            // TODO: get input shape.
            return -1;
        }
    }

    return -1;
}

int maix_kpu_helper_get_output_shape(kpu_model_context_t *ctx, k210_kpu_shape_t *shape)
{
    if(!shape)
    {
        return -1;
    }

    shape->vaild = 0;

    if(ctx->is_nncase)
    {
#if ONLY_KMODEL_V3
        rt_kprintf("Only support Kmodel V3\n");
#else
        if(0x00 == nncase_get_output_shape(ctx, &shape->chn, &shape->h, &shape->w))
        {
            shape->vaild = 1;
            return 0;
        }
#endif // ONLY_KMODEL_V3
        return -1;
    }
    else
    {
        const kpu_model_layer_header_t *_layer = NULL;
        const kpu_model_layer_header_t *output_layer = ctx->layer_headers + ctx->layers_length - 1;
        const kpu_model_layer_header_t *conv2_layer = ctx->layer_headers + ctx->layers_length - 2;

        if((KL_DEQUANTIZE != output_layer->type) || (KL_K210_CONV != conv2_layer->type))
        {
            return -1;
        }

        const uint8_t *body = ctx->body_start;
        for(int i = 0; i < (ctx->layers_length - 2); i++)
        {
            _layer = ctx->layer_headers + i;
            body += _layer->body_size;
        }

        if(KL_K210_CONV == _layer->type)
        {
            const kpu_model_conv_layer_argument_t *conv_layer = (const kpu_model_conv_layer_argument_t *)body;
            kpu_layer_argument_t layer_arg = *(volatile kpu_layer_argument_t *)(ctx->model_buffer + conv_layer->layer_offset);

            shape->chn = 1 + layer_arg.image_channel_num.data.o_ch_num;
            shape->h = 1 + layer_arg.image_size.data.o_col_high;
            shape->w = 1 + layer_arg.image_size.data.o_row_wid;
            shape->vaild = 1;

            return 0;
        }
    }

    return -1;
}


int maix_kpu_helper_get_output_count(kpu_model_context_t *ctx)
{
    if(ctx->is_nncase)
    {
#if ONLY_KMODEL_V3
        rt_kprintf("Only support Kmodel V3\n");
#else
        return nncase_get_output_count(ctx);
#endif // ONLY_KMODEL_V3
    }

    return ctx->output_count;
}
