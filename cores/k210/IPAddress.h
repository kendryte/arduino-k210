// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Printable.h"
#include "WString.h"
#include <stdint.h>
#define IPADDRESS_V4_BYTES_INDEX 12
#define IPADDRESS_V4_DWORD_INDEX 3
enum IPType { IPv4, IPv6 };
class IPAddress : public Printable {
public:
    IPAddress();
    explicit IPAddress(IPType family);
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j,
              uint8_t k, uint8_t l, uint8_t m, uint8_t n, uint8_t o, uint8_t p);
    IPAddress(uint32_t address);
    IPAddress(const uint8_t* address);
    IPAddress(IPType family, const uint8_t* address);
    IPAddress& operator=(uint32_t address);
    IPAddress& operator=(const uint8_t* address);
               operator uint32_t() const;
    bool       operator==(const IPAddress& other) const;
    bool       operator!=(const IPAddress& other) const
    {
        return !(*this == other);
    }
    bool     operator==(const uint8_t* address) const;
    uint8_t& operator[](int index);
    uint8_t  operator[](int index) const;
    bool     fromString(const char* text);
    bool     fromString(const String& text)
    {
        return fromString(text.c_str());
    }
    IPType type() const
    {
        return family_;
    }
    size_t printTo(Print& destination) const override;

protected:
    bool fromString4(const char* text);
    bool fromString6(const char* text);

private:
    uint8_t  bytes_[16] = {};
    IPType   family_    = IPv4;
    uint8_t* raw_address()
    {
        return bytes_ + (family_ == IPv4 ? 12 : 0);
    }
    const uint8_t* raw_address() const
    {
        return bytes_ + (family_ == IPv4 ? 12 : 0);
    }
    friend class Client;
    friend class UDP;
    friend class Server;
    friend class EthernetClass;
    friend class DhcpClass;
    friend class DNSClient;
};
extern const IPAddress IN6ADDR_ANY;
extern const IPAddress INADDR_NONE;
