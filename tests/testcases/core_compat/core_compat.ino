// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <I2S.h>
#include <ScopedTimer.h>
#include <Ticker.h>

class TextSink : public Print {
public:
    String text;
    size_t write(uint8_t ch) override { return text.concat(static_cast<char>(ch)) ? 1 : 0; }
};
void setup() {
    ScopedTimer setupTimer("core_compat", false);
    Serial.begin(115200);
    TextSink sink;
    sink.println("compat");
    sink.print(-123);
    sink.printf(" %s", "ok");
    String value("self");
    value += value;
    value.replace("self", "test");
    IPAddress address;
    bool passed = address.fromString("2001:db8::1") && value == "testtest" && sink.text == "compat\r\n-123 ok";
    passed = passed && (42 + String(" units")) == "42 units" && ('x' + String("yz")) == "xyz";
    File invalid;
    passed = passed && !invalid && invalid.read() == -1;
    Ticker ticker;
    ticker.detach();
    I2SClass audio(0, 0, 18, 35, 33);
    audio.setBufferSize(128);
    passed = passed && audio.getBufferSize() == 128;
    SPISettings settings(1000000, SPI_MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(settings); // Inactive bus: verifies the safe no-op path.
    SPI.endTransaction();
    Serial.println(passed ? "CORE_COMPAT_PASS" : "CORE_COMPAT_FAIL");
}
void loop() { delay(1000); }
