// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <ctype.h>
// ctype accepts only EOF or values representable as unsigned char.
inline bool isAlphaNumeric(int value)
{
    return value >= 0 && value <= 255 && isalnum(value) != 0;
}
inline bool isAlpha(int value)
{
    return value >= 0 && value <= 255 && isalpha(value) != 0;
}
inline bool isWhitespace(int value)
{
    return value >= 0 && value <= 255 && isblank(value) != 0;
}
inline bool isControl(int value)
{
    return value >= 0 && value <= 255 && iscntrl(value) != 0;
}
inline bool isDigit(int value)
{
    return value >= 0 && value <= 255 && isdigit(value) != 0;
}
inline bool isGraph(int value)
{
    return value >= 0 && value <= 255 && isgraph(value) != 0;
}
inline bool isLowerCase(int value)
{
    return value >= 0 && value <= 255 && islower(value) != 0;
}
inline bool isPrintable(int value)
{
    return value >= 0 && value <= 255 && isprint(value) != 0;
}
inline bool isPunct(int value)
{
    return value >= 0 && value <= 255 && ispunct(value) != 0;
}
inline bool isSpace(int value)
{
    return value >= 0 && value <= 255 && isspace(value) != 0;
}
inline bool isUpperCase(int value)
{
    return value >= 0 && value <= 255 && isupper(value) != 0;
}
inline bool isHexadecimalDigit(int value)
{
    return value >= 0 && value <= 255 && isxdigit(value) != 0;
}
inline bool isAscii(int value)
{
    return value >= 0 && value < 128;
}
inline int toAscii(int value)
{
    return value & 127;
}
inline int toLowerCase(int value)
{
    return value >= 0 && value <= 255 ? tolower(value) : value;
}
inline int toUpperCase(int value)
{
    return value >= 0 && value <= 255 ? toupper(value) : value;
}
