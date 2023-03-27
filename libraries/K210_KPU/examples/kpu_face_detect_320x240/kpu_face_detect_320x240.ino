#include "Arduino.h"

#include "Image.h"

#include "GC0328.h"
#include "OV2640.h"

#include "ST7789V.h"

#include "KPU.h"

#include "FS.h"
#include "FFat.h"

using namespace K210;

// GC0328 cam;
OV2640 cam;
ST7789V lcd(240, 320);

Image *img_ai, *img_display;

KPU_Yolo2 yolo2;
float anchor[] = {0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875, 0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875, 0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937};

void setup()
{
    camera_buffers_t buff;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    lcd.begin();
    // lcd.invertDisplay(1);
    lcd.setRotation(3);
    lcd.setTextSize(2);
    lcd.setTextColor(0x07E0);
    lcd.setCursor(0,0);

    if(!FFat.begin()){
        lcd.printf("FFat Mount Failed\n");
        Serial.printf("FFat Mount Failed");
        while(1) {}
    }

    if(0x00 != cam.reset(FRAMESIZE_QVGA))
    {
        lcd.printf("camera reset failed\n");
        Serial.printf("camera reset failed\n");
        while(1) {}
    }
    cam.set_vflip(true);
    cam.set_hmirror(true);

    cam.get_buffers(&buff);
    if((NULL == buff.disply) || (NULL == buff.ai.r))
    {
        lcd.printf("get camera buffers failed\n");
        Serial.printf("get camera buffers failed\n");
        while(1) {}
    }

    img_ai = new Image(cam.width(), cam.height(), IMAGE_FORMAT_R8G8B8, buff.ai.r);
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);

    if(0x00 != yolo2.load_kmodel(FFat, "/KPU/yolo_face_detect/face_detect_320x240.kmodel"))
    {
        lcd.printf("load kmodel failed\n");
        Serial.printf("load kmodel failed\n");
        while(1) {}
    }

    if(0x00 != yolo2.begin(anchor, sizeof(anchor) / sizeof(float)))
    {
        lcd.printf("yolo2 init failed\n");
        Serial.printf("yolo2 init failed\n");
        while(1) {}
    }

    lcd.setFrameBuffer(img_display);
}

void loop()
{
    obj_info_t info;

    if(0x00 != cam.snapshot())
    {
        lcd.setCursor(0,0);
        lcd.printf("camera snapshot failed\n");
        lcd.refresh();

        Serial.printf("camera snapshot failed\n");
        return;
    }

    if(0x00 != yolo2.run(img_ai, &info, DMAC_CHANNEL5))
    {
        lcd.setCursor(0,0);
        lcd.printf("yolo2 run failed\n");
        lcd.refresh();

        Serial.printf("yolo2 run failed\n");
        return;
    }

    for(int i = 0; i < info.obj_number; i++)
    {
        // Serial.printf("x %d y %d w %d h %d prob %d\n", info.obj[i].x, info.obj[i].y, info.obj[i].w, info.obj[i].h, int(info.obj[i].prob * 100));

        lcd.setCursor(info.obj[i].x, info.obj[i].y);
        lcd.printf("%.3f", info.obj[i].prob);

        lcd.drawRect(info.obj[i].x, info.obj[i].y, info.obj[i].w, info.obj[i].h, 0xF800);
    }

    lcd.refresh();
}
