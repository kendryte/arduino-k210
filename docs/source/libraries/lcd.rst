#########
LCD
#########


Bus8080
########

我们使用OSPI模拟8080来驱动MCU接口的屏幕

构造函数
^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    Bus8080(int8_t clk_pin = TFT_CLK_PIN,
            int8_t cs_pin = TFT_CSX_PIN,
            int8_t dc_pin = TFT_DCX_PIN,
            int8_t rst_pin = TFT_RST_PIN
            );

参数
======

* ``clk_pin`` 时钟输出引脚
* ``cs_pin`` 屏幕片选引脚
* ``dc_pin`` 数据或指令引脚
* ``rst_pin`` 复位引脚


返回值
========

无

示例说明
============

待补充...

begin
^^^^^^

描述
======

初始化

语法
======

.. code-block:: arduino

    void begin(uint32_t freq = 15000000);

参数
======

* ``freq`` 时钟输出频率, 默认 ``15M``


返回值
========

无

示例说明
============

.. code-block:: arduino

    K210::Bus8080::begin(freq);

reset
^^^^^^

描述
======

复位屏幕

语法
======

.. code-block:: arduino

    void reset(PinStatus valid, int rst_ms);

参数
======

* ``valid`` 屏幕正常工作时复位引脚电平

* ``rst_ms`` 复位屏幕时长, 单位为毫秒


返回值
========

无

示例说明
============

.. code-block:: arduino

    K210::Bus8080::reset(HIGH, 10);

writeCommand
^^^^^^^^^^^^^^

描述
======

写屏幕寄存器

语法
======

- 1

.. code-block:: arduino

    void writeCommand(uint8_t command);

- 2

.. code-block:: arduino

    void writeCommand(uint16_t command);


返回值
========

无

示例说明
============

.. code-block:: arduino

    K210::Bus8080::writeCommand((uint8_t)PIXEL_FORMAT_SET);

writeData
^^^^^^^^^^^^

描述
======

写数据到屏幕

语法
======

- 1

.. code-block:: arduino

    void writeData(uint8_t *data, uint32_t len);

- 2

.. code-block:: arduino

    void writeData(uint16_t *data, uint32_t len);

- 3

.. code-block:: arduino

    void writeData(uint32_t *data, uint32_t len);

参数
======

* ``data`` 数据指针地址

* ``len`` 数据长度


返回值
========

无

示例说明
============

.. code-block:: arduino

    K210::Bus8080::writeData((uint8_t)0x55);

fillScreen
^^^^^^^^^^^^

描述
======

使用指定颜色填充屏幕,可用来快速清屏

语法
======

.. code-block:: arduino

    void fillScreen(uint16_t color, int16_t w, int16_t h);

参数
======

* ``color`` `RGB565` 颜色的值

* ``w`` 屏幕宽度

* ``h`` 屏幕高度

返回值
========

无

示例说明
============

.. code-block:: arduino

    K210::Bus8080::fillScreen(color, _width, _height);


--------------

ST7789V
##########

基于 8080 总线, 我们可以驱动一款屏幕, 目前提供驱动 `ST7789V` 控制器的屏幕驱动示例, 用户也可重写部分函数,来驱动其他屏幕

`ST7789V` 是 `Adafruit_GFX`_ 的派生类,用户可阅读官方文档来使用对应的API。

Arduino的Adafruit_GFX库为我们所有的LCD和OLED显示屏提供了通用语法和图形功能集。这使得Arduino示例程序可以很容易地在不同类型的显示屏之间进行调整，并且任何新特性、性能改进和错误修复都将立即应用于我们提供的完整的彩色显示。

Adafruit_GFX库可以使用Arduino库管理器安装……这是首选的方式。在Arduino IDE“工具”菜单中，选择“管理库…”，在搜索栏中输入“gfx”可以快速找到它。

文档: Adafruit_GFX库文档链接_。

源码存储库: Adafruit_GFX源码_。

构造函数
^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    ST7789V(int16_t w, int16_t h, 
          int8_t clk_pin = TFT_CLK_PIN,
          int8_t cs_pin = TFT_CSX_PIN,
          int8_t dc_pin = TFT_DCX_PIN,
          int8_t rst_pin = TFT_RST_PIN);

参数
======

* ``w`` 屏幕宽度

* ``h`` 屏幕高度


返回值
========

无

示例说明
============

.. code-block:: arduino

    ST7789V lcd(240, 320);

begin
^^^^^^^

描述
======

初始化

语法
======

.. code-block:: arduino

    virtual void begin(uint32_t freq = 15 * 1000 * 1000);

参数
======

* ``frame_buffer`` 帧缓存, 在配合摄像头使用的时候,可以使用摄像头的``display_buffer``,可以节省内存空间

* ``freq`` 时钟输出频率, 默认 ``15M``


返回值
========

无

示例说明
============

.. code-block:: arduino

    lcd.begin();

setFrameBuffer
^^^^^^^^^^^^^^^^

描述
======

为lcd设置帧缓存

语法
======

- 1.从Image为lcd创建帧缓存

.. code-block:: arduino

    int setFrameBuffer(K210::Image *img);

- 2.从buffer为lcd创建帧缓存

.. code-block:: arduino

    int setFrameBuffer(uint16_t *buffer, int16_t w, int16_t h);

参数
======

* ``img`` 使用Image对象作为帧缓存

* ``buffer`` 帧缓存, 在配合摄像头使用的时候,可以使用摄像头的``display_buffer``,可以节省内存空间

* ``w`` 屏幕宽度

* ``h`` 屏幕高度


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    Image img(240, 320, IMAGE_FORMAT_RGB565, true);
    lcd.setFrameBuffer(&img);

refresh
^^^^^^^^^^^^^^^^

描述
======

将帧缓存主动刷新到lcd，一般放到loop循环体中实时刷新屏幕。

语法
======

.. code-block:: arduino

    virtual void refresh(void);


返回值
========

无

示例说明
============

.. code-block:: arduino

    void loop(void) {
        lcd.refresh();
    }

invertDisplay
^^^^^^^^^^^^^^^^

描述
======

设置lcd反显，以确保某些屏幕驱动显示颜色正常。

语法
======

.. code-block:: arduino

    virtual void invertDisplay(bool i);

参数
======

* ``i`` 反显模式


返回值
========

无

示例说明
============

.. code-block:: arduino

    lcd.invertDisplay(1);

drawImage
^^^^^^^^^^^^^^^^

描述
======

在lcd上画图

语法
======

.. code-block:: arduino

    virtual void drawImage(const K210::Image *img, int16_t x = 0, int16_t y = 0);

参数
======

* ``img`` 要画的img图像

* ``x`` 画图起始x坐标

* ``y`` 画图起始y坐标



返回值
========

无

示例说明
============

.. code-block:: arduino

    Image *rgb565 = img->to_rgb565();
    lcd.drawImage(rgb565);


例程 - test_rotation.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**测试屏幕旋转并显示字符串**

这份程序是一个基于ST7789V驱动的LCD屏幕显示程序，它可以在LCD屏幕上显示文本和图像。程序中使用了ST7789V和Image类，其中ST7789V类用于与LCD屏幕通信，Image类用于创建和管理图像数据。

程序的工作原理是在循环中不断更新LCD屏幕的显示内容。具体来说，程序通过调用ST7789V类的函数来设置屏幕的旋转角度、填充颜色、光标位置和刷新屏幕等操作。\
同时，程序还使用了Image类来创建一帧图像缓存，并通过setFrameBuffer()函数将其与ST7789V类关联，从而实现将图像数据显示在LCD屏幕上的功能。

需要注意的是，程序中使用的LCD屏幕需要与ST7789V驱动兼容，且需要正确设置其参数（如屏幕分辨率、旋转角度等）。在使用LCD屏幕时，还需注意电源和信号线的连接，避免出现短路和接触不良等问题。

.. literalinclude:: ../../../libraries/ST7789V/examples/test_rotation/test_rotation.ino
    :language: arduino


.. _Adafruit_GFX: https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.h
.. _Adafruit_GFX源码: https://github.com/adafruit/Adafruit-GFX-Library
.. _Adafruit_GFX库文档链接: https://learn.adafruit.com/adafruit-gfx-graphics-library
