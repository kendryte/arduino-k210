#include <limits.h>

#include "Arduino.h"

typedef struct {
    voidFuncPtrParam fn;
    void* arg;
} InterruptHandle_t;

static InterruptHandle_t __pinInterruptHandlers[ K210_IO_NUMS ] = {0,};

void pinMode(pin_size_t pinNumber, PinMode pinMode)
{
    if(K210_IO_NUMS <= pinNumber) {
        return;
    }

    gpio_drive_mode_t mode = GPIO_DM_OUTPUT;
    fpioa_function_t func = hal_gpiohs_set_pin_func(pinNumber);

    switch (pinMode) {
        case INPUT: {
            mode = GPIO_DM_INPUT;
            break;
        } case INPUT_PULLUP: {
            mode = GPIO_DM_INPUT_PULL_UP;
            break;
        } case INPUT_PULLDOWN: {
            mode = GPIO_DM_INPUT_PULL_DOWN;
            break;
        } default:
          case OUTPUT:
          case OUTPUT_OPENDRAIN: {
            mode = GPIO_DM_OUTPUT;
            break;
        }
    }

    if((FUNC_GPIOHS0 <= func) && (func <= FUNC_GPIOHS31)) {
        gpiohs_set_drive_mode(func - FUNC_GPIOHS0, mode);
    }
}

void pinModeClear(pin_size_t pinNumber)
{
    hal_gpiohs_clr_pin_func(pinNumber);
}

void digitalWrite(pin_size_t pinNumber, PinStatus status)
{
    if(K210_IO_NUMS <= pinNumber) {
        return;
    }

    fpioa_function_t func = hal_gpiohs_get_pin_func(pinNumber);

    if((FUNC_GPIOHS0 <= func) && (func <= FUNC_GPIOHS31)) {
        gpiohs_set_pin(func - FUNC_GPIOHS0, (LOW == status) ? GPIO_PV_LOW : GPIO_PV_HIGH);
    }
}

PinStatus digitalRead(pin_size_t pinNumber)
{
    if(K210_IO_NUMS <= pinNumber) {
        return LOW;
    }

    fpioa_function_t func = hal_gpiohs_get_pin_func(pinNumber);

    gpio_pin_value_t val = GPIO_PV_LOW;

    if((FUNC_GPIOHS0 <= func) && (func <= FUNC_GPIOHS31)) {
         val= gpiohs_get_pin(func - FUNC_GPIOHS0);
    }

    return (GPIO_PV_LOW == val) ? LOW : HIGH; 
}

int analogRead(pin_size_t pinNumber)
{
    (void)pinNumber;

    return -1;
}

void analogReference(uint8_t mode)
{
    (void)mode;

    return;
}

void analogWrite(pin_size_t pinNumber, int value)
{
    (void)pinNumber;
    (void)value;

    return;
}

static int onPinInterrupt(void * arg) {
	InterruptHandle_t * isr = (InterruptHandle_t*)arg;

    if(isr->fn) {
        isr->fn(isr->arg);
    }

    return 0;
}

void attachInterruptFunctionalParam(pin_size_t pinNumber, voidFuncPtrParam callback, PinStatus mode, void* param)
{
    if(K210_IO_NUMS <= pinNumber) {
        return;
    }

    gpio_pin_edge_t edge = GPIO_PE_NONE;

    switch (mode) {
        case LOW: {
            edge = GPIO_PE_LOW;
            break;
        } case HIGH: {
            edge = GPIO_PE_HIGH;
            break;
        } case CHANGE: {
            edge = GPIO_PE_BOTH;
            break;
        } case FALLING: {
            edge = GPIO_PE_FALLING;
            break;
        } case RISING: {
            edge = GPIO_PE_RISING;
            break;
        } default: {
            edge = GPIO_PE_NONE;
            break;
        }
    }

    fpioa_function_t func = hal_gpiohs_get_pin_func(pinNumber);

    if((FUNC_GPIOHS0 <= func) && (func <= FUNC_GPIOHS31)) {
        __pinInterruptHandlers[pinNumber].fn = callback;
        __pinInterruptHandlers[pinNumber].arg = param;

        gpiohs_set_pin_edge(func - FUNC_GPIOHS0, edge);
        gpiohs_irq_register(func - FUNC_GPIOHS0, 1, onPinInterrupt, &__pinInterruptHandlers[pinNumber]);
    }
}

void attachInterrupt(pin_size_t pinNumber, voidFuncPtr callback, PinStatus mode)
{
    attachInterruptFunctionalParam(pinNumber, (voidFuncPtrParam)callback, mode, NULL);
}

void attachInterruptParam(pin_size_t pinNumber, voidFuncPtrParam callback, PinStatus mode, void* param)
{
    attachInterruptFunctionalParam(pinNumber, callback, mode, param);
}

void detachInterrupt(pin_size_t pinNumber)
{
    if(K210_IO_NUMS <= pinNumber) {
        return;
    }

    fpioa_function_t func = hal_gpiohs_get_pin_func(pinNumber);

    if((FUNC_GPIOHS0 <= func) && (func <= FUNC_GPIOHS31)) {
        gpiohs_set_pin_edge(func - FUNC_GPIOHS0, GPIO_PE_NONE);
        gpiohs_irq_unregister(func - FUNC_GPIOHS0);
    }
}

#define WAIT_FOR_PIN_STATE(state) \
    while (digitalRead(pin) != (state)) { \
        if (read_cycle() - start_cycle_count > timeout_cycles) { \
            return 0; \
        } \
    }

unsigned long pulseIn(pin_size_t pin, uint8_t state, unsigned long timeout)
{
    const uint64_t max_timeout_us = clockCyclesToMicroseconds(UINT_MAX);
    if (timeout > max_timeout_us) {
        timeout = max_timeout_us;
    }
    const uint64_t timeout_cycles = microsecondsToClockCycles(timeout);
    const uint64_t start_cycle_count = read_cycle();
    WAIT_FOR_PIN_STATE(!state);
    WAIT_FOR_PIN_STATE(state);
    const uint64_t pulse_start_cycle_count = read_cycle();
    WAIT_FOR_PIN_STATE(!state);
    return clockCyclesToMicroseconds(read_cycle() - pulse_start_cycle_count);
}

unsigned long pulseInLong(pin_size_t pin, uint8_t state, unsigned long timeout)
{
    return pulseIn(pin, state, timeout);
}

uint8_t shiftIn(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
        //digitalWrite(clockPin, HIGH);
        if(bitOrder == LSBFIRST)
            value |= digitalRead(dataPin) << i;
        else
            value |= digitalRead(dataPin) << (7 - i);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
    return value;
}

void shiftOut(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder, uint8_t val) {
    uint8_t i;

    for(i = 0; i < 8; i++) {
        if(bitOrder == LSBFIRST)
            digitalWrite(dataPin, !!(val & (1 << i)));
        else
            digitalWrite(dataPin, !!(val & (1 << (7 - i))));

        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}
