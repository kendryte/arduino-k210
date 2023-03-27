#define DBG_TAG "AES"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtthread.h"

#include "SHA256.h"

namespace K210 {

sha256_context_t SHA256::ctx;

/* static */ void SHA256::begin(size_t dataLen)
{
    sha256_init(&ctx, dataLen);
}

/* static */ void SHA256::end()
{
    rt_memset(&ctx, 0, sizeof(sha256_context_t));

    sysctl_clock_disable(SYSCTL_CLOCK_SHA);
}

/* static */ void SHA256::update(const void *input, size_t len)
{
    sha256_update(&ctx, input, len);
}

/* static */ void SHA256::digest(uint8_t *sha256)
{
    sha256_final(&ctx, sha256);
}

}
