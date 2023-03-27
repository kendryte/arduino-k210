#include "Arduino.h"

#include "Image.h"
#include "ST7789V.h"

#include "FS.h"
#include "FFat.h"

using namespace K210;

ST7789V lcd(240, 320);

// 初始化函数
void setup()
{
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    // 挂载文件系统
    if(!FFat.begin()){
        Serial.println("FFat Mount Failed");
        return;
    }

    // 初始化LCD
    lcd.begin();
    // lcd.invertDisplay(1);
    lcd.setRotation(3);
    lcd.setTextSize(3);

    // 加载nmp图片
    Image *img = Image::load_bmp(FFat, "/2.bmp");

    if(img)
    {
        Serial.printf("width %d, height %d, bpp %d, pixel %p\n", img->w, img->h, img->bpp, img->pixel);

        // 转换图片格式为rgb565
        Image *rgb565 = img->to_rgb565();
        if(rgb565)
        {
            // 显示
            lcd.drawImage(rgb565);
            delete rgb565;
        }

        delete img;
    }
    lcd.refresh();
}

// 主循环
void loop()
{
    // lcd.setCursor(0, 0);
    // lcd.printf("Test1234");

    // lcd.refresh();
}
