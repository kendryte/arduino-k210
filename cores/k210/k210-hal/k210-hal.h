#pragma once

#include "k210_bsp.h"

#include "k210_aes.h"
#include "k210_clint.h"
#include "k210_dmac.h"
#include "k210_dvp.h"
#include "k210_fft.h"
#include "k210_fpioa.h"
#include "k210_gpio_common.h"
#include "k210_gpio.h"
#include "k210_gpiohs.h"
#include "k210_i2c.h"
#include "k210_i2s.h"
#include "k210_io.h"
#include "k210_plic.h"
#include "k210_pwm.h"
#include "k210_rtc.h"
#include "k210_sha256.h"
#include "k210_spi.h"
#include "k210_sysctl.h"
#include "k210_timer.h"
#include "k210_uart.h"
#include "k210_uarths.h"
#include "k210_utils.h"
#include "k210_wdt.h"

#include "k210-hal-dma.h"
#include "k210-hal-flash.h"
#include "k210-hal-fpioa.h"
#include "k210-hal-gpio.h"
#include "k210-hal-soft-spi.h"
#include "k210-hal-tft-bus.h"
#include "k210-hal-timer.h"
#include "k210-hal-uart.h"

// Internal func use gpio, gpiohs is for user.

// TODO: optimize USE one dma chn.
#define FLASH_WRITE_DMA_CHN         (DMAC_CHANNEL4)
#define FLASH_READ_DMA_CHN          (DMAC_CHANNEL5)

#define TFT_DCX_GPIO_FUNC           (0)

#define SOFT_SPI_SCLK_GPIO_FUNC     (1)
#define SOFT_SPI_MISO_GPIO_FUNC     (2)
#define SOFT_SPI_MOSI_GPIO_FUNC     (3)
#define SOFT_SPI_CSXX_GPIO_FUNC     (4)

