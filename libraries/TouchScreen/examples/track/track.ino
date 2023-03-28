#include "Arduino.h"

#include "NS2009.h"
#include "ST7789V.h"

#include "Wire.h"

#include "k210-hal.h"

#define CLR_PIN     (16)

TwoWire i2c1(1);
NS2009 ts(&i2c1);

ST7789V lcd(240, 320);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    }

    // need configure the pins
    if (false == i2c1.begin(30, 31))
    {
        Serial.println("ts i2c begin failed!");
        while (1)
        {
        }
    }

    if (false == ts.begin())
    {
        Serial.println("ts begin failed!");
        while (1)
        {
        }
    }

    lcd.begin();
    lcd.setRotation(3);

    // ts calibration
    // int cal[7];
    // ts.do_tscal(lcd, cal);
    // for(int i = 0; i < 7; i++)
    // {
    //     Serial.printf("%d->%d\n", i, cal[i]);
    // }

    lcd.fillScreen(0xFFFF);

    pinMode(CLR_PIN, INPUT_PULLUP);

    Serial.println("Start!");
}

static inline void lcd_draw(int x, int y)
{
    lcd.writePixel(x, y, 0xF800);
    lcd.endWrite();
}

void loop()
{
    auto p = ts.poll();

    if((p.type != TOUCH_END) && (p.type != TOUCH_NONE))
    {
        lcd_draw(p.x, p.y);
    }

    if(0x00 == digitalRead(CLR_PIN))
    {
        lcd.fillScreen(0xFFFF);
    }
}
