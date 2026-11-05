// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Arduino.h"
#include "Stream.h"
#include "k210_i2c.h"

#define HARDWARE_I2C_BUFF_LEN 64
using user_onRequest = void (*)(void);
using user_onReceive = void (*)(uint8_t*, int);

// Master transactions use the permissive K210 SDK. Pin order is SCL, SDA.
// One instance owns each hardware controller. A pending command belongs to the
// task that called beginTransmission; other tasks fail until it is completed
// or cancelled with flush(). Call begin/end/destruction with callers quiescent.
class TwoWire : public Stream {
public:
    explicit TwoWire(int busNum);
    ~TwoWire() override;
    TwoWire(const TwoWire&) = delete;
    TwoWire& operator=(const TwoWire&) = delete;
    bool     begin(int8_t sclPin, int8_t sdaPin, uint32_t frequency = 100000);
    bool     begin(uint16_t address, int8_t sclPin, int8_t sdaPin, uint32_t frequency);
    bool     begin()
    {
        return begin(-1, -1, uint32_t(100000));
    }
    bool begin(uint8_t address)
    {
        return begin(address, -1, -1, 100000);
    }
    bool begin(int address)
    {
        return begin(uint16_t(address), -1, -1, 100000);
    }
    bool end();
    void setClock(uint32_t frequency);
    // Slave mode is unsupported. These compatibility hooks do not register IRQs.
    void     onReceive(void (*)(int)) { }
    void     onRequest(void (*)()) { }
    void     _on_event(i2c_event_t) { }
    void     _on_receive(uint32_t) { }
    uint32_t _on_transmit()
    {
        return 0;
    }
    void beginTransmission(uint16_t address)
    {
        start(address);
    }
    void beginTransmission(uint8_t address)
    {
        start(address);
    }
    void beginTransmission(int address)
    {
        start(address < 0 || address > 1023 ? 1024 : address);
    }
    uint8_t endTransmission(bool stop = true);
    size_t  requestFrom(uint16_t address, size_t count, bool stop)
    {
        return receive(address, count, stop);
    }
    size_t requestFrom(uint8_t address, size_t count, bool stop)
    {
        return receive(address, count, stop);
    }
    size_t requestFrom(uint16_t address, size_t count)
    {
        return receive(address, count, true);
    }
    size_t requestFrom(uint8_t address, size_t count)
    {
        return receive(address, count, true);
    }
    uint8_t requestFrom(uint8_t address, uint8_t count, uint8_t stop)
    {
        return receive(address, count, stop != 0);
    }
    uint8_t requestFrom(uint16_t address, uint8_t count, bool stop)
    {
        return receive(address, count, stop);
    }
    uint8_t requestFrom(uint16_t address, uint8_t count, uint8_t stop)
    {
        return receive(address, count, stop != 0);
    }
    uint8_t requestFrom(uint8_t address, uint8_t count)
    {
        return receive(address, count, true);
    }
    uint8_t requestFrom(uint16_t address, uint8_t count)
    {
        return receive(address, count, true);
    }
    uint8_t requestFrom(int address, int count)
    {
        return requestFrom(address, count, 1);
    }
    uint8_t requestFrom(int address, int count, bool stop)
    {
        return requestFrom(address, count, int(stop));
    }
    uint8_t requestFrom(int address, int count, int stop);
    using Print::write;
    size_t write(uint8_t value) override
    {
        return write(&value, 1);
    }
    size_t                    write(const uint8_t* buffer, size_t count) override;
    int                       available() override;
    int                       peek() override;
    int                       read() override;
    void                      flush() override;
    const i2c_device_number_t _i2cNum;

private:
    void        start(uint16_t address);
    size_t      receive(uint16_t address, size_t count, bool stop);
    bool        accessible() const;
    void        cancel();
    uint8_t     transferStatus(int sdkResult) const;
    rt_mutex_t  mutex_ = nullptr;
    rt_thread_t task_  = nullptr;
    uint32_t    clock_ = 100000;
    int8_t      scl_ = -1, sda_ = -1;
    uint16_t    address_                   = 0;
    uint8_t     tx_[HARDWARE_I2C_BUFF_LEN] = {}, rx_[HARDWARE_I2C_BUFF_LEN] = {};
    size_t      txSize_ = 0, rxSize_ = 0, rxOffset_ = 0;
    enum Phase { Idle, Collecting, CommandPending } phase_ = Idle;
    bool ready_ = false, overflow_ = false;
};
extern TwoWire Wire;
