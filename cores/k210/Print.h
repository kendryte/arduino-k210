// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Printable.h"
#include "WString.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#define BIN 2
#define OCT 8
#define DEC 10
#define HEX 16

class Print {
public:
    virtual ~Print()                    = default;
    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t* data, size_t count);
    size_t         write(const char* text);
    size_t         write(const char* text, size_t count);
    virtual int    availableForWrite()
    {
        return 0;
    }
    virtual void flush() { }
    int          getWriteError() const
    {
        return error_;
    }
    void clearWriteError()
    {
        error_ = 0;
    }
    size_t                    printf(const char* format, ...) __attribute__((format(printf, 2, 3)));
    size_t                    print(const char* text);
    size_t                    print(const __FlashStringHelper* text);
    size_t                    print(const String& text);
    size_t                    print(const Printable& value);
    size_t                    print(char value);
    size_t                    print(unsigned char value, int base = DEC);
    size_t                    print(int value, int base = DEC);
    size_t                    print(unsigned int value, int base = DEC);
    size_t                    print(long value, int base = DEC);
    size_t                    print(unsigned long value, int base = DEC);
    size_t                    print(long long value, int base = DEC);
    size_t                    print(unsigned long long value, int base = DEC);
    size_t                    print(double value, int decimals = 2);
    size_t                    println();
    template <class T> size_t println(const T& value)
    {
        const size_t count = print(value);
        return count + println();
    }
    template <class T> size_t println(T value, int format)
    {
        const size_t count = print(value, format);
        return count + println();
    }

protected:
    void setWriteError(int error = 1)
    {
        error_ = error;
    }

private:
    int                             error_ = 0;
    template <class Integer> size_t number(Integer value, int radix);
};
