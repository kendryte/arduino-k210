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

#include <inttypes.h>
#include "Stream.h"

#include "rtthread.h"

#include "k210-hal.h"

// namespace arduino {

class HardwareSerial : public Stream
{
  public:
    HardwareSerial(int uartNum);
    ~HardwareSerial();

    void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1);
    void end();

    int available(void);

    // rtt serial driver not support.
    int peek(void) { return -1; }
    void flush(void) {}

    int read(void);
    size_t read(uint8_t *buffer, size_t size);
    inline size_t read(char * buffer, size_t size) {
        return read((uint8_t*) buffer, size);
    }

    size_t write(uint8_t);
    size_t write(const uint8_t *buffer, size_t size);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    int availableForWrite() { return 0; }

    operator bool() const;

  protected:
    int _uartNum;
    hal_uart_t *_uart;
    size_t _bufferSize;

    int8_t _rxPin, _txPin;
};

extern HardwareSerial Serial;

// XXX: Are we keeping the serialEvent API?
extern void serialEventRun(void) __attribute__((weak));

// }

// using arduino::HardwareSerial;
