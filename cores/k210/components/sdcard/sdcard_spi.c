#define DBG_TAG "SPI_SD_SPI"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

/* Includes ------------------------------------------------------------------*/
#include "sdcard/sdcard.h"

#include "rtthread.h"
#include "k210-hal.h"

/* Private spisd_interface_t soft SPI ----------------------------------------*/
typedef void (*soft_spi_transfer)(uint8_t *tx_buff, uint8_t *rx_buff, uint32_t size);
soft_spi_transfer _spi_transfer = hal_soft_spi_transfer_slow;

static void _soft_spi_init(int8_t sck, int8_t miso, int8_t mosi, int8_t cs)
{
    hal_fpioa_set_pin_func(cs, FUNC_GPIO0 + SOFT_SPI_CSXX_GPIO_FUNC);
    gpio_set_drive_mode(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_DM_OUTPUT);
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_HIGH);

    hal_soft_spi_init(sck, miso, mosi);
}

static void _soft_spi_set_speed(uint32_t freq)
{
    if(freq > SPI_SPEED_NO_INIT_HZ) {
        _spi_transfer = hal_soft_spi_transfer_fast;
    } else {
        _spi_transfer = hal_soft_spi_transfer_slow;
    }
}

static void _soft_spi_select(void)
{
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_LOW);
}

static void _soft_spi_relese(void)
{
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_HIGH);
}

static bool _soft_spi_is_present(void)
{
    return true;
}

static uint8_t _soft_spi_wr_rd_byte(uint8_t byte)
{
    uint8_t read = 0x00;

    _spi_transfer(&byte, &read, 1);

    return read;
}

static void _soft_spi_write(uint8_t const *buffer, uint32_t size)
{
    _spi_transfer((uint8_t *)buffer, NULL, size);
}

static void _soft_spi_read(uint8_t *buffer, uint32_t size)
{
    _spi_transfer(NULL, buffer, size);
}

const spisd_interface_t soft_spi_interface = {
    .init       =   _soft_spi_init,
    .set_speed  =   _soft_spi_set_speed,
    .select     =   _soft_spi_select,
    .relese     =   _soft_spi_relese,
    .is_present =   _soft_spi_is_present,
    .wr_rd_byte =   _soft_spi_wr_rd_byte,
    .write      =   _soft_spi_write,
    .read       =   _soft_spi_read,
};

uint8_t _sdcard_use_hw_spi1 = 0;

/* Private spisd_interface_t hard SPI ----------------------------------------*/
static void _hard_spi_init(int8_t sck, int8_t miso, int8_t mosi, int8_t cs)
{
    hal_fpioa_set_pin_func(cs, FUNC_GPIO0 + SOFT_SPI_CSXX_GPIO_FUNC);
    gpio_set_drive_mode(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_DM_OUTPUT);
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_HIGH);

    hal_fpioa_set_pin_func(sck,  FUNC_SPI1_SCLK);
    hal_fpioa_set_pin_func(miso, FUNC_SPI1_D1);
    hal_fpioa_set_pin_func(mosi, FUNC_SPI1_D0);

    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_set_clk_rate(SPI_DEVICE_1, 400 * 1000);

    _sdcard_use_hw_spi1 = 1;
}

static void _hard_spi_set_speed(uint32_t freq)
{
    spi_set_clk_rate(SPI_DEVICE_1, freq);
}

static void _hard_spi_select(void)
{
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_LOW);
}

static void _hard_spi_relese(void)
{
    gpio_set_pin(SOFT_SPI_CSXX_GPIO_FUNC, GPIO_PV_HIGH);
}

static bool _hard_spi_is_present(void)
{
    return true;
}

static uint8_t _hard_spi_wr_rd_byte(uint8_t byte)
{
    uint8_t d;

    // spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, &byte, &d, 1);

    return d;
}

static void _hard_spi_write(uint8_t const *buffer, uint32_t size)
{
    // spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_send_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, buffer, size);
}

static void _hard_spi_read(uint8_t *buffer, uint32_t size)
{
    // spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_receive_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, buffer, size);
}

const spisd_interface_t hard_spi_interface = {
    .init       =   _hard_spi_init,
    .set_speed  =   _hard_spi_set_speed,
    .select     =   _hard_spi_select,
    .relese     =   _hard_spi_relese,
    .is_present =   _hard_spi_is_present,
    .wr_rd_byte =   _hard_spi_wr_rd_byte,
    .write      =   _hard_spi_write,
    .read       =   _hard_spi_read,
};
