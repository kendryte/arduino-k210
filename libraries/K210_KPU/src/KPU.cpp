#define DBG_TAG "KPU"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtthread.h"

#include "KPU.h"

namespace K210
{

    KPU_Base::KPU_Base()
    {
        state.u32 = 0;

        shape.input.vaild = 0;
        shape.output.vaild = 0;

        model.size = 0;
        memset(model.name, 0, sizeof(model.name));
    }

    KPU_Base::~KPU_Base()
    {
        end();
    }

    void KPU_Base::end()
    {
        if(state.load_kmodel)
        {
            state.load_kmodel = 0;
            kpu_model_free(&model.ctx);
        }

        if(model.buffer)
        {
            rt_free_align(model.buffer);
            model.buffer = NULL;
        }
    }

    int KPU_Base::load_kmodel(uint8_t *buffer, size_t size, const char *name)
    {
        if(state.load_kmodel)
        {
            end();
        }

        state.load_kmodel = 0;

        model.buffer = buffer;
        model.size = size;
        strncpy(model.name, name, sizeof(model.name));

        if(0x00 != kpu_load_kmodel(&model.ctx, model.buffer))
        {
            LOG_E("load model failed");
            return -1;
        }

        maix_kpu_helper_get_input_shape(&model.ctx, &shape.input);
        maix_kpu_helper_get_output_shape(&model.ctx, &shape.output);

        if(0x00 == shape.input.vaild)
        {
            kpu_model_free(&model.ctx);

            LOG_E("parse model input shape failed");
            return -1;
        }

        // rt_kprintf("input %dx%dx%d, output %dx%dx%d\n",
        //            shape.input.chn, shape.input.h, shape.input.w,
        //            shape.output.chn, shape.output.h, shape.output.w);

        state.load_kmodel = 1;

        return 0;
    }

    int KPU_Base::load_kmodel(fs::FS &fs, const char *name)
    {
        fs::File file = fs.open(name);

        if(!file || file.isDirectory())
        {
            file.close();

            LOG_E("open file %s failed.", name);
            return -1;
        }

        size_t file_size = file.size();

        size_t hdr_size = file_size > 64 * 1024 ? 64 * 1024 : file_size;
        uint8_t *hdr_buff = (uint8_t *)rt_malloc_align(hdr_size, 8);
        if(NULL == hdr_buff)
        {
            file.close();

            LOG_E("malloc failed");
            return -1;
        }

        if(hdr_size != file.read(hdr_buff, hdr_size))
        {
            file.close();
            rt_free_align(hdr_buff);

            LOG_E("read file hdr failed");
            return -1;
        }

        int model_size = maix_kpu_helper_probe_model_size(hdr_buff, hdr_size);
        rt_free_align(hdr_buff);

        if(model_size <= 0)
        {
            file.close();

            LOG_E("parse model failed");
            return -1;
        }

        if(model_size > file_size)
        {
            file.close();

            LOG_E("Model in filesystem maybe damaged");
            return -1;
        }
        else if(model_size < file_size)
        {
            model_size = file_size;
        }

        uint8_t *model_buff = (uint8_t *)rt_malloc_align(model_size, 8);
        if(NULL == model_buff)
        {
            file.close();

            LOG_E("malloc failed");
            return -1;
        }

        file.seek(0, fs::SeekSet);
        if(model_size != file.read(model_buff, model_size))
        {
            file.close();
            rt_free_align(model_buff);

            LOG_E("read file failed");
            return -1;
        }
        file.close();

        char model_name[128];
        snprintf(model_name, sizeof(model_name), "fs:/%s", name);

        if(0x00 != load_kmodel(model_buff, model_size, model_name))
        {
            rt_free_align(model_buff);

            LOG_E("load model failed");
            return -1;
        }

        return 0;
    }

    int KPU_Base::load_kmodel(uint32_t offset)
    {
        size_t hdr_size = 64 * 1024;
        uint8_t *hdr_buff = (uint8_t *)rt_malloc_align(hdr_size, 8);
        if(NULL == hdr_buff)
        {
            LOG_E("malloc failed");
            return -1;
        }

        if(W25QXX_OK != hal_flash_read_data(offset, hdr_buff, hdr_size))
        {
            rt_free_align(hdr_buff);

            LOG_E("read flash failed");
            return -1;
        }

        int model_size = maix_kpu_helper_probe_model_size(hdr_buff, hdr_size);

        rt_free_align(hdr_buff);

        if(model_size <= 0)
        {
            LOG_E("parse model failed");
            return -1;
        }

        uint8_t *model_buff = (uint8_t *)rt_malloc_align(model_size, 8);
        if(NULL == model_buff)
        {
            LOG_E("malloc failed");
            return -1;
        }

        if(W25QXX_OK != hal_flash_read_data(offset, model_buff, model_size))
        {
            rt_free_align(model_buff);

            LOG_E("read flash failed");
            return -1;
        }

        char model_name[128];
        snprintf(model_name, sizeof(model_name), "flash:/%d", offset);

        if(0x00 != load_kmodel(model_buff, model_size, model_name))
        {
            rt_free_align(model_buff);

            LOG_E("load model failed");
            return -1;
        }

        return 0;
    }

    extern "C"
    {
        static volatile int _ai_done_flag = 0;

        static void ai_done(void *ctx)
        {
            _ai_done_flag = 1;
        }
    }

    int KPU_Base::run_kmodel(uint8_t *r8g8b8, int w, int h, dmac_channel_number_t dam_ch)
    {
        bool used_hal_dma_chn = false;
        dmac_channel_number_t _dma_ch;

        if (0x00 == state.load_kmodel)
        {
            LOG_E("Please load kmodel before");
            return -1;
        }

        if(NULL == r8g8b8)
        {
            LOG_E("invaild input");
            return -1;
        }

        if ((0x00 == shape.input.vaild) ||
            (w != shape.input.w) ||
            (h != shape.input.h))
        {
            LOG_E("model input %dx%d, but image is %dx%d", shape.input.w, shape.input.h, w, h);
            return -1;
        }

        if(DMAC_CHANNEL_MAX == dam_ch)
        {
            used_hal_dma_chn = true;
            _dma_ch = hal_dma_get_free_chn();
            if(DMAC_CHANNEL_MAX == _dma_ch)
            {
                LOG_E("no free dma chn");
                return -1;
            }
        }
        else
        {
            _dma_ch = dam_ch;
        }

        _ai_done_flag = 0;
        if(0x00 != kpu_run_kmodel(&model.ctx, r8g8b8, _dma_ch, ai_done, NULL))
        {
            if(used_hal_dma_chn)
            {
                hal_dma_releas_chn(_dma_ch);
            }

            LOG_E("run kmodel failed");
            return -1;
        }
        while (0x00 == _ai_done_flag);
        _ai_done_flag = 0;

        dmac_irq_unregister(_dma_ch);

        if(used_hal_dma_chn)
        {
            hal_dma_releas_chn(_dma_ch);
        }

        state.run_kmodel = 1;

        return 0;
    }

    int KPU_Base::run_kmodel(Image *img, dmac_channel_number_t dam_ch)
    {
        if(IMAGE_FORMAT_R8G8B8 != img->format)
        {
            LOG_E("Invaild image format");
            return -1;
        }

        return run_kmodel(img->pixel, img->w, img->h, dam_ch);
    }

    int KPU_Base::get_result(uint8_t **data, size_t *count, uint32_t startIndex)
    {
        if ((0x00 == state.load_kmodel) || (0x00 == state.run_kmodel))
        {
            LOG_E("Please load/run kmodel before");
            return -1;
        }

        return kpu_get_output(&model.ctx, startIndex, data, count);
    }

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    KPU_Yolo2::KPU_Yolo2() : KPU_Base()
    {

    }

    KPU_Yolo2::~KPU_Yolo2()
    {
        end();
    }

    void KPU_Yolo2::end()
    {
        if (_rl.anchor)
        {
            rt_free_align(_rl.anchor);
            _rl.anchor = NULL;
        }
        yolo_region_layer_deinit(&_rl);
    }

    int KPU_Yolo2::begin(float *anchor, int anchor_num, float threshold, float nms_value)
    {
        if((NULL == anchor) || (anchor_num % 2))
        {
            LOG_E("anchor invaild");
            return -1;
        }

        if (0x00 == state.load_kmodel)
        {
            LOG_E("Please load/run kmodel before");
            return -1;
        }

        if ((0x00 == shape.input.vaild) || (0x00 == shape.output.vaild))
        {
            LOG_E("Parse model for yolo run failed");
            return -1;
        }

        _rl.anchor = (float *)rt_malloc_align(sizeof(float) * anchor_num, 8);
        if(NULL == _rl.anchor)
        {
            LOG_E("malloc failed");
            return -1;
        }

        for(int i = 0; i < anchor_num; i++)
        {
            _rl.anchor[i] = anchor[i];
        }
        _rl.anchor_number = anchor_num / 2;
        _rl.threshold = threshold;
        _rl.nms_value = nms_value;

        int ret = yolo_region_layer_init(&_rl, &shape.input, &shape.output);
        if(0x00 != ret)
        {
            rt_free_align(_rl.anchor);
            _rl.anchor = NULL;

            LOG_E("yolo region layer init faild %d", ret);
            return -1;
        }

        return 0;
    }

    int KPU_Yolo2::run(uint8_t *r8g8b8, int w, int h, obj_info_t *info, dmac_channel_number_t dam_ch)
    {
        if(0x00 != run_kmodel(r8g8b8, w, h, dam_ch))
        {
            LOG_E("run model failed");
            return -1;
        }

        float *output = NULL;
        size_t output_size = 0;
        if(0x00 != get_result((uint8_t **)&output, &output_size))
        {
            LOG_E("get result failed");
            return -1;
        }

        _rl.input = output;
        region_layer_run(&_rl, info);

        return 0;
    }

    int KPU_Yolo2::run(Image *img, obj_info_t *info, dmac_channel_number_t dam_ch)
    {
        if(IMAGE_FORMAT_R8G8B8 != img->format)
        {
            LOG_E("Invaild image format");
            return -1;
        }

        return run(img->pixel, img->w, img->h, info, dam_ch);
    }

}
