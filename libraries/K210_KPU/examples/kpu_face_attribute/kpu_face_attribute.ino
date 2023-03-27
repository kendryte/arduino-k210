#include "Arduino.h"

#include "Image.h"

#include "GC0328.h"
#include "OV2640.h"

#include "ST7789V.h"

#include "KPU.h"
#include "KPU_Face.h"

#include "FS.h"
#include "FFat.h"

using namespace K210;

// GC0328 cam;
OV2640 cam;
ST7789V lcd(240, 320);

Image *img_ai, *img_display, *img_128x128;

// 定义KPU对象
KPU_Base landmark;
KPU_Yolo2 yolo2;
KPU_Face face;

// 人脸属性
const char* pos_face_attr[] = {"Male ", "Mouth Open ", "Smiling ", "Glasses"};
const char* neg_face_attr[] = {"Female ", "Mouth Closed", "No Smile", "No Glasses"};

// YOLOv2模型的anchor
static float anchor[9*2] = {0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875, 0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875, 0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937};

// 扩展矩形框
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

// 人脸关键点坐标
#define PIC_SIZE    (128)
static uint16_t dst_point[] ={
        int(38.2946 * PIC_SIZE / 112), int(51.6963 * PIC_SIZE / 112),
        int(73.5318 * PIC_SIZE / 112), int(51.5014 * PIC_SIZE / 112),
        int(56.0252 * PIC_SIZE / 112), int(71.7366 * PIC_SIZE / 112),
        int(41.5493 * PIC_SIZE / 112), int(92.3655 * PIC_SIZE / 112),
        int(70.7299 * PIC_SIZE / 112), int(92.2041 * PIC_SIZE / 112)
};
#undef PIC_SIZE

// 初始化
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
    lcd.setTextSize(2);
    lcd.setTextColor(0x07E0);
    lcd.setCursor(0,0);
    lcd.fillScreen(0xFFFF);

    // 初始化文件系统
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

    // 初始化图像
    img_ai = new Image(cam.width(), cam.height(), IMAGE_FORMAT_R8G8B8, buff.ai.r);
    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);

    img_128x128 = new Image(128, 128, IMAGE_FORMAT_R8G8B8, true);
    if(NULL == img_128x128)
    {
        lcd.printf("new image 128x128 failed\n");
        Serial.printf("new image 128x128 failed\n");
        while(1) {}
    }

    // 加载人脸检测模型
    if(0x00 != yolo2.load_kmodel(FFat, "/KPU/yolo_face_detect/face_detect_320x240.kmodel"))
    {
        lcd.printf("load kmodel failed\n");
        Serial.printf("load kmodel failed\n");
        while(1) {}
    }

    // 初始化YOLOv2参数
    if(0x00 != yolo2.begin(anchor, sizeof(anchor) / sizeof(float)))
    {
        lcd.printf("yolo2 init failed\n");
        Serial.printf("yolo2 init failed\n");
        while(1) {}
    }

    // 加载人脸关键点模型
    if(0x00 != landmark.load_kmodel(FFat, "/KPU/face_attribute/ld5.kmodel"))
    {
        lcd.printf("load kmodel failed2\n");
        Serial.printf("load kmodel failed2\n");
        while(1) {}
    }

    // 加载人脸属性检测模型
    if(0x00 != face.load_kmodel(FFat, "/KPU/face_attribute/fac.kmodel"))
    {
        lcd.printf("load kmodel failed3\n");
        Serial.printf("load kmodel failed3\n");
        while(1) {}
    }

    lcd.setFrameBuffer(img_display);
}

void loop()
{
    obj_info_t info;

    // 拍摄照片
    if(0x00 != cam.snapshot())
    {
        lcd.setCursor(0,0);
        lcd.printf("camera snapshot failed\n");
        lcd.refresh();

        Serial.printf("camera snapshot failed\n");
        return;
    }

    // 运行YOLOv2模型
    if(0x00 != yolo2.run(img_ai, &info, DMAC_CHANNEL5))
    {
        lcd.setCursor(0,0);
        lcd.printf("yolo2 run failed\n");
        lcd.refresh();

        Serial.printf("yolo2 run failed\n");
        return;
    }

    // 遍历检测到的目标
    for(int i = 0; i < info.obj_number; i++)
    {
        rectangle_t in, cut_rect;
        in.x = info.obj[i].x;
        in.y = info.obj[i].y;
        in.w = info.obj[i].w;
        in.h = info.obj[i].h;

        // 扩展矩形框
        extend_box(&in, &cut_rect, 0.08);
        lcd.drawRect(cut_rect.x, cut_rect.y, cut_rect.w, cut_rect.h, 0xf800);

        // 裁剪图像
        Image *img_cut = img_ai->cut(cut_rect);
        if(NULL == img_cut)
        {
            lcd.setCursor(0,0);
            lcd.printf("cut img failed\n");

            Serial.printf("cut img failed\n");
            break;
        }

        // 缩放图像
        if(0x00 != img_cut->resize(img_128x128, 128, 128, false))
        {
            delete img_cut;

            lcd.setCursor(0,0);
            lcd.printf("resize img failed\n");

            Serial.printf("resize img failed\n");
            break;
        }
        delete img_cut;

        // 运行人脸关键点检测模型
        if(0x00 != landmark.run_kmodel(img_128x128))
        {

            lcd.setCursor(0,0);
            lcd.printf("run landmark failed\n");

            Serial.printf("run landmark failed\n");
            break;
        }

        float *result;
        size_t output_size;

        // 获取人脸关键点坐标
        if(0x00 != landmark.get_result((uint8_t **)&result, &output_size))
        {
            lcd.setCursor(0,0);
            lcd.printf("get landmark result failed\n");

            Serial.printf("get landmark result failed\n");
            break;
        }

        uint16_t face_key_point[5][2];
        for (int j = 0; j < 5; j++)
        {
            face_key_point[j][0] = uint16_t(KPU_Activation::sigmoid(result[2 * j]) * cut_rect.w + cut_rect.x);
            face_key_point[j][1] = uint16_t(KPU_Activation::sigmoid(result[2 * j + 1]) * cut_rect.h + cut_rect.y);
            // 在LCD上标记人脸关键点坐标
            lcd.fillCircle(face_key_point[j][0], face_key_point[j][1], 2, 0x001f);
        }

        float T[3][3];
        // 计算仿射变换矩阵
        face.calc_affine_transform((uint16_t*)face_key_point, (uint16_t*)dst_point, 5, (float*)T);
        // 应用仿射变换
        face.apply_affine_transform(img_ai, img_128x128, (float*)T);

        // 运行人脸属性检测模型
        if(0x00 != face.run_kmodel(img_128x128))
        {
            lcd.setCursor(0,0);
            lcd.printf("run face failed\n");

            Serial.printf("run face failed\n");
            break;
        }

        // 获取人脸属性检测结果
        if(0x00 != face.get_result((uint8_t **)&result, &output_size))
        {
            lcd.setCursor(0,0);
            lcd.printf("get face result failed\n");

            Serial.printf("get face result failed\n");
            break;
        }

        // 显示人脸属性检测结果
        for (int j = 0; j < 4; j++)
        {
            float th = KPU_Activation::sigmoid(result[j]);
            if (th > 0.7)
            {
                lcd.setCursor(cut_rect.x + cut_rect.w, cut_rect.y + j * 16);
                lcd.setTextColor(0xf800);
                lcd.printf("%s", pos_face_attr[j]);
            }
            else
            {
                lcd.setCursor(cut_rect.x + cut_rect.w, cut_rect.y + j * 16);
                lcd.setTextColor(0x001f);
                lcd.printf("%s", neg_face_attr[j]);
            }
        }
    }

    lcd.refresh();
}
