// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "IPAddress.h"
#include "Print.h"
#include <stdio.h>
#include <string.h>

namespace {
bool parseV4(const char* text, uint8_t* output)
{
    if (!text)
        return false;
    for (int octet = 0; octet < 4; ++octet) {
        unsigned value = 0, digits = 0;
        while (*text >= '0' && *text <= '9') {
            value = value * 10 + (*text++ - '0');
            if (++digits > 3 || value > 255)
                return false;
        }
        if (!digits)
            return false;
        output[octet] = value;
        if (octet != 3) {
            if (*text++ != '.')
                return false;
        }
    }
    return *text == 0;
}
int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}
}
const IPAddress IN6ADDR_ANY(IPv6);
const IPAddress INADDR_NONE(UINT32_C(0));
IPAddress::IPAddress() = default;
IPAddress::IPAddress(IPType family)
    : family_(family)
{
}
IPAddress::IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    bytes_[12] = a;
    bytes_[13] = b;
    bytes_[14] = c;
    bytes_[15] = d;
}
IPAddress::IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i,
                     uint8_t j, uint8_t k, uint8_t l, uint8_t m, uint8_t n, uint8_t o, uint8_t p)
    : bytes_ { a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p }
    , family_(IPv6)
{
}
IPAddress::IPAddress(uint32_t address)
{
    *this = address;
}
IPAddress::IPAddress(const uint8_t* address)
    : IPAddress(IPv4, address)
{
}
IPAddress::IPAddress(IPType family, const uint8_t* address)
    : family_(family)
{
    if (address)
        memcpy(raw_address(), address, family == IPv4 ? 4 : 16);
}
IPAddress& IPAddress::operator=(uint32_t address)
{
    family_ = IPv4;
    memset(bytes_, 0, 12);
    memcpy(bytes_ + 12, &address, 4);
    return *this;
}
IPAddress& IPAddress::operator=(const uint8_t* address)
{
    *this = IPAddress(address);
    return *this;
}
IPAddress::operator uint32_t() const
{
    uint32_t value = 0;
    if (family_ == IPv4)
        memcpy(&value, bytes_ + 12, 4);
    return value;
}
bool IPAddress::operator==(const IPAddress& other) const
{
    return family_ == other.family_ && memcmp(bytes_, other.bytes_, 16) == 0;
}
bool IPAddress::operator==(const uint8_t* address) const
{
    return address && family_ == IPv4 && memcmp(bytes_ + 12, address, 4) == 0;
}
uint8_t& IPAddress::operator[](int index)
{
    return raw_address()[index];
}
uint8_t IPAddress::operator[](int index) const
{
    return raw_address()[index];
}
bool IPAddress::fromString(const char* text)
{
    return text && (strchr(text, ':') ? fromString6(text) : fromString4(text));
}
bool IPAddress::fromString4(const char* text)
{
    uint8_t parsed[4];
    if (!parseV4(text, parsed))
        return false;
    *this = IPAddress(parsed);
    return true;
}
bool IPAddress::fromString6(const char* text)
{
    if (!text || !*text)
        return false;
    uint16_t    words[8] = {};
    unsigned    count    = 0;
    int         hole     = -1;
    const char* cursor   = text;
    if (*cursor == ':') {
        if (cursor[1] != ':')
            return false;
        hole = 0;
        cursor += 2;
    }
    while (*cursor) {
        if (count == 8)
            return false;
        const char* start = cursor;
        unsigned    value = 0, digits = 0;
        while (hexValue(*cursor) >= 0) {
            if (++digits > 4)
                return false;
            value = value * 16 + hexValue(*cursor++);
        }
        if (*cursor == '.') {
            uint8_t v4[4];
            if (count > 6 || !parseV4(start, v4))
                return false;
            words[count++] = (v4[0] << 8) | v4[1];
            words[count++] = (v4[2] << 8) | v4[3];
            cursor += strlen(cursor);
            break;
        }
        if (!digits)
            return false;
        words[count++] = value;
        if (!*cursor)
            break;
        if (*cursor++ != ':')
            return false;
        if (*cursor == ':') {
            if (hole >= 0)
                return false;
            hole = count;
            ++cursor;
        } else if (!*cursor)
            return false;
    }
    if (hole < 0 && count != 8)
        return false;
    if (hole >= 0) {
        if (count >= 8)
            return false;
        const unsigned tail = count - hole;
        memmove(words + 8 - tail, words + hole, tail * sizeof(uint16_t));
        memset(words + hole, 0, (8 - count) * sizeof(uint16_t));
    }
    IPAddress parsed(IPv6);
    for (unsigned i = 0; i < 8; ++i) {
        parsed.bytes_[2 * i]     = words[i] >> 8;
        parsed.bytes_[2 * i + 1] = words[i];
    }
    *this = parsed;
    return true;
}
size_t IPAddress::printTo(Print& destination) const
{
    char text[48];
    if (family_ == IPv4) {
        const int length = snprintf(text, sizeof(text), "%u.%u.%u.%u", bytes_[12], bytes_[13], bytes_[14], bytes_[15]);
        return destination.write(text, length);
    }
    unsigned best = 8, bestLength = 0;
    for (unsigned i = 0; i < 8;) {
        if (bytes_[2 * i] || bytes_[2 * i + 1]) {
            ++i;
            continue;
        }
        unsigned end = i;
        while (end < 8 && !bytes_[2 * end] && !bytes_[2 * end + 1])
            ++end;
        if (end - i > bestLength && end - i >= 2) {
            best       = i;
            bestLength = end - i;
        }
        i = end;
    }
    size_t length = 0;
    for (unsigned i = 0; i < 8;) {
        if (i == best) {
            text[length++] = ':';
            text[length++] = ':';
            i += bestLength;
            continue;
        }
        if (length && text[length - 1] != ':')
            text[length++] = ':';
        length += snprintf(text + length, sizeof(text) - length, "%x", (bytes_[2 * i] << 8) | bytes_[2 * i + 1]);
        ++i;
    }
    return destination.write(text, length);
}
