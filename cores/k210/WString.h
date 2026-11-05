// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "pgmspace.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

class __FlashStringHelper;
#ifndef FPSTR
#define FPSTR(text) reinterpret_cast<const __FlashStringHelper*>(text)
#endif
#ifndef F
#define F(text) FPSTR(PSTR(text))
#endif

// Value type with explicit allocation-failure handling and a small local buffer.
class String {
public:
    String(const char* text = "");
    String(const char* text, unsigned int count);
    String(const uint8_t* text, unsigned int count);
    String(const __FlashStringHelper* text);
    String(const String& other);
    String(String&& other) noexcept;
    ~String();
    explicit String(char value);
    explicit String(unsigned char value, unsigned char base = 10);
    explicit String(int value, unsigned char base = 10);
    explicit String(unsigned int value, unsigned char base = 10);
    explicit String(long value, unsigned char base = 10);
    explicit String(unsigned long value, unsigned char base = 10);
    explicit String(long long value, unsigned char base = 10);
    explicit String(unsigned long long value, unsigned char base = 10);
    explicit String(float value, unsigned int decimals = 2);
    explicit String(double value, unsigned int decimals = 2);
    String&  operator=(const String& other);
    String&  operator=(String&& other) noexcept;
    String&  operator=(const char* text);
    String&  operator=(const __FlashStringHelper* text);
    explicit operator bool() const
    {
        return valid_;
    }
    unsigned int length() const
    {
        return size_;
    }
    bool isEmpty() const
    {
        return size_ == 0;
    }
    unsigned char reserve(unsigned int count);
    void          clear();
    const char*   c_str() const
    {
        return valid_ ? data_ : nullptr;
    }
    char* begin()
    {
        return data_;
    }
    char* end()
    {
        return data_ + size_;
    }
    const char* begin() const
    {
        return data_;
    }
    const char* end() const
    {
        return data_ + size_;
    }
    unsigned char concat(const char* text, unsigned int count);
    unsigned char concat(const uint8_t* text, unsigned int count)
    {
        return concat(reinterpret_cast<const char*>(text), count);
    }
    unsigned char              concat(const char* text);
    unsigned char              concat(const __FlashStringHelper* text);
    unsigned char              concat(const String& text);
    unsigned char              concat(char value);
    unsigned char              concat(unsigned char value);
    unsigned char              concat(int value);
    unsigned char              concat(unsigned int value);
    unsigned char              concat(long value);
    unsigned char              concat(unsigned long value);
    unsigned char              concat(long long value);
    unsigned char              concat(unsigned long long value);
    unsigned char              concat(float value);
    unsigned char              concat(double value);
    template <class T> String& operator+=(const T& value)
    {
        concat(value);
        return *this;
    }
    int           compareTo(const String& other) const;
    unsigned char equals(const String& other) const;
    unsigned char equals(const char* text) const;
    unsigned char equalsIgnoreCase(const String& other) const;
    unsigned char equalsConstantTime(const String& other) const;
    bool          operator==(const String& other) const
    {
        return equals(other);
    }
    bool operator==(const char* text) const
    {
        return equals(text);
    }
    bool operator!=(const String& other) const
    {
        return !equals(other);
    }
    bool operator!=(const char* text) const
    {
        return !equals(text);
    }
    bool operator<(const String& other) const
    {
        return compareTo(other) < 0;
    }
    bool operator>(const String& other) const
    {
        return compareTo(other) > 0;
    }
    bool operator<=(const String& other) const
    {
        return compareTo(other) <= 0;
    }
    bool operator>=(const String& other) const
    {
        return compareTo(other) >= 0;
    }
    unsigned char startsWith(const String& prefix, unsigned int offset = 0) const;
    unsigned char endsWith(const String& suffix) const;
    char          charAt(unsigned int index) const;
    void          setCharAt(unsigned int index, char value);
    char          operator[](unsigned int index) const
    {
        return charAt(index);
    }
    char& operator[](unsigned int index);
    void  getBytes(unsigned char* output, unsigned int capacity, unsigned int index = 0) const;
    void  toCharArray(char* output, unsigned int capacity, unsigned int index = 0) const
    {
        getBytes(reinterpret_cast<unsigned char*>(output), capacity, index);
    }
    int    indexOf(char value, unsigned int start = 0) const;
    int    indexOf(const String& value, unsigned int start = 0) const;
    int    lastIndexOf(char value, unsigned int start = UINT_MAX) const;
    int    lastIndexOf(const String& value, unsigned int start = UINT_MAX) const;
    String substring(unsigned int first, unsigned int last = UINT_MAX) const;
    void   replace(char before, char after);
    void   replace(const String& before, const String& after);
    void   remove(unsigned int first, unsigned int count = UINT_MAX);
    void   toLowerCase();
    void   toUpperCase();
    void   trim();
    long   toInt() const;
    float  toFloat() const;
    double toDouble() const;

private:
    char         local_[24] = {};
    char*        data_      = local_;
    unsigned int size_      = 0;
    unsigned int capacity_  = sizeof(local_) - 1;
    bool         valid_     = true;
    char         discarded_ = 0;
    void         assign(const char* text, unsigned int count);
    void         resetInvalid();
    void         take(String& other) noexcept;
};

// Concatenation returns an owning value, never a reference to a temporary.
using StringSumHelper = String;
template <class T> String operator+(String left, const T& right)
{
    if (!left.concat(right))
        return String(static_cast<const char*>(nullptr));
    return left;
}
template <class T> typename std::enable_if<std::is_arithmetic<T>::value, String>::type operator+(T left, const String& right)
{
    return String(left) + right;
}
inline bool operator==(const char* left, const String& right)
{
    return right.equals(left);
}
inline bool operator!=(const char* left, const String& right)
{
    return !right.equals(left);
}
extern const String emptyString;
