// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

namespace k210_compat {
// Return the number of characters; the caller supplies at least 66 bytes.
template <class Integer> size_t integer_text(char* output, Integer value, unsigned radix)
{
    using Unsigned = typename std::make_unsigned<Integer>::type;
    if (radix < 2 || radix > 36)
        radix = 10;
    const bool negative  = std::is_signed<Integer>::value && value < 0 && radix == 10;
    Unsigned   magnitude = static_cast<Unsigned>(value);
    if (negative)
        magnitude = Unsigned(0) - magnitude;
    char   reversed[sizeof(Integer) * 8];
    size_t count = 0;
    do {
        const unsigned digit = magnitude % radix;
        reversed[count++]    = static_cast<char>(digit < 10 ? '0' + digit : 'A' + digit - 10);
        magnitude /= radix;
    } while (magnitude);
    size_t length = 0;
    if (negative)
        output[length++] = '-';
    while (count)
        output[length++] = reversed[--count];
    output[length] = 0;
    return length;
}
}
