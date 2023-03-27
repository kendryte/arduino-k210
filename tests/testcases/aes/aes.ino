#include "Arduino.h"
#include "AES.h"

#include "unity/unity.h"

using namespace K210;



/* These functions are intended to be called before and after each test. */
void setUp(void)
{
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xAB};
    AES::begin(AES_ECB, AES_128);
    AES::setKey(key, 16);
    AES::setDataLen(16);
}

void tearDown(void)
{
    AES::end();
}

void testEncrypt(void)
{
    uint8_t output[16], data[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5'};
    const uint8_t result[] = {156, 19, 157, 96, 176, 61, 27, 63, 194, 70, 122, 247, 208, 106, 124, 176};

    AES::encrypt(data, output, 16);

    Serial.print("Encrypt:");
    for(int i = 0; i < 16; i++)
    {
        Serial.printf("%02X", output[i]);
    }
    Serial.println();

    TEST_ASSERT_EQUAL_UINT8_ARRAY(result, output, 16);
}

void testDecrypt(void)
{
    uint8_t output[16], data[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5'};
    const uint8_t result[] = {133, 56, 89, 151, 115, 201, 7, 26, 198, 56, 25, 50, 18, 251, 63, 134};

    AES::decrypt(data, output, 16);

    Serial.print("Decrypt:");
    for(int i = 0; i < 16; i++)
    {
        Serial.printf("%02X", output[i]);
    }
    Serial.println();

    TEST_ASSERT_EQUAL_UINT8_ARRAY(result, output, 16);
}

void setup() {
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(testEncrypt);
    RUN_TEST(testDecrypt);
    UNITY_END();
}

void loop() {

}
