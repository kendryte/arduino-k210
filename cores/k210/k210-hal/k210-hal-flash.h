#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define w25qxx_FLASH_PAGE_SIZE              (256)
#define w25qxx_FLASH_SECTOR_SIZE            (4096)
#define w25qxx_FLASH_PAGE_NUM_PER_SECTOR    (16)

typedef enum {
    W25QXX_ERROR    = -2,
    W25QXX_BUSY     = -1,
    W25QXX_OK       =  0,
} flash_status_t;

struct flash_id_t {
    uint8_t manuf;
    uint8_t device;
};

flash_status_t hal_flash_init(uint32_t freq_hz, struct flash_id_t *id);

flash_status_t hal_flash_set_standard_mode(void);
flash_status_t hal_flash_set_quad_mode(void);

flash_status_t hal_flash_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
flash_status_t hal_flash_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);

flash_status_t hal_flash_chip_erase(void);
flash_status_t hal_flash_erase(uint32_t addr, uint32_t length);

#ifdef __cplusplus
}
#endif
