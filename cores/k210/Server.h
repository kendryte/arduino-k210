// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Print.h"
class Server : public Print {
public:
    virtual void begin() = 0;
};
