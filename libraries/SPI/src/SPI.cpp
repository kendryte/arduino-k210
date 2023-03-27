/* 
 SPI.cpp - SPI library for esp8266

 Copyright (c) 2015 Hristo Gochkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#define DBG_TAG "SPI"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "Arduino.h"

#include "SPI.h"

#define SPI_PARAM_LOCK()    do {} while (RT_EOK != rt_mutex_take(_paramLock, RT_WAITING_FOREVER))
#define SPI_PARAM_UNLOCK()  rt_mutex_release(_paramLock)

extern uint8_t _sdcard_use_hw_spi1;

struct spi_cs_table {
    SPIBUS bus;
    bool func_stat[ SPI_CHIP_SELECT_MAX ];
};

static struct spi_cs_table _spi_cs_tab[] = {
    { SPI_BUS_0 ,       {false, false, false, false}},
    { SPI_BUS_1 ,       {false, false, false, false}},
    // { SPI_BUS_2 ,       {false, false, false, false}}, /* Not support. */
    // { SPI_BUS_3 ,       {false, false, false, false}}, /* Not support. */
};

#define SPI_CS_TAB_SIZE sizeof(_spi_cs_tab) / sizeof(_spi_cs_tab[0])

static inline void spi_release_cs_func(SPIBUS bus, spi_chip_select_t func)
{
    if(func < SPI_CHIP_SELECT_MAX )
        _spi_cs_tab[bus].func_stat[func] = false;
}

static spi_chip_select_t spi_acquire_cs_func(SPIBUS bus)
{
    for(int i = 0; i < SPI_CS_TAB_SIZE ; i++) {
        if(bus == _spi_cs_tab[i].bus) {
            for(int j = 0; j < SPI_CHIP_SELECT_MAX ; j++) {
                if(false == _spi_cs_tab[i].func_stat[j]) {
                    _spi_cs_tab[i].func_stat[j] = true;
                    return spi_chip_select_t(j);
                }
            }
        }
    }

    return SPI_CHIP_SELECT_MAX;
}

SPIClass::SPIClass(uint8_t spiBus)
    :_sckPin(-1)
    ,_mosiPin(-1)
    ,_misoPin(-1)
    ,_csPin(-1)
    ,_inTransaction(false)
    ,_paramLock(NULL)
    ,_setting(SPISettings())
{
    char _lock_name[RT_NAME_MAX];

    snprintf(_lock_name, sizeof(_lock_name), "%dPspi", spiBus);

    if(NULL == _paramLock) {
        _paramLock = rt_mutex_create(_lock_name, RT_NULL);
        if(NULL == _paramLock) {
            LOG_E("rt_mutex_create failed.\n");
            return;
        }
    }

    _spiBus = SPIBUS(spiBus);
}

SPIClass::~SPIClass()
{
    end();

    if(NULL != _paramLock) {
        rt_mutex_delete(_paramLock);
        _paramLock = NULL;
    }
}

void SPIClass::begin(int8_t sckPin, int8_t misoPin, int8_t mosiPin, int8_t csPin)
{
    bool failed = false;

    if(_sdcard_use_hw_spi1 && (SPI_BUS_1 == _spiBus)) {
        LOG_E("SPI1 already used for sdcard!");
        return;
    }

    if(g_spi0_in_use && (SPI_BUS_0 == _spiBus)) {
        LOG_E("SPI0 already used for LCD\n");
        return;
    }

    if(_initialized) {
        LOG_W("Bus already started.");
        return;
    }

    if((-1 == sckPin) /* || (-1 == misoPin) */ || (-1 == mosiPin) /* || (-1 == csPin) */) {
        LOG_E("Invaild Pins\n");
        return;
    }

    SPI_PARAM_LOCK();

    _sckPin = sckPin;
    _misoPin = misoPin;
    _mosiPin = mosiPin;
    _csPin = csPin;

    fpioa_function_t sck_fuc = FUNC_MAX , mosi_func = FUNC_MAX , miso_func = FUNC_MAX , cs_func = FUNC_MAX ;

    spi_chip_select_t chip_sel = spi_acquire_cs_func(_spiBus);

    if(SPI_CHIP_SELECT_MAX <= chip_sel) {
        LOG_E("Invaild cs_func %d\n", chip_sel);
        failed = true;
        goto _fail;
    }
    _hwCS = chip_sel;

    switch (_spiBus) {
        case SPI_BUS_0: {
            sck_fuc = FUNC_SPI0_SCLK ;
            mosi_func = FUNC_SPI0_D0 ;
            miso_func = FUNC_SPI0_D1 ;
            cs_func = fpioa_function_t( FUNC_SPI0_SS0 + _hwCS);
            break;
        } case SPI_BUS_1: {
            sck_fuc = FUNC_SPI1_SCLK ;
            mosi_func = FUNC_SPI1_D0 ;
            miso_func = FUNC_SPI1_D1 ;
            cs_func = fpioa_function_t( FUNC_SPI1_SS0 + _hwCS);
            break;
        // } case SPI_BUS_2: {
        //     break;
        // } case SPI_BUS_3: {
        //     break;
        } default: {
            LOG_E("Invaild spi_bus %d\n", _spiBus);
            failed = true;
            goto _fail;
            break;
        }
    }

    hal_fpioa_set_pin_func(_sckPin, sck_fuc);
    hal_fpioa_set_pin_func(_mosiPin, mosi_func);

    if(0 <= _misoPin) {
        hal_fpioa_set_pin_func(_misoPin, miso_func);
    }

    if(0 <= _csPin) {
        hal_fpioa_set_pin_func(_csPin, cs_func);
    }

    _initialized = true;

_fail:
    if(failed) {
        LOG_E("Init failed");

        _initialized = false;

        hal_fpioa_clr_pin_func(_sckPin);
        hal_fpioa_clr_pin_func(_mosiPin);

        if(0 <= _misoPin) {
            hal_fpioa_clr_pin_func(_misoPin);
        }

        if(0 <= _csPin) {
            hal_fpioa_clr_pin_func(_csPin);
        }
        spi_release_cs_func(_spiBus, _hwCS);
    }

    SPI_PARAM_UNLOCK();
}

void SPIClass::end()
{
    SPI_PARAM_LOCK();

    _initialized = false;

    if(spi_device_num_t(_spiBus) < SPI_DEVICE_MAX)
        sysctl_clock_disable(sysctl_clock_t(SYSCTL_CLOCK_SPI0 + _spiBus));

    // free pins
    hal_fpioa_clr_pin_func(_sckPin);
    hal_fpioa_clr_pin_func(_mosiPin);

    if(0 <= _misoPin) {
        hal_fpioa_clr_pin_func(_misoPin);
    }

    if(0 <= _csPin) {
        hal_fpioa_clr_pin_func(_csPin);
    }

    spi_release_cs_func(_spiBus, _hwCS);

    SPI_PARAM_UNLOCK();
}

void SPIClass::setFrequency(uint32_t freq)
{
    SPI_PARAM_LOCK();

    if(_setting._clock != freq) {
        _setting._clock = freq;

        spi_init(spi_device_num_t(_spiBus), spi_work_mode_t(_setting._dataMode), SPI_FF_STANDARD, 8, /* settings._bitOrder */ 0);
        spi_set_clk_rate(spi_device_num_t(_spiBus), _setting._clock);
    }

    SPI_PARAM_UNLOCK();
}

void SPIClass::setDataMode(uint8_t dataMode)
{
    SPI_PARAM_LOCK();

    if(_setting._dataMode != dataMode) {
        _setting._dataMode = dataMode;

        spi_init(spi_device_num_t(_spiBus), spi_work_mode_t(_setting._dataMode), SPI_FF_STANDARD, 8, /* settings._bitOrder */ 0);
    }

    SPI_PARAM_UNLOCK();
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
    SPI_PARAM_LOCK();

    if(_setting._bitOrder != bitOrder) {
        _setting._bitOrder = bitOrder;

        spi_init(spi_device_num_t(_spiBus), spi_work_mode_t(_setting._dataMode), SPI_FF_STANDARD, 8, /* settings._bitOrder */ 0);
    }

    SPI_PARAM_UNLOCK();
}

void SPIClass::beginTransaction(SPISettings settings)
{
    SPI_PARAM_LOCK();

    if((_setting._clock != settings._clock) || (_setting._dataMode != settings._dataMode) || (_setting._bitOrder != settings._bitOrder)) {
        _setting._clock = settings._clock;
        _setting._dataMode = settings._dataMode;
        _setting._bitOrder = settings._bitOrder;

        spi_init(spi_device_num_t(_spiBus), spi_work_mode_t(_setting._dataMode), SPI_FF_STANDARD, 8, /* settings._bitOrder */ 0);
    }

    _inTransaction = true;
}

void SPIClass::endTransaction()
{
    if(_inTransaction) {
        _inTransaction = false;

        SPI_PARAM_UNLOCK(); // <-- Im not sure should it be here or right after spiTransaction()
    }
}

void SPIClass::write(uint8_t data)
{
    uint8_t t = data;

    spi_send_data_normal(spi_device_num_t(_spiBus), _hwCS, &t, 1);
}

uint8_t SPIClass::transfer(uint8_t data)
{
    uint8_t t = data, r = 0xFF;

    spi_transfer_data_standard(spi_device_num_t(_spiBus), _hwCS, &t, &r, 1);

    return r;
}

void SPIClass::write16(uint16_t data)
{
    uint8_t t[2];
    uint16_t d = data;

    t[0] = ((uint8_t*)&d)[1];
    t[1] = ((uint8_t*)&d)[0];

    spi_send_data_normal(spi_device_num_t(_spiBus), _hwCS, t, 2);
}

uint16_t SPIClass::transfer16(uint16_t data)
{
    uint8_t t[2], r[2];
    uint16_t d = data;

    // __builtin_bswap16 ?
    t[0] = ((uint8_t*)&d)[1];
    t[1] = ((uint8_t*)&d)[0];

    spi_transfer_data_standard(spi_device_num_t(_spiBus), _hwCS, t, r, 2);

    ((uint8_t*)&d)[1] = r[0];
    ((uint8_t*)&d)[0] = r[1];

    return d;
}

void SPIClass::write32(uint32_t data)
{
    uint8_t t[4];
    uint32_t d = data;

    // __builtin_bswap32 ?
    t[0] = ((uint8_t*)&d)[3];
    t[1] = ((uint8_t*)&d)[2];
    t[2] = ((uint8_t*)&d)[1];
    t[3] = ((uint8_t*)&d)[0];

    spi_send_data_normal(spi_device_num_t(_spiBus), _hwCS, t, 2);
}

uint32_t SPIClass::transfer32(uint32_t data)
{
    uint8_t t[4], r[4];
    uint32_t d = data;

    t[0] = ((uint8_t*)&d)[3];
    t[1] = ((uint8_t*)&d)[2];
    t[2] = ((uint8_t*)&d)[1];
    t[3] = ((uint8_t*)&d)[0];

    spi_transfer_data_standard(spi_device_num_t(_spiBus), _hwCS, t, r, 4);

    ((uint8_t*)&d)[0] = r[3];
    ((uint8_t*)&d)[1] = r[2];
    ((uint8_t*)&d)[2] = r[1];
    ((uint8_t*)&d)[3] = r[0];

    return d;
}

void SPIClass::transfer(void * data, uint32_t size) 
{ 
    transferBytes((const uint8_t *)data, (uint8_t *)data, size); 
}

/**
 * @param data uint8_t * data buffer. can be NULL for Read Only operation
 * @param out  uint8_t * output buffer. can be NULL for Write Only operation
 * @param size uint32_t
 */
void SPIClass::transferBytes(const uint8_t * data, uint8_t * out, uint32_t size)
{
    spi_transfer_data_standard(spi_device_num_t(_spiBus), _hwCS, data, out, size);
}

void SPIClass::writeBytes(const uint8_t * data, uint32_t size)
{
    spi_send_data_normal(spi_device_num_t(_spiBus), _hwCS, data, size);
}

SPIClass SPI(1);
