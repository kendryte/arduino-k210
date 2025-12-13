#include "Arduino.h"

#include "Image.h"

#include "GC0328.h"
// #include "OV2640.h"

#include "ST7789V.h"
#include "FS.h"
#include "FFat.h"

using namespace K210;

GC0328 cam;
// OV2640 cam;
ST7789V lcd(240, 320);

// We only need img_sensor and img_display for the visual check
Image *img_sensor, *img_display;

void setup()
{
    camera_buffers_t buff;

    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    // 1. Initialize File System
    if(!FFat.begin()){
        Serial.printf("FFat Mount Failed. Check SD card or initialization.\n");
        while(1) {}
    }
    Serial.printf("FFat Mounted successfully.\n");
    
    // 2. Allocate Buffer for Sensor and Display
    // Allocating a large buffer for the sensor (640x480x2 = 614400 bytes)
    camera_buffers_t _set_cam_buffrt;
    _set_cam_buffrt.disply = (uint8_t *)rt_malloc_align(640*480*2, 8); 
    if(NULL == _set_cam_buffrt.disply)
    {
        Serial.printf("malloc sensor buffer failed\n");
        while(1) {}
    }

    _set_cam_buffrt.ai.r = NULL;
    _set_cam_buffrt.ai.g = NULL;
    _set_cam_buffrt.ai.b = NULL;

    // 3. Reset Camera
    // Note: Changed FRAMESIZE_QVGA to FRAMESIZE_VGA to match your initial working example
    if(0x00 != cam.reset(FRAMESIZE_VGA, &_set_cam_buffrt)) 
    {
        Serial.printf("camera reset failed\n");
        while(1) {}
    }
    
    // 4. Initialize Image objects
    img_sensor = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, _set_cam_buffrt.disply);
    
    // Allocate a buffer for the LCD (320x240 RGB565)
    uint8_t *lcd_buffer = (uint8_t *)rt_malloc_align(320*240*2, 8);
    img_display = new Image(320, 240, IMAGE_FORMAT_RGB565, lcd_buffer);

    // 5. Initialize LCD
    lcd.begin();
    lcd.invertDisplay(1);
    lcd.setRotation(1);
    lcd.setTextSize(2);
    lcd.setFrameBuffer(img_display);
    
    lcd.setCursor(0, 0);
    lcd.printf("Starting Raw Cut Test...");
    lcd.refresh();
}

// Helper function to save raw image data to file
void save_raw_image(Image *img, const char *filename)
{
    fs::File file = FFat.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.printf("ERROR: Failed to open file %s for writing.\n", filename);
        return;
    }

    size_t size = img->w * img->h * img->bpp;
    
    if (size != file.write(img->pixel, size))
    {
        Serial.printf("ERROR: Failed to write all data to %s.\n", filename);
    }
    else
    {
        Serial.printf("Saved raw data to %s (Size: %ld bytes, %dx%d, BPP: %d)\n", 
                        filename, size, img->w, img->h, img->bpp);
    }
    
    file.close();
}

void loop()
{
    static bool test_done = false;

    if (test_done) {
        delay(1000);
        return;
    }

    Serial.printf("\n--- Starting Raw Binary Cut Test (Validated) ---\n");
    
    // 1. Snapshot
    if(0x00 != cam.snapshot())
    {
        Serial.printf("camera snapshot failed\n");
        return;
    }
    Serial.printf("Source snapshot complete (%dx%d, RGB565)\n", img_sensor->w, img_sensor->h);

    // 2. Define the cut region (Center 320x240 region from 640x480 image)
    rectangle_t cut_rect = {
        .x = (uint32_t)(img_sensor->w - 320) / 2,
        .y = (uint32_t)(img_sensor->h - 240) / 2,
        .w = 320,
        .h = 240
    };

    Image *img_temp = NULL;
    uint64_t start_time;
    char filename[40]; // Increased buffer size for full path

    // Array of formats to test
    struct {
        image_format_t format;
        const char* name;
    } tests[] = {
        {IMAGE_FORMAT_RGBP888, "rgbp888"},
        {IMAGE_FORMAT_RGB565, "rgb565"},
        {IMAGE_FORMAT_RGB888, "rgb888"},
        {IMAGE_FORMAT_GRAYSCALE, "gray"}
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        // Use the generic wrapper to call the cut and convert function
        start_time = sysctl_get_time_us();
        img_temp = img_sensor->cut_to_new_format(cut_rect, tests[i].format);
        Serial.printf("cut_to_%s took %ld us\n", tests[i].name, sysctl_get_time_us() - start_time);

        if (NULL == img_temp) {
            Serial.printf("ERROR: cut_to_new_format returned NULL for %s.\n", tests[i].name);
            continue;
        }

        // --- VALIDATION CHECK ---
        if (img_temp->w == 0 || img_temp->h == 0 || img_temp->bpp == 0)
        {
            Serial.printf("CRITICAL ERROR: Returned Image object has ZERO dimensions or BPP (%dx%d, BPP: %d).\n", 
                          img_temp->w, img_temp->h, img_temp->bpp);
            delete img_temp;
            continue;
        }
        // -------------------------

        snprintf(filename, sizeof(filename), "/test_%s.raw", tests[i].name);
        
        save_raw_image(img_temp, filename);
        
        delete img_temp;
        img_temp = NULL;
    }

    // --- Display Result (Final step to verify visual cut) ---
    // Copy the central cut area to the LCD buffer
    img_sensor->cut_to_rgb565(img_display,cut_rect,  false);
    
    lcd.setCursor(0, 0);
    lcd.printf("Test Complete. Check SD.");
    lcd.refresh();

    test_done = true;
    Serial.printf("--- All Raw Binary Tests Complete ---\n");
}
