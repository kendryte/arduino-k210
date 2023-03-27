#include "Arduino.h" // 引入Arduino库

#include "Image.h" // 引入Image库

#include "GC0328.h"
#include "OV2640.h" // 引入OV2640库

#include "ST7789V.h" // 引入ST7789V库

#include "FS.h" // 引入FS库
#include "FFat.h" // 引入FFat库

using namespace K210; // 使用K210命名空间

#define KEY_PIN     (16) // 定义按键引脚

// GC0328 cam;
OV2640 cam; // 定义OV2640摄像头对象
ST7789V lcd(240, 320); // 定义ST7789V显示屏对象

Image *img_ai, *img_display; // 定义Image对象指针

void setup() // 初始化函数
{
    camera_buffers_t buff; // 定义摄像头缓存对象

    Serial.begin(115200); // 初始化串口通信
    while (!Serial) { // 等待串口连接
        ;
    }

    lcd.begin(); // 初始化显示屏
    // lcd.invertDisplay(1);
    lcd.setRotation(3); // 设置显示屏旋转方向
    lcd.setTextSize(2); // 设置显示屏字体大小
    lcd.setCursor(0,0); // 设置显示屏光标位置

    if(!FFat.begin()){ // 挂载FFat文件系统
        lcd.printf("FFat Mount Failed\n"); // 显示挂载失败信息
        Serial.printf("FFat Mount Failed"); // 输出挂载失败信息
        while(1) {} // 挂载失败，进入死循环
    }

    if(false == FFat.mkdir("/img")) // 创建/img目录
    {
        lcd.printf("FFat mkdir Failed\n"); // 显示创建失败信息
        Serial.printf("FFat mkdir Failed"); // 输出创建失败信息
        while(1) {} // 创建失败，进入死循环
    }

    if(0x00 != cam.reset(FRAMESIZE_QVGA)) // 摄像头复位
    {
        lcd.printf("camera reset failed\n"); // 显示复位失败信息
        Serial.printf("camera reset failed\n"); // 输出复位失败信息
        while(1) {} // 复位失败，进入死循环
    }

    cam.get_buffers(&buff); // 获取摄像头缓存
    if((NULL == buff.disply) || (NULL == buff.ai.r)) // 判断缓存是否获取成功
    {
        lcd.printf("get camera buffers failed\n"); // 显示获取失败信息
        Serial.printf("get camera buffers failed\n"); // 输出获取失败信息
        while(1) {} // 获取失败，进入死循环
    }

    img_ai = new Image(cam.width(), cam.height(), IMAGE_FORMAT_R8G8B8, buff.ai.r); // 创建Image对象
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply); // 创建Image对象

    lcd.setFrameBuffer(img_display); // 设置显示屏帧缓存

    pinMode(KEY_PIN, INPUT_PULLUP); // 设置按键引脚为输入模式
}

void loop() // 主循环函数
{
    if(0x00 != cam.snapshot()) // 拍摄照片
    {
        lcd.setCursor(0,0); // 设置光标位置
        lcd.printf("camera snapshot failed\n"); // 显示拍摄失败信息
        lcd.refresh(); // 刷新显示屏

        Serial.printf("camera snapshot failed\n"); // 输出拍摄失败信息
        return; // 返回
    }

    if(0x00 == digitalRead(KEY_PIN)) // 判断按键是否按下
    {
        char name[32]; // 定义文件名
        int result = -1; // 定义保存结果
        Image *rgb888 = NULL; // 定义Image对象指针

        snprintf(name, sizeof(name), "/img/img_%ld.bmp", millis()); // 格式化文件名

        if((rgb888 = img_display->to_rgb888())) // 将Image对象转换为RGB888格式
        {
            result = rgb888->save_bmp(FFat, name); // 保存照片
            delete rgb888; // 释放Image对象
        }

        if(0x00 == result) // 判断保存结果
        {
            lcd.setCursor(0,0); // 设置光标位置
            lcd.printf("Save succ, %s", name); // 显示保存成功信息
        }
        else
        {
            lcd.setCursor(0,0); // 设置光标位置
            lcd.printf("Save fail, %s", name); // 显示保存失败信息
        }
        lcd.refresh(); // 刷新显示屏

        delay(300); // 延时300ms
    }
    else
    {
        lcd.refresh(); // 刷新显示屏
    }
}
