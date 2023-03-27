########
Camera
########

Camera
########

K210 拥有一个硬件DVP接口,我们可以用来驱动摄像头模组


构造函数
^^^^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    Camera(int i2cNum, uint16_t slvAddr);

参数
======

* ``i2cNum`` 配置摄像头使用的 `i2c` 设备, 默认使用 `i2c2`

* ``slvAddr`` 摄像头 `i2c` 从机地址


返回值
========

无

示例说明
============

.. code-block:: arduino

    OV2640::OV2640(int8_t sda, int8_t scl, int i2cNum)
    :Camera(i2cNum, uint16_t(0x30))
    ,_sdaPin(sda)
    ,_sclPin(scl){}

begin
^^^^^^^

描述
======

初始化 `DVP` 

语法
======

- 1

.. code-block:: arduino

    int begin(int width, int height, camera_buffers_t *buff);

- 2

.. code-block:: arduino

    int begin(camera_pins_t &pins, uint32_t xclkFreqMhz, int width, int height, camera_buffers_t *buff);

参数
======

* ``pins`` `DVP` 引脚配置

* ``xclkFreqMhz`` `XCLK` 输出频率,单位 `MHz`

* ``width`` 图像宽度, 应与摄像头寄存器配置的输出一致

* ``height`` 图像高度, 应与摄像头寄存器配置的输出一致

* ``buff`` 用户可通过指定摄像头输出缓存地址,如果不指定,则自动 `malloc`


返回值
========

0：成功，其他值：失败

示例说明
============

待补充...

end
^^^^^

描述
======

关闭 `DVP`

语法
======

.. code-block:: arduino

    void end();


返回值
========

无

示例说明
============

待补充...

get_buffers
^^^^^^^^^^^^^

描述
======

获取摄像头输出地址信息

语法
======

.. code-block:: arduino

    void get_buffers(camera_buffers_t *buff);

参数
======

* ``buff``

    - ``disply`` `RGB565` 格式输出地址, 可作为 `LCD` 显示使用的画布
    
    - ``ai`` `AI` 输出 buffer



返回值
========

无

示例说明
============

.. code-block:: arduino

    camera_buffers_t buff;
    cam.get_buffers(&buff);

read_reg
^^^^^^^^^

描述
======

读摄像头寄存器, 用户可根据不同摄像头需求进行重写

语法
======

- 1

.. code-block:: arduino

    virtual int         read_reg            (uint8_t reg_addr, uint8_t *reg_data);

- 2

.. code-block:: arduino

    virtual int         read_reg            (uint16_t reg_addr, uint16_t *reg_data);

- 3

.. code-block:: arduino

    virtual int         read_reg            (int reg_addr, int *reg_data);

参数
======

* ``reg_addr`` 寄存器地址

* ``reg_data`` 读到的寄存器地址


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    int t;
    read_reg(0x0A, &t);


write_reg
^^^^^^^^^^^

描述
======

写摄像头寄存器, 用户可根据不同摄像头需求进行重写

语法
======

- 1

.. code-block:: arduino

    virtual int         write_reg           (uint8_t reg_addr, uint8_t reg_data);

- 2

.. code-block:: arduino

    virtual int         write_reg           (uint16_t reg_addr, uint16_t reg_data);

- 3

.. code-block:: arduino

    virtual int         write_reg           (int reg_addr, int reg_data);

参数
======

* ``reg_addr`` 寄存器地址

* ``reg_data`` 需要写入寄存器的数据


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    write_reg(0xe0, 0x04);

snapshot
^^^^^^^^^

描述
======

获取一帧图像

语法
======

.. code-block:: arduino

    virtual int         snapshot            (uint32_t timeout_ms = 200);

参数
======

* ``timeout_ms`` 读取超时时间,单位毫秒


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    cam.snapshot()

width
^^^^^^^

描述
======

获取当前图像输出的宽度

语法
======

.. code-block:: arduino

    virtual int         width               (void);

返回值
========

当前图像输出的宽度

示例说明
============

.. code-block:: arduino

    cam.width();

height
^^^^^^^

描述
======

获取当前图像输出的高度

语法
======

.. code-block:: arduino

    virtual int         height               (void);

返回值
========

当前图像输出的高度

示例说明
============

.. code-block:: arduino

    cam.height();

flip
^^^^^^^

描述
======

获取当前图像是否翻转


语法
======

.. code-block:: arduino

    virtual bool         flip               (void);


返回值
========

当前图像是否翻转

示例说明
============

.. code-block:: arduino

    cam.flip();

mirror
^^^^^^^

描述
======

获取当前图像是否镜像

语法
======

.. code-block:: arduino

    virtual bool         mirror               (void);

返回值
========

当前图像是否镜像

示例说明
============

.. code-block:: arduino

    cam.mirror();

reset
^^^^^^^^

描述
======

初始化摄像头

语法
======

.. code-block:: arduino

    virtual int         reset               (framesize_t framesize, camera_buffers_t *buff) = 0;

参数
======

* ``framesize`` 输出图像分辨率

* ``buff`` `DVP` 输出图像缓存信息


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    cam.reset(FRAMESIZE_QVGA)

read_id
^^^^^^^^

描述
======

读取摄像头 `ID`

语法
======

.. code-block:: arduino

    virtual int         read_id             (void) = 0;



返回值
========

摄像头 `ID`

示例说明
============

待补充...

set_hmirror
^^^^^^^^^^^^^^

描述
======

设置输出图像镜像

语法
======

.. code-block:: arduino

    virtual int         set_hmirror         (int enable) = 0;

参数
======

* ``enable`` 

    - 0 不镜像

    - 1 镜像


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    cam.set_hmirror(true);

set_vflip
^^^^^^^^^^^^^

描述
======

设置输出图像翻转

语法
======

.. code-block:: arduino

    virtual int         set_vflip           (int enable) = 0;

参数
======

* ``enable`` 

    - 0 不翻转

    - 1 翻转


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    cam.set_vflip(true);


------------------


GC0328
#######

构造函数
^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    GC0328(int8_t sda = CAMERA_SDA_PIN, int8_t scl = CAMERA_SCL_PIN, int i2cNum = 2);

参数
======

* ``sda`` 摄像头 `sda` 引脚

* ``scl`` 摄像头 `scl` 引脚

* ``i2cNum`` 配置摄像头使用的 `i2c` 设备, 默认 `i2c2`


返回值
========

无

示例说明
============

.. code-block:: arduino

    GC0328 cam;

----------------------


OV2640
#######

构造函数
^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    OV2640(int8_t sda = CAMERA_SDA_PIN, int8_t scl = CAMERA_SCL_PIN, int i2cNum = 2);

参数
======

* ``sda`` 摄像头 `sda` 引脚

* ``scl`` 摄像头 `scl` 引脚

* ``i2cNum`` 配置摄像头使用的 `i2c` 设备, 默认 `i2c2`



返回值
========

无

示例说明
============

.. code-block:: arduino

    OV2640 cam;


例程 - screen_display.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**摄像头读取图像并显示在 `LCD`**

以下程序是一个使用摄像头拍摄并送到LCD显示示例。它通过OV2640摄像头拍摄照片，将照片显示在240x320的ST7789V液晶显示屏上。

在setup函数中，首先初始化了摄像头，并获取了摄像头缓冲区。接着创建了两个图像对象，分别用于AI处理和显示。然后初始化了LCD，并设置了LCD旋转方向、字体大小和帧缓冲区。

在loop函数中，调用cam.snapshot函数拍摄一张照片，将文字“Test1234”显示在LCD上，并刷新LCD显示。

需要注意的是，摄像头的初始化有可能会失败，需要加入相应的错误处理。同时，在使用LCD时，需要设置LCD帧缓冲区以及调用lcd.refresh函数来更新显示。

.. literalinclude:: ../../../libraries/OV2640/examples/screen_display/screen_display.ino
    :language: arduino
