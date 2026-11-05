// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "stdlib_noniso.h"
#include <limits.h>
#include <stdio.h>
char* dtostrf(double value, signed int width, unsigned int precision, char* output)
{
    if (!output)
        return NULL;
    if (precision > INT_MAX) {
        output[0] = 0;
        return output;
    }
    // This legacy API has no size argument; the caller must provide enough storage.
    sprintf(output, "%*.*f", width, (int)precision, value);
    return output;
}
