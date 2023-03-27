#include "Arduino.h"

#include "SHA256.h"

#include "unity/unity.h"

using namespace K210;

/* These functions are intended to be called before and after each test. */
void setUp(void)
{
}

void tearDown(void)
{
}

void test_pass(void)
{
    uint8_t sha256[32];
    uint8_t data[] = "012345678901234567890123456789";
    const uint8_t result[] = {0x27, 0x6F, 0xAD, 0xFC, 0x9E, 0xDC, 0x49, 0xF5, 0xF9, 0xAF, 0x96, 0xD9, 0x76, 0x36, 0x73, 0x1D,
                              0xEF, 0x75, 0x25, 0xD4, 0xBF, 0xA1, 0x6B, 0xC0, 0x76, 0x99, 0x53, 0x48, 0x73, 0xA4, 0x74, 0xCC};

    SHA256::begin(30);
    SHA256::update(data, 30);
    SHA256::digest(sha256);
    SHA256::end();

    TEST_ASSERT_EQUAL_UINT8_ARRAY(sha256, result, 32);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(test_pass);
    UNITY_END();
}

void loop()
{
}
