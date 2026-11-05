// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Stream.h"
#include "k210-hal.h"
#include "rtthread.h"
#include <stddef.h>
#include <stdint.h>

// Interface to the existing K210 UART HAL implementation.
class HardwareSerial : public Stream {
public:
    using OnDataRecvCallback = void (*)(HardwareSerial*);
    explicit HardwareSerial(int uartNum);
    ~HardwareSerial();
    void   begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1);
    void   end();
    void   onReceive(OnDataRecvCallback callback);
           operator bool() const;
    int    available() override;
    int    read() override;
    size_t read(uint8_t* buffer, size_t size);
    size_t read(char* buffer, size_t size)
    {
        return read(reinterpret_cast<uint8_t*>(buffer), size);
    }
    int peek() override
    {
        return -1;
    } // UART HAL currently has no non-consuming read.
    void flush() override { }
    int  availableForWrite() override
    {
        return 0;
    }
    using Print::write;
    size_t write(uint8_t value) override;
    size_t write(const uint8_t* buffer, size_t size) override;

protected:
    static void        onDateRecvCallBack(void* context);
    int                _uartNum;
    hal_uart_t*        _uart;
    size_t             _bufferSize;
    int8_t             _rxPin, _txPin;
    OnDataRecvCallback _onRecvCallback;
};
extern HardwareSerial Serial, Serial1, Serial2;
void                  serialEventRun() __attribute__((weak));
