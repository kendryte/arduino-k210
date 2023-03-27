#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hal_soft_spi_transfer_slow(uint8_t *tx_buff, uint8_t *rx_buff, uint32_t size);
void hal_soft_spi_transfer_fast(uint8_t *tx_buff, uint8_t *rx_buff, uint32_t size);

void hal_soft_spi_set_clock(uint32_t freq);

void hal_soft_spi_init(int8_t sck, int8_t miso, int8_t mosi);

#ifdef __cplusplus
}
#endif
