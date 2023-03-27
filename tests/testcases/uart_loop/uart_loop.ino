#include "Arduino.h"

#include "HardwareSerial.h"

#include "unity/unity.h"

#define UART_TX             (34)
#define UART_RX             (35)
#define TEST_TX_RX_SIZE     (128)
/*****************************************************************************/
HardwareSerial testSer(1);

/* These functions are intended to be called before and after each test. */
void setUp(void)
{
}

void tearDown(void)
{
}

void testUartLoop(void)
{
    uint8_t recv[TEST_TX_RX_SIZE], send[TEST_TX_RX_SIZE] = "Hello";

    testSer.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
    if(!testSer)
    {
        Serial.println("open serial1 failed");
        TEST_ASSERT(0);
    }

    for(int i = 5; i < TEST_TX_RX_SIZE; i++)
    {
        send[i] = (uint8_t)random(0, 255);
    }

    testSer.write(send, TEST_TX_RX_SIZE);
    rt_thread_delay(2);

    memset(recv, 0, TEST_TX_RX_SIZE);
    size_t rd = testSer.read(recv, TEST_TX_RX_SIZE);

    TEST_ASSERT_EQUAL(TEST_TX_RX_SIZE, rd);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(send, recv, TEST_TX_RX_SIZE);

    testSer.end();
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(testUartLoop);
    UNITY_END();
}

void loop()
{
}
