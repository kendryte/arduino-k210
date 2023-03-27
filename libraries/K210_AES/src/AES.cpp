#define DBG_TAG "AES"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtthread.h"

#include "AES.h"

namespace K210 {

size_t              AES::_processDataLen    = 0;
size_t              AES::_totalDataLen      = 0;

uint8_t             AES::_key[32]           = { 0 };
uint8_t             AES::_iv[16]            = { 0 };
uint8_t             AES::_gcmAAD[33]        = { 0 };
uint8_t             AES::_gcmAADLen         = 0;

aes_cipher_mode_t   AES::_mode              = AES_ECB;
aes_kmode_t         AES::_keyLen            = AES_128;
aes_iv_len_t        AES::_ivLen             = IV_LEN_128;

/* static */ int AES::begin(aes_cipher_mode_t mode, aes_kmode_t keyLen)
{
    return begin(mode, keyLen, IV_LEN_INVAILD);
}

/* static */ int AES::begin(aes_cipher_mode_t mode, aes_kmode_t keyLen, aes_iv_len_t ivLen)
{
    if(AES_CIPHER_MAX <= mode) {
        LOG_E("Invaild mode");
        return -1;
    }
    _mode = mode;

    if((AES_128 != keyLen) && (AES_192 != keyLen) && (AES_256 != keyLen)) {
        LOG_E("Invaild keyLen");
        return -1;
    }
    _keyLen = keyLen;

    if(AES_CBC == mode) {
        if(IV_LEN_128 != ivLen) {
            LOG_E("CBC mode iv must be 16 Byte!");
            return -1;
        }
    } else if(AES_GCM == mode) {
        if(IV_LEN_96 != ivLen) {
            LOG_E("GCM mode iv must be 12 Byte!");
            return -1;
        }
    }
    _ivLen = ivLen;

    rt_memset(_key, 0, sizeof(_key));
    rt_memset(_iv, 0, sizeof(_iv));

    rt_memset(_gcmAAD, 0, sizeof(_gcmAAD));
    _gcmAADLen = 0;

    _totalDataLen = 0;
    _processDataLen = 0;

    return 0;
}

/* static */ void AES::end()
{
    sysctl_clock_disable(SYSCTL_CLOCK_AES);

    _processDataLen = 0;
    _totalDataLen = 0;

    rt_memset(_key, 0, sizeof(_key));
    rt_memset(_iv, 0, sizeof(_iv));
    rt_memset(_gcmAAD, 0, sizeof(_gcmAAD));
    _gcmAADLen = 0;
}

/* static */ int AES::setDataLen(size_t dataLen)
{
    if(0x00 != (_totalDataLen - _processDataLen)) {
        LOG_E("There is remain data!");
        return -1;
    }

    size_t padding_len = ((dataLen + 15) / 16) * 16;

    _processDataLen = 0;
    _totalDataLen = padding_len;

    return 0;
}

/* static */ int AES::setKey(const uint8_t *key, uint8_t len)
{
    uint8_t _len = (len > sizeof(_key)) ? sizeof(_key) : len;

    rt_memcpy(_key, key, _len);

    if(len != _keyLen) {
        LOG_E("Key length not same");
        return -1;
    }

    return 0;
}

/* static */ int AES::setIV(const uint8_t *iv, uint8_t len)
{
    uint8_t _len = (len > sizeof(_iv)) ? sizeof(_iv) : len;

    rt_memcpy(_iv, iv, _len);

    if(len != _ivLen) {
        LOG_E("IV length not same");
        return -1;
    }

    return 0;
}

/* static */ int AES::process(uint8_t *input, uint8_t *output, size_t len, aes_encrypt_sel_t enc_dec)
{
    size_t _len = ((_processDataLen + len) > _totalDataLen) ? (_totalDataLen - _processDataLen) : len;

    if(0 >= _len) {
        LOG_E("Error data length %d, _processDataLen %d, _totalDataLen %d\r\n", len, _processDataLen, _totalDataLen);
        return -1;
    }

    if(0x00 == _processDataLen) {
        aes_init(_key, _keyLen, _iv, _ivLen, _gcmAAD, _mode, enc_dec, _gcmAADLen, _totalDataLen);
    }

    aes_process(input, output, _len, _mode);

    _processDataLen += _len;

    return 0;
}

/* static */ int AES::encrypt(uint8_t *input, uint8_t *output, size_t len)
{
    return process(input, output, len, AES_HARD_ENCRYPTION);
}

/* static */ int AES::decrypt(uint8_t *input, uint8_t *output, size_t len)
{
    return process(input, output, len, AES_HARD_DECRYPTION);
}

/* static */ int AES::gcmSetAAD(uint8_t *aad, uint8_t len)
{
    uint8_t _len = (len > sizeof(_gcmAAD)) ? sizeof(_gcmAAD) : len;

    if(AES_GCM == _mode) {
        rt_memcpy(_gcmAAD, aad, _len);
        return 0;
    }
    return -1;
}

/* static */ int AES::gcmGetTag(uint8_t *gcmTag)
{
    if(AES_GCM == _mode) {
        gcm_get_tag(gcmTag);
        return 0;
    }

    return -1;
}

}
