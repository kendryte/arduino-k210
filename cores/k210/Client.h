// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "IPAddress.h"
#include "Stream.h"
class Client : public Stream {
public:
    virtual int     connect(const char* host, uint16_t port)  = 0;
    virtual int     connect(IPAddress address, uint16_t port) = 0;
    virtual void    stop()                                    = 0;
    virtual uint8_t connected()                               = 0;
    virtual         operator bool()                           = 0;
    using Stream::read;
    virtual int read(uint8_t* output, size_t count) = 0;

protected:
    uint8_t* rawIPAddress(IPAddress& address)
    {
        return address.raw_address();
    }
};
