#define DBG_TAG "HAL_FLASH"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"
#include "rtthread.h"

/*********************************************************************************************************************/
/* W25Qxx Write & Read Start.                                                                                        */
/*********************************************************************************************************************/

/* clang-format off */
#define DATALENGTH                          (8)

#define WRITE_ENABLE                        (0x06)
#define WRITE_DISABLE                       (0x04)
#define READ_REG1                           (0x05)
#define READ_REG2                           (0x35)
#define READ_REG3                           (0x15)
#define WRITE_REG1                          (0x01)
#define WRITE_REG2                          (0x31)
#define WRITE_REG3                          (0x11)
#define READ_DATA                           (0x03)
#define FAST_READ                           (0x0B)
#define FAST_READ_DUAL_OUTPUT               (0x3B)
#define FAST_READ_QUAL_OUTPUT               (0x6B)
#define FAST_READ_DUAL_IO                   (0xBB)
#define FAST_READ_QUAL_IO                   (0xEB)
#define DUAL_READ_RESET                     (0xFFFF)
#define QUAL_READ_RESET                     (0xFF)
#define PAGE_PROGRAM                        (0x02)
#define QUAD_PAGE_PROGRAM                   (0x32)
#define SECTOR_ERASE                        (0x20)
#define BLOCK_32K_ERASE                     (0x52)
#define BLOCK_64K_ERASE                     (0xD8)
#define CHIP_ERASE                          (0x60)
#define READ_ID                             (0x90)
#define ENABLE_QPI                          (0x38)
#define EXIT_QPI                            (0xFF)
#define ENABLE_RESET                        (0x66)
#define RESET_DEVICE                        (0x99)

#define REG1_BUSY_MASK                      (0x01)
#define REG2_QUAL_MASK                      (0x02)
/* clang-format on */

/**
 * @brief      w25qxx read operating enumerate
 */
typedef enum _flash_read
{
    W25QXX_STANDARD = 0,
    W25QXX_STANDARD_FAST,
    W25QXX_DUAL,
    W25QXX_DUAL_FAST,
    W25QXX_QUAD,
    W25QXX_QUAD_FAST,
} flash_read_t;

typedef flash_status_t (*hal_flash_erase_func)(uint32_t addr);

struct erase_func_t {
    uint32_t unit;
    hal_flash_erase_func func;
};

static flash_status_t w25qxx_sector_erase_dma(uint32_t addr);
static flash_status_t w25qxx_32k_block_erase_dma(uint32_t addr);
static flash_status_t w25qxx_64k_block_erase_dma(uint32_t addr);

static const struct erase_func_t erase_func_table[] = {
    {64 * 1024,     w25qxx_64k_block_erase_dma},
    {32 * 1024,     w25qxx_32k_block_erase_dma},
    { 4 * 1024,     w25qxx_sector_erase_dma},
    {        0,     NULL},
};

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
flash_status_t (*w25qxx_page_program_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);
flash_status_t (*w25qxx_read_fun)(uint32_t addr, uint8_t *data_buf, uint32_t length);

static flash_status_t w25qxx_page_program_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);
static flash_status_t w25qxx_quad_page_program_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);

flash_status_t w25qxx_stand_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);
flash_status_t w25qxx_quad_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length);

/*********************************************************************************************************************/
static flash_status_t w25qxx_receive_data_dma(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_receive_data_standard_dma(DMAC_CHANNEL0, DMAC_CHANNEL1, SPI_DEVICE_3, SPI_CHIP_SELECT_0, cmd_buff, cmd_len, rx_buff, rx_len);

    return W25QXX_OK;
}

static flash_status_t w25qxx_send_data_dma(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_send_data_standard_dma(DMAC_CHANNEL0, SPI_DEVICE_3, SPI_CHIP_SELECT_0, cmd_buff, cmd_len, tx_buff, tx_len);

    return W25QXX_OK;
}

static inline flash_status_t w25qxx_receive_data_enhanced_dma(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    spi_receive_data_multiple_dma(DMAC_CHANNEL0, DMAC_CHANNEL1, SPI_DEVICE_3, SPI_CHIP_SELECT_0, cmd_buff, cmd_len, rx_buff, rx_len);

    return W25QXX_OK;
}

static inline flash_status_t w25qxx_send_data_enhanced_dma(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    spi_send_data_multiple_dma(DMAC_CHANNEL0, SPI_DEVICE_3, SPI_CHIP_SELECT_0, cmd_buff, cmd_len, tx_buff, tx_len);

    return W25QXX_OK;
}

static flash_status_t w25qxx_init_dma(uint32_t rate)
{
    spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_set_clk_rate(SPI_DEVICE_3, rate);

    w25qxx_page_program_fun = w25qxx_page_program_dma;
    w25qxx_read_fun = w25qxx_stand_read_data_dma;

    return W25QXX_OK;
}

static flash_status_t w25qxx_read_id_dma(uint8_t *manuf_id, uint8_t *device_id)
{
    uint8_t cmd[4] = {READ_ID, 0x00, 0x00, 0x00};
    uint8_t data[2] = {0};

    w25qxx_receive_data_dma(cmd, 4, data, 2);
    *manuf_id = data[0];
    *device_id = data[1];

    return W25QXX_OK;
}

static flash_status_t w25qxx_write_enable_dma(void)
{
    uint8_t cmd[1] = {WRITE_ENABLE};

    w25qxx_send_data_dma(cmd, 1, 0, 0);

    return W25QXX_OK;
}

static flash_status_t w25qxx_write_status_reg_dma(uint8_t reg1_data, uint8_t reg2_data)
{
    uint8_t cmd[3] = {WRITE_REG1, reg1_data, reg2_data};

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 3, 0, 0);

    return W25QXX_OK;
}

flash_status_t w25qxx_write_status_reg1_dma(uint8_t reg_data)
{
    uint8_t cmd[2] = {WRITE_REG1, reg_data};

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 2, 0, 0);

    return W25QXX_OK;
}

flash_status_t w25qxx_write_status_reg2_dma(uint8_t reg_data)
{
    uint8_t cmd[2] = {WRITE_REG2, reg_data};

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 2, 0, 0);

    return W25QXX_OK;
}

flash_status_t w25qxx_write_status_reg3_dma(uint8_t reg_data)
{
    uint8_t cmd[2] = {WRITE_REG3, reg_data};

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 2, 0, 0);

    return W25QXX_OK;
}

flash_status_t w25qxx_read_status_reg1_dma(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG1};
    uint8_t data[1] = {0};

    w25qxx_receive_data_dma(cmd, 1, data, 1);
    *reg_data = data[0];

    return W25QXX_OK;
}

flash_status_t w25qxx_read_status_reg2_dma(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG2};
    uint8_t data[1] = {0};

    w25qxx_receive_data_dma(cmd, 1, data, 1);
    *reg_data = data[0];

    return W25QXX_OK;
}

flash_status_t w25qxx_read_status_reg3_dma(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG3};
    uint8_t data[1] = {0};

    w25qxx_receive_data_dma(cmd, 1, data, 1);
    *reg_data = data[0];

    return W25QXX_OK;
}

static flash_status_t w25qxx_is_busy_dma(void)
{
    uint8_t status = 0;

    w25qxx_read_status_reg1_dma(&status);
    if (status & REG1_BUSY_MASK)
        return W25QXX_BUSY;

    return W25QXX_OK;
}

static flash_status_t w25qxx_sector_erase_dma(uint32_t addr)
{
    uint8_t cmd[4] = {SECTOR_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 4, 0, 0);

    return W25QXX_OK;
}

static flash_status_t w25qxx_32k_block_erase_dma(uint32_t addr)
{
    uint8_t cmd[4] = {BLOCK_32K_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 4, 0, 0);

    return W25QXX_OK;
}

static flash_status_t w25qxx_64k_block_erase_dma(uint32_t addr)
{
    uint8_t cmd[4] = {BLOCK_64K_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 4, 0, 0);

    return W25QXX_OK;
}

static inline flash_status_t w25qxx_chip_erase_dma(void)
{
    uint8_t cmd[1] = {CHIP_ERASE};

    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 1, 0, 0);

    return W25QXX_OK;
}

static inline flash_status_t w25qxx_enable_quad_mode_dma(void)
{
    uint8_t reg_data = 0;

    w25qxx_read_status_reg2_dma(&reg_data);

    if (!(reg_data & REG2_QUAL_MASK)) {
        reg_data |= REG2_QUAL_MASK;
        /*
         * Don't touch these codes,
         * here is the fix for PUYA P25Q128L writing QE bit
         */
        w25qxx_write_status_reg2_dma(reg_data);
        w25qxx_write_status_reg_dma(0x00, reg_data);
        while (w25qxx_is_busy_dma()) {
            continue;
        }
    }

    w25qxx_page_program_fun = w25qxx_quad_page_program_dma;
    w25qxx_read_fun = w25qxx_quad_read_data_dma;

    return W25QXX_OK;
}

static inline flash_status_t w25qxx_disable_quad_mode_dma(void)
{
    uint8_t reg_data = 0;

    w25qxx_read_status_reg2_dma(&reg_data);

    if (reg_data & REG2_QUAL_MASK) {
        reg_data &= (~REG2_QUAL_MASK);
        /*
         * Don't touch these codes,
         * here is the fix for PUYA P25Q128L writing QE bit
         */
        w25qxx_write_status_reg2_dma(reg_data);
        w25qxx_write_status_reg_dma(0x00, reg_data);
        while (w25qxx_is_busy_dma()) {
            continue;
        }
    }

    w25qxx_page_program_fun = w25qxx_page_program_dma;
    w25qxx_read_fun = w25qxx_stand_read_data_dma;

    return W25QXX_OK;
}

static flash_status_t w25qxx_page_program_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint8_t cmd[4] = {PAGE_PROGRAM};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    w25qxx_write_enable_dma();
    w25qxx_send_data_dma(cmd, 4, data_buf, length);
    while (w25qxx_is_busy_dma() == W25QXX_BUSY)
        ;

    return W25QXX_OK;
}

static flash_status_t w25qxx_quad_page_program_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t cmd[2] = {0};

    cmd[0] = QUAD_PAGE_PROGRAM;
    cmd[1] = addr;
    w25qxx_write_enable_dma();
    spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_QUAD, DATALENGTH, 0);
    spi_init_non_standard(SPI_DEVICE_3, 8/*instrction length*/, 24/*address length*/, 0/*wait cycles*/,
                          SPI_AITM_STANDARD/*spi address trans mode*/);
    w25qxx_send_data_enhanced_dma(cmd, 2, data_buf, length);
    while (w25qxx_is_busy_dma() == W25QXX_BUSY)
        ;

    return W25QXX_OK;
}

static flash_status_t w25qxx_sector_program(uint32_t addr, uint8_t *data_buf)
{
    uint8_t index = 0;

    for (index = 0; index < w25qxx_FLASH_PAGE_NUM_PER_SECTOR; index++)
    {
        w25qxx_page_program_fun(addr, data_buf, w25qxx_FLASH_PAGE_SIZE);
        addr += w25qxx_FLASH_PAGE_SIZE;
        data_buf += w25qxx_FLASH_PAGE_SIZE;
    }

    return W25QXX_OK;
}

static flash_status_t w25qxx_write_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t sector_addr = 0;
    uint32_t sector_offset = 0;
    uint32_t sector_remain = 0;
    uint32_t write_len = 0;
    uint32_t index = 0;
    uint8_t *pread = NULL;
    uint8_t *pwrite = NULL;
    uint8_t swap_buf[w25qxx_FLASH_SECTOR_SIZE] = {0};

    while (length)
    {
        sector_addr = addr & (~(w25qxx_FLASH_SECTOR_SIZE - 1));
        sector_offset = addr & (w25qxx_FLASH_SECTOR_SIZE - 1);
        sector_remain = w25qxx_FLASH_SECTOR_SIZE - sector_offset;
        write_len = ((length < sector_remain) ? length : sector_remain);
        w25qxx_read_fun(sector_addr, swap_buf, w25qxx_FLASH_SECTOR_SIZE);
        pread = swap_buf + sector_offset;
        pwrite = data_buf;
        for (index = 0; index < write_len; index++)
        {
            if ((*pwrite) != ((*pwrite) & (*pread)))
            {
                w25qxx_sector_erase_dma(sector_addr);
                while (w25qxx_is_busy_dma() == W25QXX_BUSY)
                    ;
                break;
            }
            pwrite++;
            pread++;
        }
        if (write_len == w25qxx_FLASH_SECTOR_SIZE)
        {
            w25qxx_sector_program(sector_addr, data_buf);
        }
        else
        {
            pread = swap_buf + sector_offset;
            pwrite = data_buf;
            for (index = 0; index < write_len; index++)
                *pread++ = *pwrite++;
            w25qxx_sector_program(sector_addr, swap_buf);
        }
        length -= write_len;
        addr += write_len;
        data_buf += write_len;
    }

    return W25QXX_OK;
}

flash_status_t w25qxx_write_data_direct_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t page_remain = 0;
    uint32_t write_len = 0;

    while (length)
    {
        page_remain = w25qxx_FLASH_PAGE_SIZE - (addr & (w25qxx_FLASH_PAGE_SIZE - 1));
        write_len = ((length < page_remain) ? length : page_remain);
        w25qxx_page_program_fun(addr, data_buf, write_len);
        length -= write_len;
        addr += write_len;
        data_buf += write_len;
    }

    return W25QXX_OK;
}

static flash_status_t _w25qxx_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length, flash_read_t mode)
{
    uint32_t cmd[2] = {0};

    switch (mode) {
#if 0
        case W25QXX_STANDARD:
            *(((uint8_t *)cmd) + 0) = READ_DATA;
            *(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
            *(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
            *(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
            w25qxx_receive_data_dma((uint8_t *)cmd, 4, data_buf, length);
            break;
        case W25QXX_DUAL:
            cmd[0] = FAST_READ_DUAL_OUTPUT;
            cmd[1] = addr;
            spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_DUAL, DATALENGTH, 0);
            spi_init_non_standard(SPI_DEVICE_3, 8/*instrction length*/, 24/*address length*/, 8/*wait cycles*/,
                                  SPI_AITM_STANDARD/*spi address trans mode*/);
            w25qxx_receive_data_enhanced_dma(cmd, 2, data_buf, length);
            break;
        case W25QXX_DUAL_FAST:
            cmd[0] = FAST_READ_DUAL_IO;
            cmd[1] = addr << 8;
            spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_DUAL, DATALENGTH, 0);
            spi_init_non_standard(SPI_DEVICE_3, 8/*instrction length*/, 32/*address length*/, 0/*wait cycles*/,
                                  SPI_AITM_ADDR_STANDARD/*spi address trans mode*/);
            w25qxx_receive_data_enhanced_dma(cmd, 2, data_buf, length);
            break;
        case W25QXX_QUAD:
            cmd[0] = FAST_READ_QUAL_OUTPUT;
            cmd[1] = addr;
            spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_QUAD, DATALENGTH, 0);
            spi_init_non_standard(SPI_DEVICE_3, 8/*instrction length*/, 24/*address length*/, 8/*wait cycles*/,
                                  SPI_AITM_STANDARD/*spi address trans mode*/);
            w25qxx_receive_data_enhanced_dma(cmd, 2, data_buf, length);
            break;
#endif
        case W25QXX_STANDARD_FAST:
            *(((uint8_t *)cmd) + 0) = FAST_READ;
            *(((uint8_t *)cmd) + 1) = (uint8_t)(addr >> 16);
            *(((uint8_t *)cmd) + 2) = (uint8_t)(addr >> 8);
            *(((uint8_t *)cmd) + 3) = (uint8_t)(addr >> 0);
            *(((uint8_t *)cmd) + 4) = 0xFF;
            w25qxx_receive_data_dma((uint8_t *)cmd, 5, data_buf, length);
            break;
        case W25QXX_QUAD_FAST:
            cmd[0] = FAST_READ_QUAL_IO;
            cmd[1] = addr << 8;
            spi_init(SPI_DEVICE_3, SPI_WORK_MODE_0, SPI_FF_QUAD, DATALENGTH, 0);
            spi_init_non_standard(SPI_DEVICE_3, 8/*instrction length*/, 32/*address length*/, 4/*wait cycles*/,
                                  SPI_AITM_ADDR_STANDARD/*spi address trans mode*/);
            w25qxx_receive_data_enhanced_dma(cmd, 2, data_buf, length);
            break;
        default:
            LOG_E("Unsupport read mode.");
            break;
    }

    return W25QXX_OK;
}

static flash_status_t w25qxx_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length, flash_read_t mode)
{
    uint32_t len = 0;

    while (length)
    {
        len = ((length >= 0x010000) ? 0x010000 : length);
        _w25qxx_read_data_dma(addr, data_buf, len, mode);
        addr += len;
        data_buf += len;
        length -= len;
    }

    return W25QXX_OK;
}

flash_status_t w25qxx_stand_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return w25qxx_read_data_dma(addr, data_buf, length, W25QXX_STANDARD_FAST);
}

flash_status_t w25qxx_quad_read_data_dma(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return w25qxx_read_data_dma(addr, data_buf, length, W25QXX_QUAD_FAST);
}

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
flash_status_t hal_flash_init(uint32_t freq_hz, struct flash_id_t *id)
{
    static int _inited = 0;

    if(0x00 == _inited) {
        _inited = 1;

        w25qxx_init_dma(freq_hz);

        w25qxx_enable_quad_mode_dma();
    }

    if(id) {
        w25qxx_read_id_dma(&id->manuf, &id->device);

        LOG_D("flash id 0x%02X:0x%02X\r\n", id->manuf, id->device);
    }

    return W25QXX_OK;
}

flash_status_t hal_flash_set_standard_mode(void)
{
    return w25qxx_disable_quad_mode_dma();
}

flash_status_t hal_flash_set_quad_mode(void)
{
    return w25qxx_enable_quad_mode_dma();
}

flash_status_t hal_flash_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return w25qxx_quad_read_data_dma(addr, data_buf, length);
}

flash_status_t hal_flash_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    return w25qxx_write_data_dma(addr, data_buf, length);
}

flash_status_t hal_flash_chip_erase(void)
{
    return w25qxx_chip_erase_dma();
}

flash_status_t hal_flash_erase(uint32_t addr, uint32_t length)
{
    uint32_t start_addr = addr;
    uint32_t erase_size = length;

    uint32_t erase_unit = 0;
    hal_flash_erase_func fn = NULL;

    for(int i = 0; i < sizeof(erase_func_table) / sizeof(erase_func_table[0]); i++) {
        fn = erase_func_table[i].func;
        erase_unit = erase_func_table[i].unit;

        if(fn && (0x00 == (start_addr % erase_unit)) && (0x00 == erase_size % erase_unit)) {
            break;
        }
    }

    if((NULL == fn) || (0x00 == erase_unit)) {
        return W25QXX_ERROR;
    }

    for(int i = 0; i < (erase_size / erase_unit); i++) {
        fn(start_addr);
        start_addr += erase_unit;
    }

    return W25QXX_OK;
}
