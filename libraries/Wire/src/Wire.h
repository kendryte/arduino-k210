/*
  Copyright (c) 2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "Arduino.h"
#include <inttypes.h>
#include "Stream.h"
#include "RingBuffer.h"

// namespace arduino {

#define HARDWARE_I2C_BUFF_LEN   (64)

typedef void(*user_onRequest)(void);
typedef void(*user_onReceive)(uint8_t*, int);

class TwoWire : public Stream
{
  protected:
    uint32_t _clockFreq;

    int8_t _sclPin, _sdaPin;

    uint16_t _txAddress;

    RingBufferN<HARDWARE_I2C_BUFF_LEN> *_txBuff, *_rxBuff;

    bool _nonStop;
    rt_thread_t _nonStopTask;
    rt_mutex_t _lock;

  private:
    bool _initialized;
    bool _isSlave;
    bool _slaveTransmitting;
    void (*user_onRequest)(void);
    void (*user_onReceive)(int);

  public:
    // for slave irq access these methods.
    i2c_device_number_t _i2cNum;
    void _on_event(i2c_event_t event);
    void _on_receive(uint32_t data);
    uint32_t _on_transmit(void);

    TwoWire(int busNum);
    virtual ~TwoWire();

    bool begin(int8_t sclPin, int8_t sdaPin, uint32_t freq = 100000);
    bool begin(uint16_t address, int8_t sclPin, int8_t sdaPin, uint32_t freq);
    // Explicit Overload for Arduino MainStream API compatibility
    inline bool begin()
    {
        return begin(-1, -1, static_cast<uint32_t>(0));
    }
    inline bool begin(uint8_t addr)
    {
        return begin(addr, -1, -1, 0);
    }
    inline bool begin(int addr)
    {
        return begin(static_cast<uint8_t>(addr), -1, -1, 0);
    }
    bool end();

    void setClock(uint32_t freq);
  
    void beginTransmission(uint16_t address);
    void beginTransmission(uint8_t address);
    void beginTransmission(int address);

    uint8_t endTransmission(bool sendStop);
    uint8_t endTransmission(void);

    size_t requestFrom(uint16_t address, size_t len, bool sendStop);
    size_t requestFrom(uint8_t address, size_t len, bool sendStop);
    uint8_t requestFrom(uint8_t address, uint8_t size, uint8_t sendStop);
    uint8_t requestFrom(uint16_t address, uint8_t size, bool sendStop);
    uint8_t requestFrom(uint16_t address, uint8_t size, uint8_t sendStop);
    uint8_t requestFrom(uint8_t address, uint8_t size);
    uint8_t requestFrom(uint16_t address, uint8_t size);
    uint8_t requestFrom(int address, int size);
    uint8_t requestFrom(int address, int size, int sendStop);

    void onReceive(void(*)(int));
    void onRequest(void(*)(void));

    size_t write(uint8_t); 
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write;

    int available();
    int read();
    int peek();
    void flush();
};

extern TwoWire Wire;

// }

// using arduino::TwoWire;
