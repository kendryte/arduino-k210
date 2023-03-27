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

const char* obj_name[20] = {"aeroplane","bicycle", "bird","boat","bottle","bus","car","cat","chair","cow","diningtable", "dog","horse", "motorbike","person","pottedplant", "sheep","sofa", "train", "tvmonitor"};
static float anchor[5 * 2] = {1.3221, 1.73145, 3.19275, 4.00944, 5.05587, 8.09892, 9.47112, 4.84053, 11.2364, 10.0071};

#define CUSTOM_IMG_W    (320)
#define CUSTOM_IMG_H    (256)

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
    lcd.fillScreen(0xFFFF);

    if(!FFat.begin()){
        lcd.printf("FFat Mount Failed\n");
        Serial.printf("FFat Mount Failed");
        while(1) {}
    }

    camera_buffers_t _set_cam_buffrt;
    _set_cam_buffrt.disply = (uint8_t *)rt_malloc_align(320 * 240 * 2 + CUSTOM_IMG_W * CUSTOM_IMG_H * 3, 8);
    if(NULL == _set_cam_buffrt.disply)
    {
        lcd.printf("malloc camera buffers failed\n");
        Serial.printf("malloc camera buffers failed\n");
        while(1) {}
    }

    _set_cam_buffrt.ai.r = _set_cam_buffrt.disply + 320 * 240 * 2;
    _set_cam_buffrt.ai.g = _set_cam_buffrt.ai.r + CUSTOM_IMG_W * CUSTOM_IMG_H * 1;
    _set_cam_buffrt.ai.b = _set_cam_buffrt.ai.g + CUSTOM_IMG_W * CUSTOM_IMG_H * 1;

    if(0x00 != cam.reset(FRAMESIZE_QVGA, &_set_cam_buffrt))
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

    img_ai = new Image(CUSTOM_IMG_W, CUSTOM_IMG_H, IMAGE_FORMAT_R8G8B8, buff.ai.r);
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);

    if(0x00 != yolo2.load_kmodel(FFat, "/KPU/voc20_object_detect/voc20_detect.kmodel"))
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
        lcd.printf("%s %.2f", obj_name[info.obj[i].class_id], info.obj[i].prob);

        lcd.drawRect(info.obj[i].x, info.obj[i].y, info.obj[i].w, info.obj[i].h, 0xF800);
    }

    lcd.refresh();
}
