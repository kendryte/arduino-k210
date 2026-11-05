// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <stddef.h>
class Print;
class Printable {
public:
    virtual size_t printTo(Print& destination) const = 0;
};
