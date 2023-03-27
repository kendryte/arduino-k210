######
Image
######

`Image` 库用于处理k210的图像，为Camera和KPU提供图像类以及操作方法,包括图像裁剪缩放、rgb像素格式转换、bmp读取保存等功能。

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
        IMAGE_FORMAT_R8G8B8,        // bpp 3
        IMAGE_FORMAT_INVAILD = 4,
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

* ``width`` 裁剪图像宽

* ``height`` 裁剪图像高

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

    img_128x128 = new Image(128, 128, IMAGE_FORMAT_R8G8B8, true);
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

to_r8g8b8
^^^^^^^^^^^^^^

描述
======

转换为 `r8g8b8` 格式，作为KPU输入图需要的格式

语法
======

- 1

.. code-block:: arduino

    int to_r8g8b8(Image *dst, bool create = true);

- 2

.. code-block:: arduino

    Image * to_r8g8b8(void);

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

    Image *img_r8g8b8;
    img_r8g8b8 = img_gray->to_r8g8b8();

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

例程 - color_convert_test.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**颜色格式转换**

以下是一个图像格式转换处理示例程序。它使用OV2640摄像头获取图像数据，并通过ST7789V液晶屏将不同格式的图像数据（RGB565、RGB888和R8G8B8）显示出来。

程序中使用了OV2640库、ST7789V库、FFat库和K210图像处理库来实现图像采集和显示，通过调用Image类的方法（to_grayscale，to_rgb565，to_rgb888，to_r8g8b8）实现不同格式间的转换。

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
