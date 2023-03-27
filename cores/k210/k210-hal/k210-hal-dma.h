#pragma once

#include <stdint.h>

#include "k210_dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

dmac_channel_number_t hal_dma_get_free_chn(void);
void hal_dma_releas_chn(dmac_channel_number_t chn);

#ifdef __cplusplus
}
#endif
