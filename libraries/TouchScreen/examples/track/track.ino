#include "Arduino.h"

#include "NS2009.h"
#include "ST7789V.h"

#include "Wire.h"

#include "k210-hal.h"

#define CLR_PIN     (16)

TwoWire i2c1(1);            // 使用I2C1
NS2009 ts(&i2c1);           // 构造NS2009

ST7789V lcd(240, 320);      // 构造LCD

void setup()
{
    Serial.begin(115200);   // 初始化串口，115200波特率
    while (!Serial)
    {
    }

    // need configure the pins
    if (false == i2c1.begin(30, 31))    // 配置触摸芯片的引脚
    {
        Serial.println("ts i2c begin failed!");
        while (1)
        {
        }
    }

    if (false == ts.begin())            //初始化NS2009
    {
        Serial.println("ts begin failed!");
        while (1)
        {
        }
    }

    lcd.begin();            //初始化屏幕
    lcd.setRotation(3);     // 旋转屏幕

    // 取消注释，则在开机之后进行触摸校准
    // int cal[7];
    // ts.do_tscal(lcd, cal);
    // for(int i = 0; i < 7; i++)
    // {
    //     Serial.printf("%d->%d\n", i, cal[i]);
    // }

    lcd.fillScreen(0xFFFF); // 填充屏幕为白色

    pinMode(CLR_PIN, INPUT_PULLUP); // 初始化引脚，按下清空轨迹

    Serial.println("Start!");
}

// 在屏幕上画点
static inline void lcd_draw(int x, int y)
{
    lcd.writePixel(x, y, 0xF800);
    lcd.endWrite();
}

void loop()
{
    auto p = ts.poll(); // 读取触摸信息

    // 判断触摸信息，并画点
    if((p.type != TOUCH_END) && (p.type != TOUCH_NONE))
    {
        lcd_draw(p.x, p.y);
    }

    // 判断按键是否按下，按下则清屏
    if(0x00 == digitalRead(CLR_PIN))
    {
        lcd.fillScreen(0xFFFF);
    }
}
