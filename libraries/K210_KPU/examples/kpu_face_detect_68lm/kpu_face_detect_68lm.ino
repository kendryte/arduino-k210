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

Image *img_ai, *img_display, *img_128x128;

KPU_Base landmark;
KPU_Yolo2 yolo2;

static float anchor[9*2] = {0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875, 0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875, 0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937};

static void extend_box(rectangle_t *in, rectangle_t *out, float scale)
{
    uint32_t x = in->x;
    uint32_t y = in->y;
    uint32_t w = in->w;
    uint32_t h = in->h;
    float new_x1_t = x - scale * w;
    float new_x2_t = x + w + scale * w;
    float new_y1_t = y - scale * h;
    float new_y2_t = y + h + scale * h;

    uint32_t new_x1 = new_x1_t > 1 ? floorf(new_x1_t) : 1;
    uint32_t new_x2 = new_x2_t < 320 ? floorf(new_x2_t) : 319;
    uint32_t new_y1 = new_y1_t > 1 ? floorf(new_y1_t) : 1;
    uint32_t new_y2 = new_y2_t < 240 ? floorf(new_y2_t) : 239;
    out->x = new_x1;
    out->y = new_y1;
    out->w = new_x2 - new_x1 + 1;
    out->h = new_y2 - new_y1 + 1;
}

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

    img_128x128 = new Image(128, 128, IMAGE_FORMAT_R8G8B8, true);
    if(NULL == img_128x128)
    {
        lcd.printf("new image 128x128 failed\n");
        Serial.printf("new image 128x128 failed\n");
        while(1) {}
    }

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

    if(0x00 != landmark.load_kmodel(FFat, "/KPU/face_detect_with_68landmark/landmark68.kmodel"))
    {
        lcd.printf("load kmodel failed2\n");
        Serial.printf("load kmodel failed2\n");
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
        rectangle_t in, cut_rect;
        in.x = info.obj[i].x;
        in.y = info.obj[i].y;
        in.w = info.obj[i].w;
        in.h = info.obj[i].h;

        extend_box(&in, &cut_rect, 0.08);
        lcd.drawRect(cut_rect.x, cut_rect.y, cut_rect.w, cut_rect.h, 0xf800);

        Image *img_cut = img_ai->cut(cut_rect);
        if(NULL == img_cut)
        {
            lcd.setCursor(0,0);
            lcd.printf("cut img failed\n");

            Serial.printf("cut img failed\n");
            break;
        }

        if(0x00 != img_cut->resize(img_128x128, 128, 128, false))
        {
            delete img_cut;

            lcd.setCursor(0,0);
            lcd.printf("resize img failed\n");

            Serial.printf("resize img failed\n");
            break;
        }
        delete img_cut;

        if(0x00 != landmark.run_kmodel(img_128x128))
        {

            lcd.setCursor(0,0);
            lcd.printf("run landmark failed\n");

            Serial.printf("run landmark failed\n");
            break;
        }

        float *result;
        size_t output_size;

        if(0x00 != landmark.get_result((uint8_t **)&result, &output_size))
        {
            lcd.setCursor(0,0);
            lcd.printf("get landmark result failed\n");

            Serial.printf("get landmark result failed\n");
            break;
        }

        for (int j = 0; j < 68; j++)
        {
            int x = int(KPU_Activation::sigmoid(result[2 * j]) * cut_rect.w + cut_rect.x);
            int y = int(KPU_Activation::sigmoid(result[2 * j + 1]) * cut_rect.h + cut_rect.y);

            lcd.fillCircle(x, y, 2, 0x001f);
        }
    }

    lcd.refresh();
}
