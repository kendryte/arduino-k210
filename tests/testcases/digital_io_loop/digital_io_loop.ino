#include "Arduino.h"

#include "unity/unity.h"

#define PIN_OUTPUT      (34)
#define PIN_INPUT       (35)

/*****************************************************************************/
/* These functions are intended to be called before and after each test. */
void setUp(void)
{
    pinMode(PIN_INPUT, INPUT);
    pinMode(PIN_OUTPUT, OUTPUT);

    digitalWrite(PIN_OUTPUT, HIGH);
}

void tearDown(void)
{
    pinModeClear(PIN_INPUT);
    pinModeClear(PIN_OUTPUT);
}

void testOutput(void)
{
    PinStatus sts;

    digitalWrite(PIN_OUTPUT, LOW);
    sts = digitalRead(PIN_INPUT);
    TEST_ASSERT_EQUAL(LOW, sts);

    digitalWrite(PIN_OUTPUT, HIGH);
    sts = digitalRead(PIN_INPUT);
    TEST_ASSERT_EQUAL(HIGH, sts);
}

static volatile int flag = -1;
static volatile int edgeCount = 0;

void pin_irq(void)
{
    flag = 1;
}

void testInterrupt(void)
{
    flag = -1;

    attachInterrupt(PIN_INPUT, pin_irq, FALLING);

    digitalWrite(PIN_OUTPUT, LOW);
    rt_thread_delay(1);

    TEST_ASSERT_EQUAL(flag, 1);
    detachInterrupt(PIN_INPUT);
}

void pin_irq_param(void *context)
{
    volatile int *count = static_cast<volatile int *>(context);
    ++*count;
}

void testRisingInterrupt(void)
{
    flag = -1;
    digitalWrite(PIN_OUTPUT, LOW);
    rt_thread_delay(1);
    attachInterrupt(PIN_INPUT, pin_irq, RISING);
    digitalWrite(PIN_OUTPUT, HIGH);
    rt_thread_delay(1);
    TEST_ASSERT_EQUAL(1, flag);
    detachInterrupt(PIN_INPUT);
}

void testChangeInterruptWithParameter(void)
{
    edgeCount = 0;
    attachInterruptParam(PIN_INPUT, pin_irq_param, CHANGE, const_cast<int *>(&edgeCount));
    digitalWrite(PIN_OUTPUT, LOW);
    rt_thread_delay(1);
    digitalWrite(PIN_OUTPUT, HIGH);
    rt_thread_delay(1);
    TEST_ASSERT_EQUAL(2, edgeCount);
    detachInterrupt(PIN_INPUT);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(testOutput);
    RUN_TEST(testInterrupt);
    RUN_TEST(testRisingInterrupt);
    RUN_TEST(testChangeInterruptWithParameter);
    UNITY_END();
}

void loop()
{

}
