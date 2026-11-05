// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "Print.h"
#include "k210_format.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

size_t Print::write(const uint8_t* data, size_t count)
{
    if (!data)
        return 0;
    size_t accepted = 0;
    for (; accepted < count; ++accepted) {
        if (write(data[accepted]) != 1) {
            setWriteError();
            break;
        }
    }
    return accepted;
}
size_t Print::write(const char* text)
{
    return text ? write(text, strlen(text)) : 0;
}
size_t Print::write(const char* text, size_t count)
{
    return write(reinterpret_cast<const uint8_t*>(text), count);
}
size_t Print::print(const char* text)
{
    return write(text);
}
size_t Print::print(const __FlashStringHelper* text)
{
    return write(reinterpret_cast<const char*>(text));
}
size_t Print::print(const String& text)
{
    return write(text.c_str(), text.length());
}
size_t Print::print(const Printable& value)
{
    return value.printTo(*this);
}
size_t Print::print(char value)
{
    return write(static_cast<uint8_t>(value));
}
size_t Print::println()
{
    return write("\r\n", 2);
}

template <class Integer> size_t Print::number(Integer value, int radix)
{
    if (radix == 0)
        return write(static_cast<uint8_t>(value));
    char         text[66];
    const size_t count = k210_compat::integer_text(text, value, radix);
    return write(text, count);
}
size_t Print::print(unsigned char value, int base)
{
    return number(static_cast<unsigned long>(value), base);
}
size_t Print::print(int value, int base)
{
    return number(static_cast<long>(value), base);
}
size_t Print::print(unsigned int value, int base)
{
    return number(static_cast<unsigned long>(value), base);
}
size_t Print::print(long value, int base)
{
    return number(value, base);
}
size_t Print::print(unsigned long value, int base)
{
    return number(value, base);
}
size_t Print::print(long long value, int base)
{
    return number(value, base);
}
size_t Print::print(unsigned long long value, int base)
{
    return number(value, base);
}
size_t Print::print(double value, int decimals)
{
    // Bit classification works even when platform.txt enables -ffast-math.
    uint64_t bits;
    static_assert(sizeof(bits) == sizeof(value), "64-bit double required");
    memcpy(&bits, &value, sizeof(bits));
    if ((bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000))
        return write((bits & UINT64_C(0xfffffffffffff)) ? "nan" : "inf");
    if (value > 4294967040.0 || value < -4294967040.0)
        return write("ovf");
    if (decimals < 0)
        decimals = 0;
    if (decimals > 255)
        decimals = 255;
    char      text[280];
    const int count = snprintf(text, sizeof(text), "%.*f", decimals, value == 0 ? 0.0 : value);
    if (count < 0 || static_cast<size_t>(count) >= sizeof(text)) {
        setWriteError();
        return 0;
    }
    return write(text, count);
}
size_t Print::printf(const char* format, ...)
{
    if (!format)
        return 0;
    char    local[96];
    va_list arguments, replay;
    va_start(arguments, format);
    va_copy(replay, arguments);
    const int count = vsnprintf(local, sizeof(local), format, arguments);
    va_end(arguments);
    if (count < 0) {
        va_end(replay);
        setWriteError();
        return 0;
    }
    char* text = local;
    if (static_cast<size_t>(count) >= sizeof(local)) {
        text = static_cast<char*>(malloc(static_cast<size_t>(count) + 1));
        if (!text) {
            va_end(replay);
            setWriteError();
            return 0;
        }
        vsnprintf(text, static_cast<size_t>(count) + 1, format, replay);
    }
    va_end(replay);
    const size_t accepted = write(text, count);
    if (text != local)
        free(text);
    return accepted;
}
