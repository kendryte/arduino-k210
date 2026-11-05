#include "Arduino.h"
#include "Image.h"
#include "ST7789V.h"
#include "FFat.h"

#include "GC0328.h"
// #include "OV2640.h"

using namespace K210;

// 定义摄像头和LCD
GC0328 cam;
// OV2640 cam;

ST7789V lcd(240, 320);

// 定义图像指针
Image *img_camera_rgb565; // 从摄像头获取的原始RGB565图像

// 测试配置结构
struct ResizeTestConfig {
    const char* test_name;
    uint32_t src_width;
    uint32_t src_height;
    image_format_t src_format;
    const char* src_filename;
    
    uint32_t dst_width;
    uint32_t dst_height;
    image_format_t dst_format;
    const char* dst_filename;
    
    const char* description;
};

// 保存图像到文件系统
bool save_image_to_file(Image *img, const char* filename)
{
    if (!img || !img->data()) {
        Serial.printf("ERROR: Invalid image for saving: %s\n", filename);
        return false;
    }
    
    // 根据格式选择保存方法
    int result = -1;

    switch (img->format()) {
    case IMAGE_FORMAT_INVALID:
    break;
    case IMAGE_FORMAT_RGB888:
      // 保存为BMP格式
      result = img->save_bmp(FFat, filename);
      if (result == 0) {
        Serial.printf("Saved RGB888 as BMP: %s\n", filename);
      }
      break;

    case IMAGE_FORMAT_RGB565:
    case IMAGE_FORMAT_GRAYSCALE:
    case IMAGE_FORMAT_RGBP888:
      // 对于非RGB888格式，先转换为RGB888再保存
      Image *rgb888_img = img->to_rgb888();
      if (rgb888_img) {
        char bmp_filename[64];
        snprintf(bmp_filename, sizeof(bmp_filename), "%s.bmp", filename);
        result = rgb888_img->save_bmp(FFat, bmp_filename);
        delete rgb888_img;

        if (result == 0) {
          Serial.printf("Saved as BMP: %s\n", bmp_filename);
        }
      }
      break;
    }

    // 尝试保存为JPEG
    char jpeg_filename[64];
    snprintf(jpeg_filename, sizeof(jpeg_filename), "%s.jpg", filename);
    int jpeg_result = img->save_jpeg(FFat, jpeg_filename, 85);
    if (jpeg_result == 0) {
        Serial.printf("Saved as JPEG: %s\n", jpeg_filename);
    }
    
    return (result == 0 || jpeg_result == 0);
}

// 显示测试信息到LCD和串口
void display_test_status(const char* test_name, const char* description, 
                        uint32_t src_w, uint32_t src_h, image_format_t src_fmt,
                        uint32_t dst_w, uint32_t dst_h, image_format_t dst_fmt,
                        bool success, const char* filename = NULL)
{
    Serial.println("\n" + String(50, '='));
    Serial.printf("Test: %s\n", test_name);
    Serial.printf("Description: %s\n", description);
    Serial.printf("Source: %dx%d, Format: %d\n", src_w, src_h, src_fmt);
    Serial.printf("Target: %dx%d, Format: %d\n", dst_w, dst_h, dst_fmt);
    if (filename) {
        Serial.printf("Saved to: %s\n", filename);
    }
    Serial.printf("Status: %s\n", success ? "SUCCESS" : "FAILED");
    Serial.println(String(50, '='));

    // 更新LCD显示
    lcd.fillScreen(success ? 0x0000 : 0xF800); // 成功:黑色背景，失败:红色背景
    lcd.setCursor(0, 0);
    lcd.setTextColor(success ? 0xFFFF : 0x0000);
    
    lcd.printf("Test: %s\n", test_name);
    lcd.printf("Src: %dx%d Fmt:%d\n", src_w, src_h, src_fmt);
    lcd.printf("Dst: %dx%d Fmt:%d\n", dst_w, dst_h, dst_fmt);
    lcd.printf("Status: %s\n", success ? "PASS" : "FAIL");
    
    if (filename) {
        lcd.printf("File: %s\n", filename);
    }
    
    lcd.refresh();
}

// 从摄像头获取原始RGB565图像
bool capture_original_image()
{
    Serial.println("\n=== Capturing Original Image ===");
    
    if (0x00 != cam.snapshot()) {
        Serial.println("ERROR: Camera snapshot failed");
        return false;
    }
    
    Serial.println("Camera snapshot successful");
    return true;
}

// 测试1: 从RGB565转换到其他格式
void test_convert_from_rgb565(Image *original_rgb565)
{
    Serial.println("\n=== Test 1: Convert from RGB565 to other formats ===");
    
    if (!original_rgb565 || !original_rgb565->data()) {
        Serial.println("ERROR: Invalid original image");
        return;
    }
    
    // 保存原始RGB565图像
    bool save_original = save_image_to_file(original_rgb565, "/img_original_rgb565");
    display_test_status("Save Original RGB565", "Save camera RGB565 to file",
                       original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                       original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                       save_original, "/img_original_rgb565");
    
    delay(1000);
    
    // 转换到灰度图
    Serial.println("\n--- Converting RGB565 to GRAYSCALE ---");
    Image *img_gray = original_rgb565->to_grayscale();
    if (img_gray) {
        bool save_result = save_image_to_file(img_gray, "/img_rgb565_to_gray");
        display_test_status("RGB565->GRAY", "Convert RGB565 to Grayscale",
                           original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                           img_gray->width(), img_gray->height(), img_gray->format(),
                           save_result, "/img_rgb565_to_gray");
        
        // 显示灰度图
        Image *gray_display = img_gray->to_rgb565();
        if (gray_display) {
            lcd.drawImage(gray_display, 0, 120);
            lcd.refresh();
            delete gray_display;
        }
        
        delete img_gray;
    } else {
        display_test_status("RGB565->GRAY", "Convert RGB565 to Grayscale",
                           original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                           0, 0, IMAGE_FORMAT_INVALID, false);
    }
    
    delay(1500);
    
    // 转换到RGB888
    Serial.println("\n--- Converting RGB565 to RGB888 ---");
    Image *img_rgb888 = original_rgb565->to_rgb888();
    if (img_rgb888) {
        bool save_result = save_image_to_file(img_rgb888, "/img_rgb565_to_rgb888");
        display_test_status("RGB565->RGB888", "Convert RGB565 to RGB888",
                           original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                           img_rgb888->width(), img_rgb888->height(), img_rgb888->format(),
                           save_result, "/img_rgb565_to_rgb888");
        delete img_rgb888;
    }
    
    delay(1000);
    
    // 转换到RGBP888
    Serial.println("\n--- Converting RGB565 to RGBP888 ---");
    Image *img_rgbp888 = original_rgb565->to_rgbp888();
    if (img_rgbp888) {
        bool save_result = save_image_to_file(img_rgbp888, "/img_rgb565_to_rgbp888");
        display_test_status("RGB565->RGBP888", "Convert RGB565 to Planar RGB888",
                           original_rgb565->width(), original_rgb565->height(), original_rgb565->format(),
                           img_rgbp888->width(), img_rgbp888->height(), img_rgbp888->format(),
                           save_result, "/img_rgb565_to_rgbp888");
        delete img_rgbp888;
    }
    
    delay(1000);
}

// 测试2: 从各种格式进行尺寸调整和格式转换
void test_resize_from_different_formats(Image *original_rgb565)
{
    Serial.println("\n=== Test 2: Resize from different formats ===");
    
    if (!original_rgb565) return;
    
    // 创建不同格式的基础图像
    Image *base_gray = original_rgb565->to_grayscale();
    Image *base_rgb888 = original_rgb565->to_rgb888();
    Image *base_rgbp888 = original_rgb565->to_rgbp888();
    
    // 测试配置数组
    ResizeTestConfig test_cases[] = {
        // 从灰度图开始
        {
            "Gray->RGB565", 
            base_gray ? base_gray->width() : 0, 
            base_gray ? base_gray->height() : 0, 
            IMAGE_FORMAT_GRAYSCALE,
            "/img_gray_original",
            160, 120, 
            IMAGE_FORMAT_RGB565,
            "/img_gray_to_rgb565_160x120",
            "Resize GRAY to RGB565 160x120"
        },
        {
            "Gray->RGB888", 
            base_gray ? base_gray->width() : 0, 
            base_gray ? base_gray->height() : 0, 
            IMAGE_FORMAT_GRAYSCALE,
            "/img_gray_original",
            128, 96, 
            IMAGE_FORMAT_RGB888,
            "/img_gray_to_rgb888_128x96",
            "Resize GRAY to RGB888 128x96"
        },
        {
            "Gray->RGBP888", 
            base_gray ? base_gray->width() : 0, 
            base_gray ? base_gray->height() : 0, 
            IMAGE_FORMAT_GRAYSCALE,
            "/img_gray_original",
            224, 224, 
            IMAGE_FORMAT_RGBP888,
            "/img_gray_to_rgbp888_224x224",
            "Resize GRAY to RGBP888 224x224 (AI Input)"
        },
        
        // 从RGB888开始
        {
            "RGB888->Gray", 
            base_rgb888 ? base_rgb888->width() : 0, 
            base_rgb888 ? base_rgb888->height() : 0, 
            IMAGE_FORMAT_RGB888,
            "/img_rgb888_original",
            240, 180, 
            IMAGE_FORMAT_GRAYSCALE,
            "/img_rgb888_to_gray_240x180",
            "Resize RGB888 to GRAY 240x180"
        },
        {
            "RGB888->RGB565", 
            base_rgb888 ? base_rgb888->width() : 0, 
            base_rgb888 ? base_rgb888->height() : 0, 
            IMAGE_FORMAT_RGB888,
            "/img_rgb888_original",
            192, 144, 
            IMAGE_FORMAT_RGB565,
            "/img_rgb888_to_rgb565_192x144",
            "Resize RGB888 to RGB565 192x144"
        },
        {
            "RGB888->RGBP888", 
            base_rgb888 ? base_rgb888->width() : 0, 
            base_rgb888 ? base_rgb888->height() : 0, 
            IMAGE_FORMAT_RGB888,
            "/img_rgb888_original",
            96, 96, 
            IMAGE_FORMAT_RGBP888,
            "/img_rgb888_to_rgbp888_96x96",
            "Resize RGB888 to RGBP888 96x96"
        },
        
        // 从RGBP888开始
        {
            "RGBP888->Gray", 
            base_rgbp888 ? base_rgbp888->width() : 0, 
            base_rgbp888 ? base_rgbp888->height() : 0, 
            IMAGE_FORMAT_RGBP888,
            "/img_rgbp888_original",
            64, 64, 
            IMAGE_FORMAT_GRAYSCALE,
            "/img_rgbp888_to_gray_64x64",
            "Resize RGBP888 to GRAY 64x64"
        },
        {
            "RGBP888->RGB565", 
            base_rgbp888 ? base_rgbp888->width() : 0, 
            base_rgbp888 ? base_rgbp888->height() : 0, 
            IMAGE_FORMAT_RGBP888,
            "/img_rgbp888_original",
            320, 240, 
            IMAGE_FORMAT_RGB565,
            "/img_rgbp888_to_rgb565_320x240",
            "Resize RGBP888 to RGB565 320x240"
        },
        {
            "RGBP888->RGB888", 
            base_rgbp888 ? base_rgbp888->width() : 0, 
            base_rgbp888 ? base_rgbp888->height() : 0, 
            IMAGE_FORMAT_RGBP888,
            "/img_rgbp888_original",
            256, 192, 
            IMAGE_FORMAT_RGB888,
            "/img_rgbp888_to_rgb888_256x192",
            "Resize RGBP888 to RGB888 256x192"
        },
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        ResizeTestConfig &test = test_cases[i];
        
        Serial.printf("\n--- Test Case %d/%d: %s ---\n", i+1, num_cases, test.test_name);
        
        // 获取源图像
        Image *src_image = NULL;
        if (test.src_format == IMAGE_FORMAT_GRAYSCALE && base_gray) {
            src_image = base_gray;
        } else if (test.src_format == IMAGE_FORMAT_RGB888 && base_rgb888) {
            src_image = base_rgb888;
        } else if (test.src_format == IMAGE_FORMAT_RGBP888 && base_rgbp888) {
            src_image = base_rgbp888;
        } else if (test.src_format == IMAGE_FORMAT_RGB565) {
            src_image = original_rgb565;
        }
        
        if (!src_image) {
            Serial.printf("SKIP: No source image for format %d\n", test.src_format);
            continue;
        }
        
        // 执行尺寸调整和格式转换
        Image *result_image = new Image();
        int resize_result = -1;
        
        // 使用通用函数
        resize_result = Image::resize_to_new_format(
            src_image, 
            result_image, 
            test.dst_width, 
            test.dst_height, 
            test.dst_format, 
            true
        );
        
        if (resize_result == 0 && result_image && result_image->data()) {
            // 保存结果
            bool save_result = save_image_to_file(result_image, test.dst_filename);
            
            // 显示状态
            display_test_status(test.test_name, test.description,
                              test.src_width, test.src_height, test.src_format,
                              test.dst_width, test.dst_height, test.dst_format,
                              save_result, test.dst_filename);
            
            // 显示结果图像（转换为RGB565显示）
            if (result_image->format() != IMAGE_FORMAT_RGB565) {
                Image *display_img = result_image->to_rgb565();
                if (display_img) {
                    // 居中显示缩略图
                    int x = (lcd.width() - display_img->width()/2) / 2;
                    int y = 120;
                    lcd.drawImage(display_img, x, y);
                    lcd.refresh();
                    delete display_img;
                }
            } else {
                // 直接显示RGB565
                int x = (lcd.width() - result_image->width()/2) / 2;
                int y = 120;
                lcd.drawImage(result_image, x, y);
                lcd.refresh();
            }
            
            delete result_image;
        } else {
            display_test_status(test.test_name, test.description,
                              test.src_width, test.src_height, test.src_format,
                              test.dst_width, test.dst_height, test.dst_format,
                              false);
            
            if (result_image) delete result_image;
        }
        
        delay(1200); // 每个测试之间的延迟
    }
    
    // 清理基础图像
    if (base_gray) delete base_gray;
    if (base_rgb888) delete base_rgb888;
    if (base_rgbp888) delete base_rgbp888;
}

// 测试3: 测试快捷函数
void test_convenience_functions(Image *original_rgb565)
{
    Serial.println("\n=== Test 3: Convenience Functions ===");
    
    if (!original_rgb565) return;
    
    // 保存原始图像
    save_image_to_file(original_rgb565, "/img_test3_original");
    
    lcd.fillScreen(0x0000);
    lcd.setCursor(0, 0);
    lcd.setTextColor(0x07E0);
    lcd.printf("Test 3: Convenience Functions\n");
    lcd.refresh();
    
    // 测试各种快捷函数
    struct QuickTest {
        const char* name;
        Image* (*func)(Image*, uint32_t, uint32_t);
        uint32_t width;
        uint32_t height;
        const char* filename;
    };
    
    QuickTest quick_tests[] = {
        {"resize_to_grayscale", [](Image* img, uint32_t w, uint32_t h) { 
            return img->resize_to_grayscale(w, h); }, 
            128, 128, "/img_quick_gray_128"},
        {"resize_to_rgb565", [](Image* img, uint32_t w, uint32_t h) { 
            return img->resize_to_rgb565(w, h); }, 
            160, 120, "/img_quick_rgb565_160x120"},
        {"resize_to_rgb888", [](Image* img, uint32_t w, uint32_t h) { 
            return img->resize_to_rgb888(w, h); }, 
            192, 144, "/img_quick_rgb888_192x144"},
        {"resize_to_rgbp888", [](Image* img, uint32_t w, uint32_t h) { 
            return img->resize_to_rgbp888(w, h); }, 
            224, 224, "/img_quick_rgbp888_224"},
    };
    
    for (int i = 0; i < sizeof(quick_tests)/sizeof(quick_tests[0]); i++) {
        QuickTest &test = quick_tests[i];
        
        Serial.printf("\n--- Quick Test: %s (%dx%d) ---\n", 
                     test.name, test.width, test.height);
        
        Image *result = test.func(original_rgb565, test.width, test.height);
        if (result) {
            bool save_result = save_image_to_file(result, test.filename);
            
            lcd.setCursor(0, 30 + i*20);
            lcd.printf("%s: %s", test.name, save_result ? "PASS" : "FAIL");
            lcd.refresh();
            
            if (save_result) {
                Serial.printf("Saved: %s\n", test.filename);
            }
            
            delete result;
        } else {
            lcd.setCursor(0, 30 + i*20);
            lcd.printf("%s: FAIL", test.name);
            lcd.refresh();
            Serial.printf("Failed: %s\n", test.name);
        }
        
        delay(800);
    }
    
    delay(2000);
}

void setup()
{
    camera_buffers_t buff;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Serial.println("\n========================================");
    Serial.println("Starting resize_to_new_format Test Suite");
    Serial.println("========================================");

    // 初始化LCD
    lcd.begin();
    lcd.setRotation(3);
    lcd.setTextSize(3);
    lcd.setTextColor(0x07E0);
    lcd.setCursor(0, 0);
    lcd.fillScreen(0x0000);
    lcd.printf("Starting Test...");
    lcd.refresh();

    // 挂载文件系统
    if(!FFat.begin(true)){  // 参数true表示如果挂载失败则格式化
        lcd.printf("FFat Mount Failed\n");
        Serial.println("FFat Mount Failed");
        while(1) {}
    }
    Serial.println("FFat mounted successfully");

    // 初始化摄像头
    lcd.setCursor(0, 20);
    lcd.printf("Initializing camera...");
    lcd.refresh();
    
    if(0x00 != cam.reset(FRAMESIZE_QVGA)) {
        lcd.printf("Camera reset failed\n");
        Serial.println("Camera reset failed");
        while(1) {}
    }
    cam.set_vflip(true);
    cam.set_hmirror(true);
    Serial.println("Camera initialized");

    // 获取摄像头缓存
    cam.get_buffers(&buff);
    if((NULL == buff.disply)) {
        lcd.printf("Get camera buffers failed\n");
        Serial.println("Get camera buffers failed");
        while(1) {}
    }

    // 初始化图像指针 - 只使用RGB565显示缓冲区
    img_camera_rgb565 = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);
    
    lcd.setCursor(0, 40);
    lcd.printf("Setup complete!");
    lcd.refresh();
    
    Serial.println("Setup complete. Ready to start tests.");
    delay(1000);
}

void loop()
{
    static int test_phase = 0;
    
    Serial.printf("\n\n=== TEST PHASE %d STARTING ===\n", test_phase);
    
    // 捕获新图像
    if (!capture_original_image()) {
        delay(1000);
        return;
    }
    
    switch(test_phase) {
        case 0:
            lcd.fillScreen(0x0000);
            lcd.setCursor(0, 0);
            lcd.setTextColor(0x07E0);
            lcd.printf("Phase 1: Basic Conversion\n");
            lcd.refresh();
            test_convert_from_rgb565(img_camera_rgb565);
            break;
            
        case 1:
            lcd.fillScreen(0x0000);
            lcd.setCursor(0, 0);
            lcd.setTextColor(0x07E0);
            lcd.printf("Phase 2: Resize & Convert\n");
            lcd.refresh();
            test_resize_from_different_formats(img_camera_rgb565);
            break;
            
        case 2:
            lcd.fillScreen(0x0000);
            lcd.setCursor(0, 0);
            lcd.setTextColor(0x07E0);
            lcd.printf("Phase 3: Quick Functions\n");
            lcd.refresh();
            test_convenience_functions(img_camera_rgb565);
            break;
            
        case 3:
            lcd.fillScreen(0x0000);
            lcd.setCursor(0, 0);
            lcd.setTextColor(0x07E0);
            lcd.printf("Phase 4: File Listing\n");
            lcd.refresh();
            break;
            
        default:
            // 所有测试完成
            lcd.fillScreen(0x0000);
            lcd.setCursor(0, 0);
            lcd.setTextColor(0xFFFF);
            lcd.printf("All Tests Complete!\n\n");
            lcd.printf("Check FFat for saved\n");
            lcd.printf("image files.\n\n");
            lcd.printf("Restarting in 5s...");
            lcd.refresh();
            
            Serial.println("\n========================================");
            Serial.println("ALL TESTS COMPLETE!");
            Serial.println("Check FFat file system for saved images");
            Serial.println("========================================");
            
            delay(5000);
            test_phase = -1; // 重置测试阶段
            break;
    }
    
    test_phase++;
    delay(2000); // 测试阶段之间的延迟
}
