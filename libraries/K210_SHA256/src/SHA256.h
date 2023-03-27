#pragma once

#include "Arduino.h"

#include "k210-hal.h"

namespace K210 {

class SHA256 {
public:
    static void begin(size_t dataLen);
    static void end();

    static void update(const void *input, size_t len);
    static void digest(uint8_t *sha256);

private:
    static sha256_context_t ctx;
};

} // namespace K210
