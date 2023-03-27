#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include "Arduino.h"

#define SPI_HAS_TRANSACTION

// namespace arduino {

typedef enum {
  SPI_MODE0 = SPI_WORK_MODE_0,
  SPI_MODE1 = SPI_WORK_MODE_1,
  SPI_MODE2 = SPI_WORK_MODE_2,
  SPI_MODE3 = SPI_WORK_MODE_3,
} SPIMode;

typedef enum {
  SPI_LSBFIRST,
  SPI_MSBFIRST
} SPIBitOrder;

typedef enum {
  SPI_BUS_0 = SPI_DEVICE_0, // general for lcd
  SPI_BUS_1 = SPI_DEVICE_1,
  // SPI_BUS_2 = SPI_DEVICE_2, // spi slave device, not support now
  // SPI_BUS_3 = SPI_DEVICE_3, // for system flash, user can not modify pin configure
  SPI_BUS_MAX = 255
} SPIBUS;

class SPISettings
{
public:
    SPISettings() :_clock(1000000), _bitOrder(SPI_MSBFIRST), _dataMode(SPI_MODE0) {}
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) :_clock(clock), _bitOrder(bitOrder), _dataMode(dataMode) {}
    uint32_t _clock;
    uint8_t  _bitOrder;
    uint8_t  _dataMode;
};

class SPIClass
{
public:
    SPIClass(uint8_t spiBus = SPI_BUS_1);
    ~SPIClass();

    /// @brief SPI begin
    /// @param sckPin Spi SCLK pin
    /// @param misoPin Spi MISO pin, -1 not used
    /// @param mosiPin Spi MOSI pin
    /// @param csPin Spi CS pin, -1 not used
    void begin(int8_t sckPin, int8_t misoPin, int8_t mosiPin, int8_t csPin);
    inline void begin() {
        return begin(-1, -1, -1, -1);
    }
    void end();

    void setBitOrder(uint8_t bitOrder);
    void setDataMode(uint8_t dataMode);
    void setFrequency(uint32_t freq);

    void beginTransaction(SPISettings settings);
    void endTransaction(void);
    void transfer(void * data, uint32_t size);
    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    uint32_t transfer32(uint32_t data);

    void transferBytes(const uint8_t * data, uint8_t * out, uint32_t size);
    // void transferBits(uint32_t data, uint32_t * out, uint8_t bits);

    void write(uint8_t data);
    void write16(uint16_t data);
    void write32(uint32_t data);
    void writeBytes(const uint8_t * data, uint32_t size);
    // void writePixels(const void * data, uint32_t size);//ili9341 compatible
    // void writePattern(const uint8_t * data, uint8_t size, uint32_t repeat);

private:
    bool _initialized;

    SPIBUS _spiBus;
    int8_t _sckPin, _mosiPin, _misoPin, _csPin;

    spi_chip_select_t _hwCS;

    bool _inTransaction;
    rt_mutex_t _paramLock;

    SPISettings _setting;
};

extern SPIClass SPI;

// }

// using arduino::SPIClass;

#endif
