#define DBG_TAG "HAL_DMA"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtthread.h"

#include "k210_bsp.h"
#include "k210-hal.h"

#define DMA_CHN_NUM                 sizeof(_dma_chn_info) / sizeof(_dma_chn_info[0])

struct dma_chn_info_t {
    uint8_t chn_sts; /* 0: not used, 1: used */
    const dmac_channel_number_t chn;
};

static _lock_t _hal_dma_lock = 0;

static struct dma_chn_info_t _dma_chn_info[] = {
    {0, DMAC_CHANNEL0},
    {0, DMAC_CHANNEL1},
    {0, DMAC_CHANNEL2},
    {0, DMAC_CHANNEL3},
    // {0, DMAC_CHANNEL4}, // for flash
    // {0, DMAC_CHANNEL5}, // for flash
};

dmac_channel_number_t hal_dma_get_free_chn(void)
{
    _lock_acquire(&_hal_dma_lock);

    for(int i = 0; i < DMA_CHN_NUM; i++) {
        if(0x00 == _dma_chn_info[i].chn_sts) {
            _dma_chn_info[i].chn_sts = 1;
            _lock_release(&_hal_dma_lock);
            return _dma_chn_info[i].chn;
        }
    }

    _lock_release(&_hal_dma_lock);

    return DMAC_CHANNEL_MAX;
}

void hal_dma_releas_chn(dmac_channel_number_t chn)
{
    if(chn >= DMA_CHN_NUM) {
        LOG_E("release invaild chn(%d)", chn);
        return;
    }

    _lock_acquire(&_hal_dma_lock);

    for(int i = 0; i < DMA_CHN_NUM; i++) {
        if(chn == _dma_chn_info[i].chn) {
            _dma_chn_info[i].chn_sts = 0;
            _lock_release(&_hal_dma_lock);
            return;
        }
    }

    _lock_release(&_hal_dma_lock);
}
