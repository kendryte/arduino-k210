// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Print.h"

enum LookaheadMode { SKIP_ALL, SKIP_NONE, SKIP_WHITESPACE };
class Stream : public Print {
public:
    virtual int read()      = 0;
    virtual int peek()      = 0;
    virtual int available() = 0;
    void        setTimeout(unsigned long milliseconds)
    {
        _timeout = milliseconds;
    }
    unsigned long getTimeout() const
    {
        return _timeout;
    }
    bool find(char value)
    {
        return find(&value, 1);
    }
    bool find(const char* pattern);
    bool find(const char* pattern, size_t count);
    bool find(const uint8_t* pattern)
    {
        return find(reinterpret_cast<const char*>(pattern));
    }
    bool find(const uint8_t* pattern, size_t count)
    {
        return find(reinterpret_cast<const char*>(pattern), count);
    }
    bool findUntil(const char* pattern, const char* stop);
    bool findUntil(const char* pattern, size_t count, const char* stop, size_t stopCount);
    bool findUntil(const uint8_t* pattern, const char* stop)
    {
        return findUntil(reinterpret_cast<const char*>(pattern), stop);
    }
    bool findUntil(const uint8_t* pattern, size_t count, const char* stop, size_t stopCount)
    {
        return findUntil(reinterpret_cast<const char*>(pattern), count, stop, stopCount);
    }
    long   parseInt(LookaheadMode mode = SKIP_ALL, char ignore = '\x01');
    float  parseFloat(LookaheadMode mode = SKIP_ALL, char ignore = '\x01');
    size_t readBytes(char* output, size_t count);
    size_t readBytes(uint8_t* output, size_t count)
    {
        return readBytes(reinterpret_cast<char*>(output), count);
    }
    size_t readBytesUntil(char stop, char* output, size_t count);
    size_t readBytesUntil(char stop, uint8_t* output, size_t count)
    {
        return readBytesUntil(stop, reinterpret_cast<char*>(output), count);
    }
    String readString();
    String readStringUntil(char stop);

protected:
    unsigned long _timeout     = 1000;
    unsigned long _startMillis = 0;
    int           timedRead();
    int           timedPeek();
    int           peekNextDigit(LookaheadMode mode, bool decimal);
    long          parseInt(char ignore)
    {
        return parseInt(SKIP_ALL, ignore);
    }
    float parseFloat(char ignore)
    {
        return parseFloat(SKIP_ALL, ignore);
    }
    struct MultiTarget {
        const char* str;
        size_t      len;
        size_t      index;
    };
    int findMulti(MultiTarget* patterns, int count);

private:
    int waitByte(bool consume);
};
