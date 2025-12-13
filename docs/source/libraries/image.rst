######
Image
######

`Image` 库用于处理K210的图像，为Camera和KPU提供图像类以及操作方法，包括图像裁剪缩放、RGB像素格式转换、BMP/JPEG读取保存等功能。

构造函数
^^^^^^^^^^^^

描述
======

构造函数

语法
======

- 1.通过create参数决定是否自动创建图像缓存

.. code-block:: arduino

    Image(uint32_t width, uint32_t height, image_format_t f, bool create = false);

- 2.使用用户提供的缓存创建Image

.. code-block:: arduino

    Image(uint32_t width, uint32_t height, image_format_t f, uint8_t *buffer);

参数
======

* ``width`` 图像宽

* ``height`` 图像高

* ``image_format_t`` 图像格式

.. code-block:: arduino

    enum image_format_t : uint32_t
    {
        IMAGE_FORMAT_GRAYSCALE = 0, // bpp 1
        IMAGE_FORMAT_RGB565,        // bpp 2
        IMAGE_FORMAT_RGB888,        // bpp 3
        IMAGE_FORMAT_RGBP888,        // bpp 3
        IMAGE_FORMAT_INVALID = 4,
    };

* ``create`` 是否创建图像内存

* ``buffer`` 用户分配的图像内存


返回值
========

无

示例说明
============

使用buff.disply缓存创建显示Image

.. code-block:: arduino

    img_display = new Image(cam.width(), cam.height(), IMAGE_FORMAT_RGB565, buff.disply);

属性获取方法
^^^^^^^^^^^^^^

描述
======

获取图像属性信息

方法
======

.. code-block:: arduino

    // 获取图像宽度
    uint32_t width() const;
    
    // 获取图像高度
    uint32_t height() const;
    
    // 获取图像每像素字节数
    uint32_t bpp() const;
    
    // 获取图像格式
    image_format_t format() const;
    
    // 获取图像数据指针
    uint8_t* data() const;
    uint8_t* pixel() const;
    
    // 获取图像数据总大小（字节）
    uint32_t size() const;

返回值
========

相应的属性值

示例说明
============

.. code-block:: arduino

    Image *img = new Image(320, 240, IMAGE_FORMAT_RGB565, true);
    
    uint32_t w = img->width();     // 返回 320
    uint32_t h = img->height();    // 返回 240
    uint32_t s = img->size();      // 返回 320 * 240 * 2 = 153600
    uint8_t* pixels = img->data(); // 返回图像数据指针

cut
^^^^^^^^^

描述
======

图像裁剪

语法
======

- 1.从src裁剪r矩形大小的图像到dst，create决定是否创建dst图像缓存

.. code-block:: arduino

    static int cut(Image *src, Image *dst, rectangle_t &r, bool create);

- 2.src_Image对象调用cut方法裁剪r矩形大小图像到dst

.. code-block:: arduino

    int cut(Image *dst, rectangle_t &r, bool create = true);

- 3.src_Image对象调用cut方法裁剪r矩形大小图像并返回结果图像

.. code-block:: arduino

    Image * cut(rectangle_t &r);

参数
======

* ``src`` 源图像

* ``dst`` 生成的图像

* ``rectangle_t`` 裁剪的矩形框信息

.. code-block:: arduino

    typedef struct rectangle
    {
        uint32_t x;
        uint32_t y;
        uint32_t w;
        uint32_t h;
    } rectangle_t;

* ``create`` 是否为生成的图像创建内存


返回值
========

- 语法1,2  
    0：成功，其他值：失败

- 语法3  
    目标图像指针


示例说明
============

.. code-block:: arduino

    Image *img_cut = img_ai->cut(cut_rect);

cut_to_new_format
^^^^^^^^^^^^^^^^^^^

描述
======

裁剪并转换图像格式

语法
======

- 1.从src裁剪r矩形大小的图像并转换为新格式到dst

.. code-block:: arduino

    static int cut_to_new_format(Image *src, Image *dst, rectangle_t &r, image_format_t new_format, bool create);

- 2.src_Image对象调用cut_to_new_format方法裁剪r矩形大小图像并转换格式到dst

.. code-block:: arduino

    int cut_to_new_format(Image *dst, rectangle_t &r, image_format_t new_format, bool create = true);

- 3.src_Image对象调用cut_to_new_format方法裁剪r矩形大小图像并转换格式，返回结果图像

.. code-block:: arduino

    Image * cut_to_new_format(rectangle_t &r, image_format_t new_format);

参数
======

* ``src`` 源图像
* ``dst`` 生成的图像
* ``rectangle_t`` 裁剪的矩形框信息
* ``new_format`` 目标图像格式
* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1,2  
    0：成功，其他值：失败
- 语法3  
    目标图像指针

cut_to_grayscale
^^^^^^^^^^^^^^^^^

描述
======

裁剪并转换为灰度图

语法
======

- 1

.. code-block:: arduino

    int cut_to_grayscale(Image *dst, rectangle_t &r, bool create = true);

- 2

.. code-block:: arduino

    Image * cut_to_grayscale(rectangle_t &r);

参数
======

* ``dst`` 生成的图像
* ``rectangle_t`` 裁剪的矩形框信息
* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败
- 语法2
    目标图像指针

cut_to_rgb565
^^^^^^^^^^^^^^

描述
======

裁剪并转换为 `rgb565` 格式

语法
======

- 1

.. code-block:: arduino

    int cut_to_rgb565(Image *dst, rectangle_t &r, bool create = true);

- 2

.. code-block:: arduino

    Image * cut_to_rgb565(rectangle_t &r);

参数
======

* ``dst`` 生成的图像
* ``rectangle_t`` 裁剪的矩形框信息
* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败
- 语法2
    目标图像指针

cut_to_rgb888
^^^^^^^^^^^^^^

描述
======

裁剪并转换为 `rgb888` 格式

语法
======

- 1

.. code-block:: arduino

    int cut_to_rgb888(Image *dst, rectangle_t &r, bool create = true);

- 2

.. code-block:: arduino

    Image * cut_to_rgb888(rectangle_t &r);

参数
======

* ``dst`` 生成的图像
* ``rectangle_t`` 裁剪的矩形框信息
* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败
- 语法2
    目标图像指针

cut_to_rgbp888
^^^^^^^^^^^^^^

描述
======

裁剪并转换为 `rgbp888` 格式

语法
======

- 1

.. code-block:: arduino

    int cut_to_rgbp888(Image *dst, rectangle_t &r, bool create = true);

- 2

.. code-block:: arduino

    Image * cut_to_rgbp888(rectangle_t &r);

参数
======

* ``dst`` 生成的图像
* ``rectangle_t`` 裁剪的矩形框信息
* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败
- 语法2
    目标图像指针

resize
^^^^^^^^^

描述
======

图像缩放

语法
======

- 1.从src图resize图像到dst，create决定是否创建dst图像缓存

.. code-block:: arduino

    static int resize(Image *src, Image *dst, uint32_t width, uint32_t height, bool create);

- 2.src_Image对象调用resize方法缩放图像到dst

.. code-block:: arduino

    int resize(Image *dst, uint32_t width, uint32_t height, bool create = true);

- 3.src_Image对象调用resize方法并返回结果图像

.. code-block:: arduino

    Image * resize(uint32_t width, uint32_t height);

参数
======

* ``src`` 源图像

* ``dst`` 生成的图像

* ``width`` 目标图像宽

* ``height`` 目标图像高

* ``create`` 是否为生成的图像创建内存


返回值
========

- 语法1,2  
    0：成功，其他值：失败

- 语法3  
    目标图像指针

示例说明
============

.. code-block:: arduino

    img_128x128 = new Image(128, 128, IMAGE_FORMAT_RGBP888, true);
    img_cut->resize(img_128x128, 128, 128, false);


to_grayscale
^^^^^^^^^^^^^^

描述
======

生成灰度图

语法
======

- 1

.. code-block:: arduino

    int to_grayscale(Image *dst, bool create = true);

- 2

.. code-block:: arduino

    Image * to_grayscale(void);

参数
======

* ``dst`` 生成的图像

* ``create`` 是否为生成的图像创建内存


返回值
========

- 语法1 
    0：成功，其他值：失败

- 语法2
    目标图像指针

示例说明
============

.. code-block:: arduino

    Image *img_gray;
    img_gray = img_display->to_grayscale();

to_rgb565
^^^^^^^^^^^^^^

描述
======

转换为 `rgb565` 格式

语法
======

- 1

.. code-block:: arduino

    int to_rgb565(Image *dst, bool create = true);

- 2

.. code-block:: arduino

    Image * to_rgb565(void);

参数
======

* ``dst`` 生成的图像

* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败

- 语法2
    目标图像指针

示例说明
============

.. code-block:: arduino

    Image *img_rgb565;
    img_rgb565 = img_gray->to_rgb565();

to_rgb888
^^^^^^^^^^^^^^

描述
======

转换为 `rgb888` 格式

语法
======

- 1

.. code-block:: arduino

    int to_rgb888(Image *dst, bool create = true);

- 2

.. code-block:: arduino

    Image * to_rgb888(void);

参数
======

* ``dst`` 生成的图像

* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败

- 语法2
    目标图像指针

示例说明
============

.. code-block:: arduino

    Image *img_rgb888;
    img_rgb888 = img_gray->to_rgb888();

to_rgbp888
^^^^^^^^^^^^^^

描述
======

转换为 `rgbp888` 格式，作为KPU输入图需要的格式

语法
======

- 1

.. code-block:: arduino

    int to_rgbp888(Image *dst, bool create = true);

- 2

.. code-block:: arduino

    Image * to_rgbp888(void);

参数
======

* ``dst`` 生成的图像

* ``create`` 是否为生成的图像创建内存

返回值
========

- 语法1 
    0：成功，其他值：失败

- 语法2
    目标图像指针

示例说明
============

.. code-block:: arduino

    Image *img_rgbp888;
    img_rgbp888 = img_gray->to_rgbp888();

load_bmp
^^^^^^^^^

描述
======

读取bmp图

语法
======

- 1.加载bmp图到dst

.. code-block:: arduino

    static int load_bmp(Image *dst, fs::FS &fs, const char *name);

- 2.加载bmp图并返回Image对象

.. code-block:: arduino

    static Image * load_bmp(fs::FS &fs, const char *name);

参数
======

* ``dst`` 读取到的图像

* ``fs`` 文件系统，使用SD卡则为 `FFat`

* ``name`` bmp图像文件路径


返回值
========

- 语法1 
    0：成功，其他值：失败

- 语法2
    目标图像指针

示例说明
============

.. code-block:: arduino

    Image *img = Image::load_bmp(FFat, "/2.bmp");

save_bmp
^^^^^^^^^

描述
======

保存为bmp图

语法
======

- 1

.. code-block:: arduino

    static int save_bmp(Image *img, fs::FS &fs, const char *name);

- 2

.. code-block:: arduino

    int save_bmp(fs::FS &fs, const char *name);

参数
======

* ``img`` 要保存的图像

* ``fs`` 文件系统，使用SD卡则为 `FFat`

* ``name`` bmp图像文件路径



返回值
========

0：成功，其他值：失败


示例说明
============

.. code-block:: arduino

    if((rgb888 = img_display->to_rgb888()))
    {
        result = rgb888->save_bmp(FFat, "/img1.bmp");
        delete rgb888;
    }

save_jpeg
^^^^^^^^^

描述
======

保存为JPEG图像

语法
======

- 1

.. code-block:: arduino

    static int save_jpeg(Image *img, fs::FS &fs, const char *name, int quality = 80);

- 2

.. code-block:: arduino

    int save_jpeg(fs::FS &fs, const char *name, int quality = 80);

参数
======

* ``img`` 要保存的图像
* ``fs`` 文件系统，使用SD卡则为 `FFat`
* ``name`` JPEG图像文件路径
* ``quality`` JPEG质量（1-100，默认80）

返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    if(img_display)
    {
        result = img_display->save_jpeg(FFat, "/image.jpg", 90);
    }

compress_jpeg
^^^^^^^^^^^^^

描述
======

压缩图像为JPEG格式到缓冲区

语法
======

- 1

.. code-block:: arduino

    static int compress_jpeg(Image *img, uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality = 80);

- 2

.. code-block:: arduino

    int compress_jpeg(uint8_t *jpeg_buffer, size_t buffer_capacity, size_t *jpeg_size, int quality = 80);

参数
======

* ``img`` 要压缩的图像
* ``jpeg_buffer`` 存放JPEG数据的缓冲区
* ``buffer_capacity`` 缓冲区容量
* ``jpeg_size`` 返回实际JPEG数据大小
* ``quality`` JPEG质量（1-100，默认80）

返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    uint8_t jpeg_buffer[32768];
    size_t jpeg_size = 0;
    
    if(img_display)
    {
        result = img_display->compress_jpeg(jpeg_buffer, sizeof(jpeg_buffer), &jpeg_size, 85);
        if(result == 0)
        {
            // 使用压缩后的JPEG数据
        }
    }

例程 - color_convert_test.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**颜色格式转换**

以下是一个图像格式转换处理示例程序。它使用OV2640摄像头获取图像数据，并通过ST7789V液晶屏将不同格式的图像数据（RGB565、RGB888和RGBP888）显示出来。

程序中使用了OV2640库、ST7789V库、FFat库和K210图像处理库来实现图像采集和显示，通过调用Image类的方法（to_grayscale，to_rgb565，to_rgb888，to_rgbp888）实现不同格式间的转换。

需要注意的点包括：需要正确配置OV2640摄像头和ST7789V液晶屏的参数、需要正确挂载SD卡、需要正确获取图像缓冲区并创建Image对象、需要正确调用Image类的方法进行图像格式转换，并在合适的时候释放内存以避免内存泄漏。

.. literalinclude:: ../../../libraries/Image/examples/color_convert_test/color_convert_test.ino
    :language: arduino


例程 - load_and_display.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**加载bmp图并显示**

以下是一个BMP格式图片显示示例程序。它使用FFat库从SD卡中读取指定的BMP格式图片，并通过ST7789V液晶屏将其显示出来。

程序中使用了ST7789V库、FS库和FFat库来实现图像读取和显示，通过调用Image类的方法实现将BMP格式图片转换为RGB565格式并在液晶屏上显示。

需要注意的点包括：需要正确挂载SD卡、需要正确获取图像文件并创建Image对象、需要正确调用Image类的方法进行格式转换，并在合适的时候释放内存以避免内存泄漏。\
同时需要确保所读取的BMP格式图片分辨率不超过液晶屏分辨率，否则可能会导致图像显示异常。

.. literalinclude:: ../../../libraries/Image/examples/load_and_display/load_and_display.ino
    :language: arduino


例程 - save_camera_image.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**获取Camera图像并保存为bmp文件**

以下是一个摄像头拍照并保存为bmp的应用示例，可以将拍摄的画面显示在ST7789V液晶屏幕上，并支持通过按键保存拍摄的照片到SD卡上。

程序中使用了多个外部库和组件，包括OV2640驱动、ST7789V液晶屏驱动、FS文件系统和FFat库。在setup函数中进行了初始化操作，包括初始化串口、LCD屏幕、文件系统以及相机驱动等。在loop函数中实现了拍照、保存照片的功能，通过按键控制保存行为。

需要注意的点有：

1. 确保外部组件连接正确并且工作正常，例如摄像头模块、LCD屏幕、SD卡等。

2. 对于每个外部组件，需要按照其提供的API或者手册进行正确的初始化操作，否则可能会导致程序无法正常工作。

3. 在保存照片时，需要确保SD卡已经正确挂载，并且具有写入权限。

.. literalinclude:: ../../../libraries/Image/examples/save_camera_image/save_camera_image.ino
    :language: arduino

例程 - save_camera_jpeg.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**获取Camera图像并保存为JPEG文件**

以下是一个摄像头拍照并保存为JPEG的应用示例，可以将拍摄的画面显示在ST7789V液晶屏幕上，并支持通过按键保存拍摄的照片到SD卡上。相比BMP格式，JPEG提供更好的压缩率。

程序中使用了多个外部库和组件，包括OV2640驱动、ST7789V液晶屏驱动、FS文件系统和FFat库。在setup函数中进行了初始化操作，包括初始化串口、LCD屏幕、文件系统以及相机驱动等。在loop函数中实现了拍照、保存照片的功能，通过按键控制保存行为。

需要注意的点有：

1. 确保外部组件连接正确并且工作正常，例如摄像头模块、LCD屏幕、SD卡等。

2. JPEG压缩需要额外的内存，确保系统有足够的堆空间。

3. 质量参数影响文件大小和图像质量，建议在80-95之间选择。

.. literalinclude:: ../../../libraries/Image/examples/save_camera_jpeg/save_camera_jpeg.ino
    :language: arduino

API参考总结
^^^^^^^^^^^^^^^^

下表总结了Image类的主要方法：

+----------------------+------------------------------------+-------------------------+
| 方法分类            | 方法名称                          | 描述                   |
+======================+====================================+=========================+
| 构造函数            | Image()                           | 默认构造函数           |
|                     | Image(w,h,fmt,create)             | 带参数的构造函数       |
|                     | Image(w,h,fmt,buffer)             | 使用用户缓冲区的构造   |
+----------------------+------------------------------------+-------------------------+
| 属性获取            | width(), height()                 | 获取宽高               |
|                     | bpp(), format()                   | 获取像素格式           |
|                     | data(), pixel()                   | 获取数据指针           |
|                     | size()                            | 获取图像大小           |
+----------------------+------------------------------------+-------------------------+
| 裁剪操作            | cut()                             | 简单裁剪               |
|                     | cut_to_new_format()               | 裁剪并转换格式         |
|                     | cut_to_grayscale()                | 裁剪为灰度             |
|                     | cut_to_rgb565()                   | 裁剪为RGB565           |
|                     | cut_to_rgb888()                   | 裁剪为RGB888           |
|                     | cut_to_rgbp888()                  | 裁剪为RGBP888          |
+----------------------+------------------------------------+-------------------------+
| 缩放操作            | resize()                          | 图像缩放               |
+----------------------+------------------------------------+-------------------------+
| 格式转换            | to_grayscale()                    | 转换为灰度             |
|                     | to_rgb565()                       | 转换为RGB565           |
|                     | to_rgb888()                       | 转换为RGB888           |
|                     | to_rgbp888()                      | 转换为RGBP888          |
+----------------------+------------------------------------+-------------------------+
| 文件操作            | load_bmp()                        | 加载BMP图像            |
|                     | save_bmp()                        | 保存为BMP              |
|                     | save_jpeg()                       | 保存为JPEG             |
|                     | compress_jpeg()                   | 压缩为JPEG到缓冲区     |
+----------------------+------------------------------------+-------------------------+

使用建议
^^^^^^^^^

1. **内存管理**：使用 `new` 创建的 `Image` 对象需要手动 `delete` 释放内存。使用用户缓冲区的Image对象不会自动释放缓冲区。

2. **格式转换效率**：裁剪并转换格式的复合操作（如 `cut_to_rgb565()`）通常比先裁剪再转换更高效。

3. **JPEG压缩**：JPEG压缩会消耗更多CPU资源，建议在需要节省存储空间或网络传输时使用。

4. **缓冲区对齐**：内部分配的图像缓冲区是8字节对齐的，有利于DMA操作和性能优化。

5. **错误处理**：所有方法都返回错误码，建议检查返回值确保操作成功。

6. **格式限制**：JPEG压缩目前主要支持RGB888格式，其他格式可能需要先转换。

常见问题
^^^^^^^^^

Q: 为什么有些图像操作返回NULL？
A: 当内存分配失败或参数错误时，返回NULL表示操作失败。

Q: JPEG保存的质量参数如何选择？
A: 质量参数1-100，值越高图像质量越好但文件越大。通常80-90之间提供良好的质量/大小平衡。

Q: 如何处理不同格式的图像？
A: 使用 `format()` 方法检查图像格式，然后使用相应的转换方法。

Q: 如何高效处理大图像？
A: 使用裁剪操作处理感兴趣区域，避免全图处理。使用用户提供的缓冲区减少内存分配。

Q: RGBP888格式有什么特殊用途？
A: RGBP888是KPU（神经网络处理器）要求的输入格式，三个通道分别存储在不同的平面中。
