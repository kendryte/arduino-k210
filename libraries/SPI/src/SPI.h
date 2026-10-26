// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Arduino.h"
#define SPI_HAS_TRANSACTION

enum SPIMode { SPI_MODE0 = 0, SPI_MODE1, SPI_MODE2, SPI_MODE3 };
enum SPIBitOrder { SPI_LSBFIRST = 0, SPI_MSBFIRST = 1 };
enum SPIBUS { SPI_BUS_0 = 0, SPI_BUS_1 = 1, SPI_BUS_MAX = 255 };
class SPISettings {
public:
    SPISettings(uint32_t clock = 1000000, uint8_t order = SPI_MSBFIRST, uint8_t mode = SPI_MODE0)
        : _clock(clock)
        , _bitOrder(order)
        , _dataMode(mode)
    {
    }
    uint32_t _clock;
    uint8_t  _bitOrder, _dataMode;
};
class SPIClass {
public:
    explicit SPIClass(uint8_t bus = SPI_BUS_1);
    ~SPIClass();
    SPIClass(const SPIClass&) = delete;
    SPIClass& operator=(const SPIClass&) = delete;
    void      begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t cs = -1);
    void      end();
    void      setBitOrder(uint8_t order);
    void      setDataMode(uint8_t mode);
    void      setFrequency(uint32_t hz);
    void      beginTransaction(SPISettings settings);
    void      endTransaction();
    uint8_t   transfer(uint8_t value);
    uint16_t  transfer16(uint16_t value);
    uint32_t  transfer32(uint32_t value);
    void      transfer(void* buffer, uint32_t size);
    void      transferBytes(const uint8_t* input, uint8_t* output, uint32_t size);
    void      write(uint8_t value);
    void      write16(uint16_t value);
    void      write32(uint32_t value);
    void      writeBytes(const uint8_t* input, uint32_t size);

private:
    uint8_t     bus_;
    bool        ready_       = false;
    bool        transaction_ = false;
    int8_t      pins_[4]     = { -1, -1, -1, -1 };
    rt_mutex_t  mutex_       = nullptr;
    SPISettings settings_;
    void        configure();
    uint32_t    wordTransfer(uint32_t value, unsigned bytes);
};
extern SPIClass SPI;
