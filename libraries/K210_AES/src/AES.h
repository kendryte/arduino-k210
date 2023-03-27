#pragma once

#include "Arduino.h"

#include "k210-hal.h"

namespace K210 {

class AES {
public:
    static int begin(aes_cipher_mode_t mode, aes_kmode_t keyLen);
    static int begin(aes_cipher_mode_t mode, aes_kmode_t keyLen, aes_iv_len_t ivLen);

    static void end();

    static int setDataLen(size_t dataLen);
    static int setKey(const uint8_t *key, uint8_t len);
    static int setIV(const uint8_t *iv, uint8_t len);

    static int encrypt(uint8_t *input, uint8_t *output, size_t len);
    static int decrypt(uint8_t *input, uint8_t *output, size_t len);

    static int gcmSetAAD(uint8_t *aad, uint8_t len);
    static int gcmGetTag(uint8_t *gcmTag);

private:
    static size_t _processDataLen;
    static size_t _totalDataLen;

    static uint8_t _key[32];
    static uint8_t _iv[16];

    static uint8_t _gcmAAD[33];
    static uint8_t _gcmAADLen;

    static aes_cipher_mode_t _mode;
    static aes_kmode_t _keyLen;
    static aes_iv_len_t _ivLen;

    static int process(uint8_t *input, uint8_t *output, size_t len, aes_encrypt_sel_t enc_dec);
};

} // namespace K210
