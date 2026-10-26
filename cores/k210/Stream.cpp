// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "Stream.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>
extern "C" unsigned long millis(void);
extern "C" void          yield(void);

int Stream::waitByte(bool consume)
{
    _startMillis = millis();
    for (;;) {
        const int value = consume ? read() : peek();
        if (value >= 0)
            return value;
        if (static_cast<unsigned long>(millis() - _startMillis) >= _timeout)
            return -1;
        yield();
    }
}
int Stream::timedRead()
{
    return waitByte(true);
}
int Stream::timedPeek()
{
    return waitByte(false);
}
int Stream::peekNextDigit(LookaheadMode mode, bool decimal)
{
    for (;;) {
        const int value = timedPeek();
        if (value < 0 || (value >= '0' && value <= '9') || value == '-' || (decimal && value == '.'))
            return value;
        if (mode == SKIP_NONE || (mode == SKIP_WHITESPACE && !isspace(static_cast<unsigned char>(value))))
            return -1;
        read();
    }
}
long Stream::parseInt(LookaheadMode mode, char ignore)
{
    if (peekNextDigit(mode, false) < 0)
        return 0;
    const bool negative = peek() == '-';
    if (negative)
        read();
    const unsigned long limit  = negative ? static_cast<unsigned long>(LONG_MAX) + 1UL : LONG_MAX;
    unsigned long       result = 0;
    for (;;) {
        const int value = timedPeek();
        if (value >= '0' && value <= '9') {
            const unsigned digit = value - '0';
            result               = result > (limit - digit) / 10 ? limit : result * 10 + digit;
        } else if (value < 0 || value != static_cast<unsigned char>(ignore))
            break;
        read();
    }
    if (negative && result == static_cast<unsigned long>(LONG_MAX) + 1UL)
        return LONG_MIN;
    return negative ? -static_cast<long>(result) : static_cast<long>(result);
}
float Stream::parseFloat(LookaheadMode mode, char ignore)
{
    if (peekNextDigit(mode, true) < 0)
        return 0;
    const bool negative = peek() == '-';
    if (negative)
        read();
    double result = 0, place = 1;
    bool   fraction = false;
    for (;;) {
        const int value = timedPeek();
        if (value >= '0' && value <= '9') {
            if (fraction) {
                place *= 0.1;
                result += (value - '0') * place;
            } else
                result = result * 10 + (value - '0');
        } else if (value == '.' && !fraction)
            fraction = true;
        else if (value < 0 || value != static_cast<unsigned char>(ignore))
            break;
        read();
    }
    return static_cast<float>(negative ? -result : result);
}
size_t Stream::readBytes(char* output, size_t count)
{
    if (!output)
        return 0;
    size_t used = 0;
    while (used < count) {
        const int value = timedRead();
        if (value < 0)
            break;
        output[used++] = value;
    }
    return used;
}
size_t Stream::readBytesUntil(char stop, char* output, size_t count)
{
    if (!output)
        return 0;
    size_t used = 0;
    while (used < count) {
        const int value = timedRead();
        if (value < 0 || value == static_cast<unsigned char>(stop))
            break;
        output[used++] = value;
    }
    return used;
}
String Stream::readString()
{
    String output;
    for (int value = timedRead(); value >= 0; value = timedRead())
        if (!output.concat(static_cast<char>(value)))
            break;
    return output;
}
String Stream::readStringUntil(char stop)
{
    String output;
    for (int value = timedRead(); value >= 0 && value != static_cast<unsigned char>(stop); value = timedRead())
        if (!output.concat(static_cast<char>(value)))
            break;
    return output;
}
bool Stream::find(const char* pattern)
{
    return pattern && find(pattern, strlen(pattern));
}
bool Stream::find(const char* pattern, size_t count)
{
    return findUntil(pattern, count, nullptr, 0);
}
bool Stream::findUntil(const char* pattern, const char* stop)
{
    return pattern && findUntil(pattern, strlen(pattern), stop, stop ? strlen(stop) : 0);
}
bool Stream::findUntil(const char* pattern, size_t count, const char* stop, size_t stopCount)
{
    if (!pattern || (!stop && stopCount))
        return false;
    MultiTarget candidates[] = { { pattern, count, 0 }, { stop, stopCount, 0 } };
    return findMulti(candidates, stop && stopCount ? 2 : 1) == 0;
}
int Stream::findMulti(MultiTarget* patterns, int count)
{
    if (!patterns || count <= 0)
        return -1;
    for (int i = 0; i < count; ++i) {
        patterns[i].index = 0;
        if (!patterns[i].len)
            return i;
        if (!patterns[i].str)
            return -1;
    }
    for (int value = timedRead(); value >= 0; value = timedRead()) {
        for (int i = 0; i < count; ++i) {
            MultiTarget& p = patterns[i];
            // On mismatch, retain the longest suffix that is also a prefix.
            while (p.index && static_cast<unsigned char>(p.str[p.index]) != value) {
                size_t prefix = p.index - 1;
                while (prefix && memcmp(p.str, p.str + p.index - prefix, prefix))
                    --prefix;
                p.index = prefix;
            }
            if (static_cast<unsigned char>(p.str[p.index]) == value)
                ++p.index;
            if (p.index == p.len)
                return i;
        }
    }
    return -1;
}
