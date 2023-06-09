/* Copyright 2019 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <nncase.h>
#include <kernels/k210/k210_kernels.h>
#include <runtime/target_interpreter.h>
#include <stdio.h>
#include <cstring>
#include <k210_utils.h>
//#include <utils.h>

using namespace nncase;
using namespace nncase::runtime;

#define NNCASE_DEBUG 0

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

namespace
{
void kpu_upload_dma(dmac_channel_number_t dma_ch, const uint8_t *src, uint8_t *dest, size_t input_size, plic_irq_callback_t callback, void *userdata)
{
    dmac_irq_register(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)dest, DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
    dmac_wait_done(dma_ch);
}
}

#pragma GCC diagnostic pop

class nncase_context
{
public:
    int load_kmodel(const uint8_t *buffer)
    {
        int ret = interpreter_.try_load_model(buffer) ? 0 : -1;

        uint32_t size = interpreter_.model_size(buffer);
        uint8_t *buffer_iomem = (uint8_t *)((uintptr_t)buffer);
        const uint8_t *buffer_cache = buffer;
        memcpy(buffer_iomem, buffer_cache, size);
        for (int i = 0; i < size; i++)
        {
            if (buffer_iomem[i] != buffer_cache[i])
            {
                printf("flush model fail:%d %x %x \n", i, buffer_iomem[i], buffer_cache[i]);
                while (1)
                    ;
            }
        }
        return ret;
    }

    int get_input_shape_at(size_t index, int *chn, int *h, int *w)
    {
        if(index >= interpreter_.inputs_size())
            return -1;

        auto shape = interpreter_.input_shape_at(index);
        *chn = shape[1]; *h = shape[2]; *w = shape[3];

        return 0;
    }

    int get_output_count(void)
    {
        return interpreter_.outputs_size();
    }

    int get_output_shape( int *chn, int *h, int *w)
    {
        return interpreter_.get_output_shape(chn, h, w);
    }

    int32_t probe_model_size(const uint8_t *buffer, uint32_t buffer_size)
    {
        return interpreter_.probe_model_size(buffer, buffer_size);
    }

    int get_output(uint32_t index, uint8_t **data, size_t *size)
    {
        if (index >= interpreter_.outputs_size())
            return -1;

        auto mem = interpreter_.memory_at<uint8_t>(interpreter_.output_at(index));
        *data = mem.data();
        *size = mem.size();
        return 0;
    }

    int run_kmodel(const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata)
    {
        done_callback_ = done_callback;
        userdata_ = userdata;
        interpreter_.dma_ch(dma_ch);

        auto input = interpreter_.input_at(0);
        auto mem = interpreter_.memory_at<uint8_t>(input);
        if (input.memory_type == mem_main)
        {
            std::copy(src, src + mem.size(), mem.begin());
            interpreter_.run(done_thunk, on_error_thunk, node_profile_thunk, this);
            return 0;
        }
        else if (input.memory_type == mem_k210_kpu)
        {
            auto shape = interpreter_.input_shape_at(0);
            kernels::k210::kpu_upload(src, mem.data(), shape);
            on_upload_done();

            return 0;
        }

        return -1;
    }

private:
    void on_done()
    {
#if NNCASE_DEBUG
        printf("Total: %fms\n", interpreter_.total_duration().count() / 1e6);
#endif

        if (done_callback_)
            done_callback_(userdata_);
    }

    void on_upload_done()
    {
        interpreter_.run(done_thunk, on_error_thunk, node_profile_thunk, this);
    }

    static void done_thunk(void *userdata)
    {
        reinterpret_cast<nncase_context *>(userdata)->on_done();
    }

    static void on_error_thunk(const char *err, void *userdata)
    {
#if NNCASE_DEBUG
        printf("Fatal: %s\n", err);
#endif
    }

    static void node_profile_thunk(runtime_opcode op, std::chrono::nanoseconds duration, void *userdata)
    {
#if NNCASE_DEBUG
        printf("%s: %fms\n", node_opcode_names(op).data(), duration.count() / 1e6);
#endif
    }

    static int upload_done_thunk(void *userdata)
    {
        reinterpret_cast<nncase_context *>(userdata)->on_upload_done();
        return 0;
    }

private:
    interpreter_t interpreter_;
    kpu_done_callback_t done_callback_;
    void *userdata_;
};

int nncase_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer)
{
    auto nnctx = new (std::nothrow) nncase_context();
    if (ctx)
    {
        ctx->is_nncase = 1;
        ctx->nncase_ctx = nnctx;
        return nnctx->load_kmodel(buffer);
    }
    else
    {
        return -1;
    }
}

int nncase_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->get_output(index, data, size);
}

void nncase_model_free(kpu_model_context_t *ctx)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    delete nnctx;
    ctx->nncase_ctx = nullptr;
}

int nncase_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->run_kmodel(src, dma_ch, done_callback, userdata);
}


int nncase_get_input_shape(kpu_model_context_t *ctx, size_t index, int *chn, int *h, int *w)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->get_input_shape_at(index, chn, h, w);
}

int nncase_get_output_count(kpu_model_context_t *ctx)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->get_output_count();
}

int nncase_get_output_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->get_output_shape(chn, h, w);
}

int32_t nncase_probe_model_buffer_size(const uint8_t *buffer, uint32_t buffer_size)
{
    auto nnctx = new (std::nothrow) nncase_context();

    int32_t size = nnctx->probe_model_size(buffer, buffer_size);

    delete nnctx;

    return size;
}
