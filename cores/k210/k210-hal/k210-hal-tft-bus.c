#define DBG_TAG "HAL_TFT"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"

#define DCX_COMMAND     (GPIO_PV_LOW)
#define DCX_DATA        (GPIO_PV_HIGH)

#define CHECK_DMA_CHN   do { if(dma_chn >= DMAC_CHANNEL_MAX) { LOG_E("Get dma chn failed %s", __func__); return; }} while(0)

typedef enum {
    MODE_COMMAND = 0,
    MODE_DATA_BYTE,
    MODE_DATA_HALF,
    MODE_DATA_WORD,
    MODE_DATA_FILL,
    MODE_INVAILD = 255,
} tft_bus_mod_t;

struct tft_bus_t {
    // optional
    int8_t dcx_pin;
    // mandatory
    int8_t csx_pin;
    int8_t clk_pin;

    tft_bus_mod_t mode;
    spi_chip_select_t cs;

    int dc_gpio_func;
    uint32_t freq;

    const spi_device_num_t bus;
};

static struct tft_bus_t tft_bus = {
    .dcx_pin = -1,
    .csx_pin = -1,
    .clk_pin = -1,
    .mode = MODE_INVAILD,
    .cs = SPI_CHIP_SELECT_3,
    .dc_gpio_func = TFT_DCX_GPIO_FUNC,
    .freq = 10 * 1000 * 1000,
    .bus = SPI_DEVICE_0,
};

volatile uint8_t g_spi0_in_use = 0;

static inline void set_dcx(gpio_pin_value_t val)
{
    if(0 <= tft_bus.dcx_pin) {
        gpio_set_pin(tft_bus.dc_gpio_func, val);
    }
}

void hal_tft_write_command(uint8_t *cmd, uint32_t length)
{
    if((MODE_DATA_BYTE != tft_bus.mode) && (MODE_COMMAND != tft_bus.mode)) {
        tft_bus.mode = MODE_COMMAND;

        spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
        spi_init_non_standard(tft_bus.bus, 8, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    }

    dmac_channel_number_t dma_chn = hal_dma_get_free_chn();
    CHECK_DMA_CHN;

    set_dcx(DCX_COMMAND);

    spi_send_data_normal_dma(dma_chn, tft_bus.bus, tft_bus.cs, cmd, length, SPI_TRANS_CHAR);
    hal_dma_releas_chn(dma_chn);

    set_dcx(DCX_DATA);
}

void hal_tft_write_byte(uint8_t *data_buf, uint32_t length)
{
    if((MODE_DATA_BYTE != tft_bus.mode) && (MODE_COMMAND != tft_bus.mode)) {
        tft_bus.mode = MODE_DATA_BYTE;

        spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
        spi_init_non_standard(tft_bus.bus, 8, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    }

    dmac_channel_number_t dma_chn = hal_dma_get_free_chn();
    CHECK_DMA_CHN;

    set_dcx(DCX_DATA);
    spi_send_data_normal_dma(dma_chn, tft_bus.bus, tft_bus.cs, data_buf, length, SPI_TRANS_CHAR);
    hal_dma_releas_chn(dma_chn);
}

void hal_tft_write_half(uint16_t *data_buf, uint32_t length)
{
    if(MODE_DATA_HALF != tft_bus.mode) {
        tft_bus.mode = MODE_DATA_HALF;

        spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 16, 0);
        spi_init_non_standard(tft_bus.bus, 16, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    }

    dmac_channel_number_t dma_chn = hal_dma_get_free_chn();
    CHECK_DMA_CHN;

    set_dcx(DCX_DATA);
    spi_send_data_normal_dma(dma_chn, tft_bus.bus, tft_bus.cs, data_buf, length, SPI_TRANS_SHORT);
    hal_dma_releas_chn(dma_chn);
}

void hal_tft_write_word(uint32_t *data_buf, uint32_t length)
{
    if(MODE_DATA_WORD != tft_bus.mode) {
        tft_bus.mode = MODE_DATA_WORD;

        spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
        spi_init_non_standard(tft_bus.bus, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);
    }

    dmac_channel_number_t dma_chn = hal_dma_get_free_chn();
    CHECK_DMA_CHN;

    set_dcx(DCX_DATA);
    spi_send_data_normal_dma(dma_chn, tft_bus.bus, tft_bus.cs, data_buf, length, SPI_TRANS_INT);
    hal_dma_releas_chn(dma_chn);
}

void hal_tft_fill_data(uint32_t *data_buf, uint32_t length)
{
    if(MODE_DATA_FILL != tft_bus.mode) {
        tft_bus.mode = MODE_DATA_FILL;

        spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
        spi_init_non_standard(tft_bus.bus, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);
    }

    dmac_channel_number_t dma_chn = hal_dma_get_free_chn();
    CHECK_DMA_CHN;

    set_dcx(DCX_DATA);
    spi_fill_data_dma(dma_chn, tft_bus.bus, tft_bus.cs, data_buf, length);
    hal_dma_releas_chn(dma_chn);
}

void hal_tft_begin(int8_t clk_pin, int8_t cs_pin, int8_t dc_pin, uint32_t freq)
{
    if((-1 == clk_pin) || (-1 == cs_pin) /* || (-1 == dc_pin) */) {
        LOG_E("Invaild Pins");
        return;
    }

    tft_bus.clk_pin = clk_pin;
    tft_bus.csx_pin = cs_pin;
    tft_bus.dcx_pin = dc_pin;
    tft_bus.freq = freq;

    if(0 <= tft_bus.dcx_pin) {
        hal_fpioa_set_pin_func(tft_bus.dcx_pin, (fpioa_function_t)(FUNC_GPIO0 + tft_bus.dc_gpio_func));
        gpio_set_drive_mode(tft_bus.dc_gpio_func, GPIO_DM_OUTPUT);
    }

    set_dcx(DCX_DATA);

    hal_fpioa_set_pin_func(tft_bus.clk_pin, FUNC_SPI0_SCLK);
    hal_fpioa_set_pin_func(tft_bus.csx_pin, (fpioa_function_t)(FUNC_SPI0_SS0 + tft_bus.cs));

    spi_init(tft_bus.bus, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_set_clk_rate(tft_bus.bus, tft_bus.freq);

    sysctl_set_spi0_dvp_data(1);

    g_spi0_in_use = 1;
}

void hal_tft_end(void)
{
    sysctl_clock_disable(SYSCTL_CLOCK_SPI0);

    hal_fpioa_clr_pin_func(tft_bus.clk_pin);
    hal_fpioa_clr_pin_func(tft_bus.csx_pin);

    if(0 <= tft_bus.dcx_pin) {
        hal_fpioa_clr_pin_func(tft_bus.dcx_pin);
    }

    tft_bus.dcx_pin = -1;
    tft_bus.csx_pin = -1;
    tft_bus.clk_pin = -1;
    tft_bus.mode = MODE_INVAILD;
    tft_bus.cs = SPI_CHIP_SELECT_3;
    tft_bus.dc_gpio_func = TFT_DCX_GPIO_FUNC;
    tft_bus.freq = 10 * 1000 * 1000;

    g_spi0_in_use = 0;
}

void hal_tft_set_cs(int8_t pin, spi_chip_select_t chip_select)
{
    if((0 <= pin)) {
        hal_fpioa_clr_pin_func(tft_bus.csx_pin);

        if((0 <= chip_select) && (chip_select < SPI_CHIP_SELECT_MAX)) {
            tft_bus.cs = chip_select;
        }

        tft_bus.csx_pin = pin;
        hal_fpioa_set_pin_func(tft_bus.csx_pin, (fpioa_function_t)(FUNC_SPI0_SS0 + tft_bus.cs));
    }
}

void hal_tft_set_dc(int8_t pin, int gpio_func)
{
    if((0 <= pin)) {
        hal_fpioa_clr_pin_func(tft_bus.dcx_pin);

        if((0 <= gpio_func) && (gpio_func <= 7)) {
            tft_bus.dc_gpio_func = gpio_func;
        }

        tft_bus.dcx_pin = pin;
        hal_fpioa_set_pin_func(tft_bus.dcx_pin, (fpioa_function_t)(FUNC_GPIO0 + tft_bus.dc_gpio_func));

        set_dcx(DCX_DATA);
    }
}

void hal_tft_set_clock_freq(uint32_t freq)
{
    tft_bus.freq = freq;

    spi_set_clk_rate(tft_bus.bus, tft_bus.freq);
}
