// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "WString.h"
#include "k210_format.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

const String emptyString;
String::String(const char* text)
{
    assign(text, text ? strlen(text) : 0);
}
String::String(const char* text, unsigned int count)
{
    assign(text, count);
}
String::String(const uint8_t* text, unsigned int count)
    : String(reinterpret_cast<const char*>(text), count)
{
}
String::String(const __FlashStringHelper* text)
    : String(reinterpret_cast<const char*>(text))
{
}
String::String(const String& other)
{
    assign(other.c_str(), other.size_);
}
String::String(String&& other) noexcept
{
    take(other);
}
String::~String()
{
    if (data_ != local_)
        free(data_);
}
String::String(char value)
{
    assign(&value, 1);
}
String::String(unsigned char value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(int value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(unsigned int value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(long value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(unsigned long value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(long long value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(unsigned long long value, unsigned char base)
{
    char text[66];
    assign(text, k210_compat::integer_text(text, value, base));
}
String::String(float value, unsigned int decimals)
    : String(static_cast<double>(value), decimals)
{
}
String::String(double value, unsigned int decimals)
{
    if (decimals > 255) {
        resetInvalid();
        return;
    }
    const int count = snprintf(nullptr, 0, "%.*f", static_cast<int>(decimals), value);
    if (count < 0 || !reserve(static_cast<unsigned int>(count))) {
        resetInvalid();
        return;
    }
    snprintf(data_, capacity_ + 1, "%.*f", static_cast<int>(decimals), value);
    size_ = count;
}
void String::resetInvalid()
{
    clear();
    valid_ = false;
}
void String::clear()
{
    size_    = 0;
    data_[0] = 0;
}
unsigned char String::reserve(unsigned int count)
{
    if (count == UINT_MAX)
        return 0;
    if (count > capacity_) {
        // Geometric growth keeps character-at-a-time Stream reads amortized linear.
        const unsigned int grown    = capacity_ <= (UINT_MAX - 2) / 2 ? capacity_ * 2 + 1 : UINT_MAX - 1;
        const unsigned int capacity = count > grown ? count : grown;
        char*              next     = static_cast<char*>(malloc(static_cast<size_t>(capacity) + 1));
        if (!next)
            return 0;
        memcpy(next, data_, size_ + 1);
        if (data_ != local_)
            free(data_);
        data_     = next;
        capacity_ = capacity;
    }
    valid_ = true;
    return 1;
}
void String::assign(const char* text, unsigned int count)
{
    if (!text) {
        resetInvalid();
        return;
    }
    // Assignment may refer into our current storage.
    const uintptr_t address  = reinterpret_cast<uintptr_t>(text);
    const uintptr_t base     = reinterpret_cast<uintptr_t>(data_);
    const bool      internal = address >= base && address - base <= size_;
    const size_t    offset   = internal ? address - base : 0;
    if (internal && count > size_ - offset)
        count = size_ - offset;
    if (!reserve(count)) {
        resetInvalid();
        return;
    }
    memmove(data_, internal ? data_ + offset : text, count);
    size_        = count;
    data_[size_] = 0;
}
void String::take(String& other) noexcept
{
    if (other.data_ == other.local_) {
        memcpy(local_, other.local_, sizeof(local_));
        data_     = local_;
        capacity_ = sizeof(local_) - 1;
    } else {
        data_           = other.data_;
        capacity_       = other.capacity_;
        other.data_     = other.local_;
        other.capacity_ = sizeof(other.local_) - 1;
    }
    size_  = other.size_;
    valid_ = other.valid_;
    other.clear();
    other.valid_ = true;
}
String& String::operator=(const String& other)
{
    if (this != &other)
        assign(other.c_str(), other.size_);
    return *this;
}
String& String::operator=(String&& other) noexcept
{
    if (this != &other) {
        if (data_ != local_)
            free(data_);
        take(other);
    }
    return *this;
}
String& String::operator=(const char* text)
{
    assign(text, text ? strlen(text) : 0);
    return *this;
}
String& String::operator=(const __FlashStringHelper* text)
{
    return operator=(reinterpret_cast<const char*>(text));
}
unsigned char String::concat(const char* text, unsigned int count)
{
    if (!text || count > UINT_MAX - size_ - 1)
        return 0;
    const uintptr_t address = reinterpret_cast<uintptr_t>(text), base = reinterpret_cast<uintptr_t>(data_);
    const bool      internal = address >= base && address - base <= size_;
    const size_t    offset   = internal ? address - base : 0;
    if (internal && count > size_ - offset)
        return 0;
    if (!reserve(size_ + count))
        return 0;
    memmove(data_ + size_, internal ? data_ + offset : text, count);
    size_ += count;
    data_[size_] = 0;
    return 1;
}
unsigned char String::concat(const char* text)
{
    return text ? concat(text, strlen(text)) : 0;
}
unsigned char String::concat(const __FlashStringHelper* text)
{
    return concat(reinterpret_cast<const char*>(text));
}
unsigned char String::concat(const String& text)
{
    return concat(text.c_str(), text.size_);
}
unsigned char String::concat(char value)
{
    return concat(&value, 1);
}
unsigned char String::concat(unsigned char value)
{
    return concat(String(value));
}
unsigned char String::concat(int value)
{
    return concat(String(value));
}
unsigned char String::concat(unsigned int value)
{
    return concat(String(value));
}
unsigned char String::concat(long value)
{
    return concat(String(value));
}
unsigned char String::concat(unsigned long value)
{
    return concat(String(value));
}
unsigned char String::concat(long long value)
{
    return concat(String(value));
}
unsigned char String::concat(unsigned long long value)
{
    return concat(String(value));
}
unsigned char String::concat(float value)
{
    return concat(String(value));
}
unsigned char String::concat(double value)
{
    return concat(String(value));
}
int String::compareTo(const String& other) const
{
    const unsigned count  = size_ < other.size_ ? size_ : other.size_;
    const int      result = memcmp(data_, other.data_, count);
    return result ? result : (size_ > other.size_) - (size_ < other.size_);
}
unsigned char String::equals(const String& other) const
{
    return compareTo(other) == 0;
}
unsigned char String::equals(const char* text) const
{
    return text ? size_ == strlen(text) && memcmp(data_, text, size_) == 0 : size_ == 0;
}
unsigned char String::equalsIgnoreCase(const String& other) const
{
    if (size_ != other.size_)
        return 0;
    for (unsigned i = 0; i < size_; ++i)
        if (tolower(static_cast<unsigned char>(data_[i])) != tolower(static_cast<unsigned char>(other.data_[i])))
            return 0;
    return 1;
}
unsigned char String::equalsConstantTime(const String& other) const
{
    if (size_ != other.size_)
        return 0;
    volatile unsigned difference = 0;
    for (unsigned i = 0; i < size_; ++i)
        difference |= static_cast<unsigned char>(data_[i]) ^ static_cast<unsigned char>(other.data_[i]);
    return difference == 0;
}
unsigned char String::startsWith(const String& prefix, unsigned int offset) const
{
    return offset <= size_ && prefix.size_ <= size_ - offset && memcmp(data_ + offset, prefix.data_, prefix.size_) == 0;
}
unsigned char String::endsWith(const String& suffix) const
{
    return suffix.size_ <= size_ && startsWith(suffix, size_ - suffix.size_);
}
char String::charAt(unsigned int index) const
{
    return index < size_ ? data_[index] : 0;
}
void String::setCharAt(unsigned int index, char value)
{
    if (index < size_)
        data_[index] = value;
}
char& String::operator[](unsigned int index)
{
    discarded_ = 0;
    return index < size_ ? data_[index] : discarded_;
}
void String::getBytes(unsigned char* output, unsigned int capacity, unsigned int index) const
{
    if (!output || !capacity)
        return;
    unsigned count = index < size_ ? size_ - index : 0;
    if (count >= capacity)
        count = capacity - 1;
    if (count)
        memcpy(output, data_ + index, count);
    output[count] = 0;
}
int String::indexOf(char value, unsigned int start) const
{
    if (start >= size_)
        return -1;
    const void* found = memchr(data_ + start, static_cast<unsigned char>(value), size_ - start);
    return found ? static_cast<const char*>(found) - data_ : -1;
}
int String::indexOf(const String& value, unsigned int start) const
{
    if (start > size_ || value.size_ > size_ - start)
        return -1;
    for (unsigned pos = start; pos <= size_ - value.size_; ++pos)
        if (startsWith(value, pos))
            return pos;
    return -1;
}
int String::lastIndexOf(char value, unsigned int start) const
{
    if (!size_)
        return -1;
    unsigned pos = start < size_ ? start : size_ - 1;
    do {
        if (data_[pos] == value)
            return pos;
    } while (pos--);
    return -1;
}
int String::lastIndexOf(const String& value, unsigned int start) const
{
    if (value.size_ > size_)
        return -1;
    unsigned pos = start < size_ - value.size_ ? start : size_ - value.size_;
    do {
        if (startsWith(value, pos))
            return pos;
    } while (pos--);
    return -1;
}
String String::substring(unsigned int first, unsigned int last) const
{
    if (first > last)
        std::swap(first, last);
    if (first >= size_)
        return String();
    if (last > size_)
        last = size_;
    return String(data_ + first, last - first);
}
void String::replace(char before, char after)
{
    for (unsigned i = 0; i < size_; ++i)
        if (data_[i] == before)
            data_[i] = after;
}
void String::replace(const String& before, const String& after)
{
    if (!before.size_)
        return;
    String   result;
    unsigned pos = 0;
    while (pos < size_) {
        if (startsWith(before, pos)) {
            if (!result.concat(after))
                return;
            pos += before.size_;
        } else if (!result.concat(data_[pos++]))
            return;
    }
    *this = std::move(result);
}
void String::remove(unsigned int first, unsigned int count)
{
    if (first >= size_)
        return;
    if (count > size_ - first)
        count = size_ - first;
    memmove(data_ + first, data_ + first + count, size_ - first - count + 1);
    size_ -= count;
}
void String::toLowerCase()
{
    for (unsigned i = 0; i < size_; ++i)
        data_[i] = tolower(static_cast<unsigned char>(data_[i]));
}
void String::toUpperCase()
{
    for (unsigned i = 0; i < size_; ++i)
        data_[i] = toupper(static_cast<unsigned char>(data_[i]));
}
void String::trim()
{
    while (size_ && isspace(static_cast<unsigned char>(data_[size_ - 1])))
        data_[--size_] = 0;
    unsigned first = 0;
    while (first < size_ && isspace(static_cast<unsigned char>(data_[first])))
        ++first;
    remove(0, first);
}
long String::toInt() const
{
    return strtol(data_, nullptr, 10);
}
float String::toFloat() const
{
    return strtof(data_, nullptr);
}
double String::toDouble() const
{
    return strtod(data_, nullptr);
}
