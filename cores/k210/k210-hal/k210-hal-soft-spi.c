#define DBG_TAG "HAL_SOFT_SPI"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"

#include "rtthread.h"

///////////////////////////////////////////////////////////////////////////////
#define SOFT_SPI_CPHA           (1<<0)                              /* bit[0]:CPHA, clock phase */
#define SOFT_SPI_CPOL           (1<<1)                              /* bit[1]:CPOL, clock polarity */

#define SOFT_SPI_LSB            (0<<2)                              /* bit[2]: 0-LSB */
#define SOFT_SPI_MSB            (1<<2)                              /* bit[2]: 1-MSB */

#define SOFT_SPI_MODE_0         (0 | 0)                             /* CPOL = 0, CPHA = 0 */
#define SOFT_SPI_MODE_1         (0 | SOFT_SPI_CPHA)                 /* CPOL = 0, CPHA = 1 */
#define SOFT_SPI_MODE_2         (SOFT_SPI_CPOL | 0)                 /* CPOL = 1, CPHA = 0 */
#define SOFT_SPI_MODE_3         (SOFT_SPI_CPOL | SOFT_SPI_CPHA)     /* CPOL = 1, CPHA = 1 */

#define GPIO_OUT_HIGH(io)       (*(volatile uint32_t *)0x50200000U) |= (1 << (io))
#define GPIO_OUT_LOWX(io)       (*(volatile uint32_t *)0x50200000U) &= ~(1 << (io))

#define GET_GPIO_VALX(io)       (((*(volatile uint32_t *)0x50200050U) >> (io)) & 1)
///////////////////////////////////////////////////////////////////////////////
static const uint8_t _soft_spi_mode = SOFT_SPI_MODE_3 | SOFT_SPI_MSB;

static uint32_t _soft_spi_sclk_sts = 0;
///////////////////////////////////////////////////////////////////////////////
#pragma GCC push_options
#pragma GCC optimize("O0")

static void inline hal_soft_spi_toggle_sclk(void)
{
    if(_soft_spi_sclk_sts) {
        GPIO_OUT_LOWX(SOFT_SPI_SCLK_GPIO_FUNC);
    } else {
        GPIO_OUT_HIGH(SOFT_SPI_SCLK_GPIO_FUNC);
    }
    _soft_spi_sclk_sts = 1 - _soft_spi_sclk_sts;
}

void hal_soft_spi_transfer_slow(uint8_t *tx_buff, uint8_t *rx_buff, uint32_t size)
{
    rt_enter_critical();

    if(_soft_spi_mode & SOFT_SPI_CPHA) {
        hal_soft_spi_toggle_sclk();
    }

    while(size--) {
        uint8_t bit = 0, rbit = 0, send = 0xFF, read = 0xFF;

        if(NULL != tx_buff) {
            send = *tx_buff++;
        }

        for (int i = 0; i < 8; i++) {
            if(_soft_spi_mode & SOFT_SPI_MSB) {
                bit = send & (0x1 << (7 - i));
                read <<= 1; rbit = 0x01;
            } else {
                bit = send & (0x1 << i);
                read >>= 1; rbit = 0x80;
            }

            if (bit) { GPIO_OUT_HIGH(SOFT_SPI_MOSI_GPIO_FUNC); }
            else     { GPIO_OUT_LOWX(SOFT_SPI_MOSI_GPIO_FUNC); }

            hal_soft_spi_toggle_sclk();
            k210_usleep(1);

            if (GET_GPIO_VALX(SOFT_SPI_MISO_GPIO_FUNC)) { read |=  rbit; }
            else                                        { read &= ~rbit; }

            if (!(_soft_spi_mode & SOFT_SPI_CPHA) || (size != 0) || (i < 7)) {
                hal_soft_spi_toggle_sclk();
                k210_usleep(1);
            }
        }

        if(NULL != rx_buff) {
            *rx_buff++ = read;
        }
    }

    rt_exit_critical();
}

void hal_soft_spi_transfer_fast(uint8_t *tx_buff, uint8_t *rx_buff, uint32_t size)
{
    rt_enter_critical();

    if(_soft_spi_mode & SOFT_SPI_CPHA) {
        hal_soft_spi_toggle_sclk();
    }

    while(size--) {
        uint8_t bit = 0, rbit = 0, send = 0xFF, read = 0xFF;

        if(NULL != tx_buff) {
            send = *tx_buff++;
        }

        for (int i = 0; i < 8; i++) {
            if(_soft_spi_mode & SOFT_SPI_MSB) {
                bit = send & (0x1 << (7 - i));
                read <<= 1; rbit = 0x01;
            } else {
                bit = send & (0x1 << i);
                read >>= 1; rbit = 0x80;
            }

            if (bit) { GPIO_OUT_HIGH(SOFT_SPI_MOSI_GPIO_FUNC); }
            else     { GPIO_OUT_LOWX(SOFT_SPI_MOSI_GPIO_FUNC); }

            hal_soft_spi_toggle_sclk();
            asm volatile("nop"); asm volatile("nop");
            asm volatile("nop"); asm volatile("nop");

            if (GET_GPIO_VALX(SOFT_SPI_MISO_GPIO_FUNC)) { read |=  rbit; }
            else                                        { read &= ~rbit; }

            if (!(_soft_spi_mode & SOFT_SPI_CPHA) || (size != 0) || (i < 7)) {
                hal_soft_spi_toggle_sclk();
            }
        }

        if(NULL != rx_buff) {
            *rx_buff++ = read;
        }
    }

    rt_exit_critical();
}
#pragma GCC pop_options

void hal_soft_spi_init(int8_t sck, int8_t miso, int8_t mosi)
{
    hal_fpioa_set_pin_func(sck,  FUNC_GPIO0 + SOFT_SPI_SCLK_GPIO_FUNC);
    hal_fpioa_set_pin_func(miso, FUNC_GPIO0 + SOFT_SPI_MISO_GPIO_FUNC);
    hal_fpioa_set_pin_func(mosi, FUNC_GPIO0 + SOFT_SPI_MOSI_GPIO_FUNC);

    gpio_set_drive_mode(SOFT_SPI_SCLK_GPIO_FUNC, GPIO_DM_OUTPUT);
    gpio_set_drive_mode(SOFT_SPI_MISO_GPIO_FUNC, GPIO_DM_INPUT);
    gpio_set_drive_mode(SOFT_SPI_MOSI_GPIO_FUNC, GPIO_DM_OUTPUT);

    if(_soft_spi_mode & SOFT_SPI_CPOL) {
        _soft_spi_sclk_sts = 1;
        GPIO_OUT_HIGH(SOFT_SPI_SCLK_GPIO_FUNC);
    } else {
        _soft_spi_sclk_sts = 0;
        GPIO_OUT_LOWX(SOFT_SPI_SCLK_GPIO_FUNC);
    }
}
