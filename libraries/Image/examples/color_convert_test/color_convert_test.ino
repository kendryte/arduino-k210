#include "Arduino.h"

#include "Image.h"
#include "OV2640.h"
#include "ST7789V.h"

#include "FFat.h"

using namespace K210;

// 定义摄像头和LCD
OV2640 cam;
ST7789V lcd(240, 320);

// 定义图像指针
Image *img_ai, *img_display, *img_128x128;

void setup()
{
    camera_buffers_t buff;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    // 初始化LCD
    lcd.begin();
    // lcd.invertDisplay(1);
    lcd.setRotation(3);
    // lcd.setTextSize(2);
    lcd.setTextColor(0x07E0);
    lcd.setCursor(0,0);
    lcd.fillScreen(0xFFFF);

    // 挂载文件系统
    if(!FFat.begin()){
        lcd.printf("FFat Mount Failed\n");
        Serial.printf("FFat Mount Failed");
        while(1) {}
    }

    // 初始化摄像头
    if(0x00 != cam.reset(FRAMESIZE_QVGA))
    {
        lcd.printf("camera reset failed\n");
        Serial.printf("camera reset failed\n");
        while(1) {}
    }
    cam.set_vflip(true);
    cam.set_hmirror(true);

    // 获取摄像头缓存
    cam.get_buffers(&buff);
    if((NULL == buff.disply) || (NULL == buff.ai.r))
    {
        lcd.printf("get camera buffers failed\n");
        Serial.printf("get camera buffers failed\n");
        while(1) {}
    }

    // 初始化图像指针
    img_ai = new Image(cam.width(), cam.height(), IMAGE_FORMAT_R8G8B8, buff.ai.r);
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);
}

void loop()
{
    // 拍摄照片
    if(0x00 != cam.snapshot())
    {
        lcd.setCursor(0,0);
        lcd.printf("camera snapshot failed\n");
        lcd.refresh();

        Serial.printf("camera snapshot failed\n");
        return;
    }

    Image *img_gray, *img_rgb565, *img_rgb888, *img_r8g8b8;

    // 生成灰度图像
    img_gray = img_display->to_grayscale();
    if(!img_gray)
    {
        lcd.setCursor(0,0);
        lcd.printf("to gray failed\n");
        lcd.refresh();

        Serial.printf("to gray failed\n");
        return;
    }

    // 将灰度图像转换为RGB565图像
    img_rgb565 = img_gray->to_rgb565();
    lcd.drawImage(img_rgb565);

    delete img_rgb565;

    // 显示转换后的图像
    lcd.setCursor(0,0);
    lcd.printf("gray to rgb565");
    lcd.refresh();
    delay(500);

    // 将灰度图像转换为RGB888图像
    img_rgb888 = img_gray->to_rgb888();
    if(!img_rgb888)
    {
        lcd.setCursor(0,0);
        lcd.printf("to rgb888 failed\n");
        lcd.refresh();

        Serial.printf("to rgb888 failed\n");
        return;
    }

    // 将RGB888图像转换为RGB565图像
    img_rgb565 = img_rgb888->to_rgb565();
    lcd.drawImage(img_rgb565);

    delete img_rgb565;
    delete img_rgb888;

    // 显示转换后的图像
    lcd.setCursor(0,0);
    lcd.printf("gray to rgb888 to rgb565");
    lcd.refresh();
    delay(500);

    // 将灰度图像转换为R8G8B8图像
    img_r8g8b8 = img_gray->to_r8g8b8();
    if(!img_r8g8b8)
    {
        lcd.setCursor(0,0);
        lcd.printf("to img_r8g8b8 failed\n");
        lcd.refresh();

        Serial.printf("to img_r8g8b8 failed\n");
        return;
    }

    // 将r8g8b8图像转换为RGB565图像
    img_rgb565 = img_r8g8b8->to_rgb565();
    lcd.drawImage(img_rgb565);

    delete img_rgb565;
    delete img_r8g8b8;

    // 显示转换后的图像
    lcd.setCursor(0,0);
    lcd.printf("gray to img_r8g8b8 to rgb565");
    lcd.refresh();
    delay(500);

    delete img_gray;

    // 将RGB565图像转换为RGB888图像
    img_rgb888 = img_display->to_rgb888();
    if(!img_rgb888)
    {
        lcd.setCursor(0,0);
        lcd.printf("to rgb888 failed\n");
        lcd.refresh();

        Serial.printf("to rgb888 failed\n");
        return;
    }

    // 将RGB888图像转换为RGB565图像
    img_rgb565 = img_rgb888->to_rgb565();
    lcd.drawImage(img_rgb565);

    delete img_rgb565;
    delete img_rgb888;

    // 显示转换后的图像
    lcd.setCursor(0,0);
    lcd.printf("rgb565 to rgb888 to rgb565");
    lcd.refresh();
    delay(500);

    // 将rgb888图像转换为R8G8B8图像
    img_r8g8b8 = img_rgb888->to_r8g8b8();
    if(!img_r8g8b8)
    {
        lcd.setCursor(0,0);
        lcd.printf("to img_r8g8b8 failed\n");
        lcd.refresh();

        Serial.printf("to img_r8g8b8 failed\n");
        return;
    }

    // 将R8G8B8图像转换为RGB565图像
    img_rgb565 = img_r8g8b8->to_rgb565();
    lcd.drawImage(img_rgb565);

    delete img_rgb565;
    delete img_r8g8b8;
}
