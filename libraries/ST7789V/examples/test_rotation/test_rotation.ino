#include "Arduino.h"

#include "ST7789V.h"
#include "Image.h"

using namespace K210;

// 创建一个LCD
ST7789V lcd(240, 320);

// 初始化图像
Image img(240, 320, IMAGE_FORMAT_RGB565, true);

void setup(void) {
    // LCD初始化
    lcd.begin();
    // lcd.invertDisplay(1);
    // 设置LCD帧缓冲区
    lcd.setFrameBuffer(&img);
    // 设置LCD字体大小
    lcd.setTextSize(3);
}

int cnt = 0;

void loop(void) {
    // 旋转LCD
    lcd.setRotation(cnt++);

    if(cnt > 3) {
        cnt = 0;
    }

    // 填充LCD屏幕为红色
    lcd.fillScreen(0xF800);

    // 设置LCD光标位置
    lcd.setCursor(0, 0);
    // 打印字符串
    lcd.print("0123456789012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789012345678901234567890"
    "123456789012345678901234567890123456789012345678901234567890123456789012345678901"
    "23456789012345678901234567890123456789");

    // 刷新LCD
    lcd.refresh();

    // 延迟1秒
    delay(1000);
}
