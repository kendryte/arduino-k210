#define DBG_TAG "Serial"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Arduino.h"

// namespace arduino {

HardwareSerial Serial(0);

HardwareSerial::HardwareSerial(int uartNum) : 
_uart(NULL),
_bufferSize(256),
_rxPin(-1),
_txPin(-1) 
{
    if(uartNum >= /* UART_DEVICE_MAX */ 3) {
        LOG_E("HardwareSerial Invaild uart_num %d\n", uartNum);
        return;
    }
    _uartNum = uartNum;
}

HardwareSerial::~HardwareSerial()
{
    end();
}

void HardwareSerial::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin)
{
    if(0 == _uartNum) { // UART_DEVICE_1
        if (rxPin < 0 && txPin < 0) {
            rxPin = UART1_RX;
            txPin = UART1_TX;
        }
    }

    if (rxPin < 0 && txPin < 0) {
        LOG_E("Invaild rxPin(%d) or txPin(%d)\n", rxPin, txPin);
        return;
    }

    _rxPin = rxPin;
    _txPin = txPin;

    _uart = hal_uart_begin(_uartNum, baud, config, rxPin, txPin, _bufferSize);
}

void HardwareSerial::end()
{
    hal_uart_end(_uart);
    _uart = 0;

    hal_fpioa_clr_pin_func(_rxPin);
    hal_fpioa_clr_pin_func(_txPin);
}

int HardwareSerial::available(void)
{
    return hal_uart_available(_uart);
}

int HardwareSerial::read(void)
{
    if(available() > 0) {
        return hal_uart_read_one(_uart);
    }

    return -1;
}

// read characters into buffer
// terminates if size characters have been read, or no further are pending
// returns the number of characters placed in the buffer
// the buffer is NOT null terminated.
size_t HardwareSerial::read(uint8_t *buffer, size_t size)
{
    return hal_uart_read_to_buffer(_uart, buffer, size);
}

size_t HardwareSerial::write(uint8_t c)
{
    return hal_uart_write_one(_uart, c);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size)
{
    return hal_uart_write_from_buffer(_uart, buffer, size);
}

HardwareSerial::operator bool() const
{
    return hal_uart_is_opened(_uart);
}

// }

extern "C"
{
    char _SerialPutChar(char c)
    {
        if(Serial)
        {
            Serial.write(c);
            return (c & 0xFF);
        }

        return 0;
    }
}
