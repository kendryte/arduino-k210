#include "Arduino.h"

#include "Image.h"

// #include "GC0328.h"
#include "OV2640.h"

#include "ST7789V.h"

using namespace K210;

// 定义摄像头和LCD
OV2640 cam;
ST7789V lcd(240, 320);

// 定义两个图像对象，一个用于AI处理，一个用于显示
Image *img_ai, *img_display;

void setup()
{

    camera_buffers_t buff;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    // 摄像头初始化
    if(0x00 != cam.reset(FRAMESIZE_QVGA))
    {
        Serial.printf("camera reset failed\n");
        while(1) {}
    }

    // 获取摄像头缓冲区
    cam.get_buffers(&buff);
    if((NULL == buff.disply) || (NULL == buff.ai.r))
    {
        Serial.printf("get camera buffers failed\n");
        while(1) {}
    }

    // 初始化图像对象
    img_ai = new Image(cam.width(), cam.height(), IMAGE_FORMAT_R8G8B8, buff.ai.r);
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);

    // LCD初始化
    lcd.begin();

    // 设置LCD反显
    // lcd.invertDisplay(1);
    
    // 设置LCD旋转方向
    lcd.setRotation(1);
    // 设置LCD字体大小
    lcd.setTextSize(3);
    // 设置LCD帧缓冲区
    lcd.setFrameBuffer(img_display);
}

void loop()
{
    // 拍摄一张照片
    if(0x00 != cam.snapshot())
    {
        Serial.printf("camera snapshot failed\n");
        return;
    }

    // 在LCD上显示文字
    lcd.setCursor(0, 0);
    lcd.printf("Test1234");

    // 刷新LCD
    lcd.refresh();
}
