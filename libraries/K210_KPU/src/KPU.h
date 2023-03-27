#pragma once

#include "Arduino.h"

#include "k210-hal.h"
#include "k210_kpu.h"

#include "kpu_helper.h"
#include "yolo2_region_layer.h"

#include "FS.h"
#include "Image.h"

namespace K210
{

    class KPU_Base
    {
    public:
        KPU_Base();
        ~KPU_Base();

        void end();

        int load_kmodel(uint8_t *buffer, size_t size, const char *name = "default"); // model should malloced by rt_malloc_align(size, 8);

        int load_kmodel(fs::FS &fs, const char *name);
        int load_kmodel(uint32_t offset);

        int run_kmodel(uint8_t *r8g8b8, int w, int h, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);
        int run_kmodel(Image *img, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);

        int get_result(uint8_t **data, size_t *count, uint32_t startIndex = 0);

    protected:
        struct
        {
            kpu_model_context_t ctx;
            char name[128];
            uint8_t *buffer;
            size_t size;
        } model;

        union
        {
            uint32_t u32;
            struct
            {
                uint32_t load_kmodel : 1;
                uint32_t run_kmodel : 1;
            };
        } state;

        struct
        {
            k210_kpu_shape_t input, output;
        } shape;
    };

    class KPU_Activation
    {
    public:
        static float sigmoid(float x)
        {
            return 1.f / (1.f + expf(-x));
        }

        static void softmax(float *x, float *dx, uint32_t len)
        {
            float max_value = x[0];
            for (uint32_t i = 0; i < len; i++)
            {
                if (max_value < x[i])
                {
                    max_value = x[i];
                }
            }
            for (uint32_t i = 0; i < len; i++)
            {
                x[i] -= max_value;
                x[i] = expf(x[i]);
            }
            float sum_value = 0.0f;
            for (uint32_t i = 0; i < len; i++)
            {
                sum_value += x[i];
            }
            for (uint32_t i = 0; i < len; i++)
            {
                dx[i] = x[i] / sum_value;
            }
        }
    };

    class KPU_Yolo2 : public KPU_Base
    {
    public:
        KPU_Yolo2();
        ~KPU_Yolo2();

        int begin(float *anchor, int anchor_num, float threshold = 0.5, float nms_value = 0.3);
        void end();

        int run(uint8_t *r8g8b8, int w, int h, obj_info_t *info, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);
        int run(Image *img, obj_info_t *info, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);

    private:
        yolo2_region_layer_t _rl;
    };

}
