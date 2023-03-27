#include "Arduino.h"

#include "AES.h"

using namespace K210;

/**
enc:743ED9E1867A2B359AD5DA3D7811B234
dec:31323334353637383930000000000000
*/

void setup() {
    //定义一个16字节的密钥
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f};
    //定义一个16字节的明文
    uint8_t data[16] = "1234567890";
    //定义一个16字节的密文
    uint8_t output[16];

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    //开始加密
    AES::begin(AES_ECB, AES_128);
    //设置密钥
    AES::setKey(key, 16);

    //设置明文长度
    AES::setDataLen(16);
    //加密
    AES::encrypt(data, output, 16);

    //输出密文
    Serial.printf("enc:");
    for(int i = 0; i < 16; i++) {
        Serial.printf("%02X", output[i]);
    }
    Serial.printf("\r\n");

    //清空明文
    rt_memset(data, 0, 16);
    //设置密文长度
    AES::setDataLen(16);
    //解密
    AES::decrypt(output, data, 16);

    //输出明文
    Serial.printf("dec:");
    for(int i = 0; i < 16; i++) {
        Serial.printf("%02X", data[i]);
    }
    Serial.printf("\r\n");

    //结束加密
    AES::end();
}

void loop() {

}
