######
I2S
######

I2S模块主要用于驱动I2S设备，k210一共有3个I2S设备，每个设备一共有4个通道，不支持从模式。

I²S是一种电气串行总线接口标准，用于将数字音频设备连接在一起。它用于在电子设备中的集成电路之间传输PCM（脉冲编码调制）音频数据。
I²S总线将时钟和串行数据信号分开，因此接收器比需要从数据流中恢复时钟的异步通信系统所需的接收器更简单。
尽管名称相似，但I²S与双向I²C（IIC）总线无关且不兼容。
I²S总线至少由三条线路组成：

* **位时钟线**
  
  * 正式名称为“连续串行时钟（SCK）”。通常写成“位时钟（BCLK）”。
  *  在此库中函数参数 ``sckPin``。

* **字时钟线**
  
  * 正式名称为“字选择（WS）”。通常称为“左右时钟（LRCLK）”或“帧同步（FS）”。
  * 0 = 左通道，1 = 右通道
  * 在此库中函数参数 ``fsPin``。

* **数据线**

  * 正式名称为“串行数据（SD）”，但可以称为SDATA，SDIN，SDOUT，DACDAT，ADCDAT等。
  * Arduino I2S在单个数据引脚上切换输入和输出。
  * 在此库中函数参数 ``sdPin``。


构造函数
^^^^^^^^^^^^

描述
======

构造函数

语法
======

.. code-block:: arduino

    I2SClass(uint8_t deviceIndex, uint8_t clockGenerator, uint8_t sdPin, uint8_t sckPin, uint8_t fsPin);

参数
======

* ``deviceIndex`` I2S设备号

* ``clockGenerator`` 时钟

* ``sdPin`` 数据引脚

* ``sckPin`` 位时钟引脚

* ``fsPin`` 左右时钟引脚



返回值
========

无

示例说明
============

.. code-block:: arduino

    I2SClass I2S(0, 0, 18, 35, 33);

begin
^^^^^^^^^^^^^

描述
======

初始化i2s模式，配置参数。

语法
======

- 1.the SCK and FS pins are driven as outputs using the sample rate

.. code-block:: arduino

  int begin(int mode, long sampleRate, int bitsPerSample);

- 2.the SCK and FS pins are inputs, other side controls sample rate

.. code-block:: arduino

  int begin(int mode, int bitsPerSample);

参数
======

* ``mode`` 模式：飞利浦标准，左对齐，右对齐。

* ``sampleRate`` 采样率 

* ``bitsPerSample`` 位深度


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    I2S.begin(I2S_RIGHT_JUSTIFIED_MODE, 16000, 16);

end
^^^^^^^^^^^^^^

描述
======

去初始化释放I2S资源

语法
======

.. code-block:: arduino

    void end();


返回值
========

无

示例说明
============

.. code-block:: arduino

    I2S.end();

onTransmit
^^^^^^^^^^^^^^

描述
======

设置发送模式状态回调函数

语法
======

.. code-block:: arduino

    void onTransmit(callback_with_arg_t cb, void *arg);

参数
======

* ``cb`` 回调函数 

* ``arg`` 参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    I2S.onTransmit(_i2s_transmit_done, &_txdone);

onReceive
^^^^^^^^^^^^^^

描述
======

设置接受模式状态回调函数

语法
======

.. code-block:: arduino

    void onReceive(callback_with_arg_t cb, void *arg);

参数
======

* ``cb`` 回调函数 

* ``arg`` 参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    I2S.onReceive(_i2s_receive_done, &_rxdone);

write
^^^^^^^^^^^^^^

描述
======

向I2S写入数据

语法
======

.. code-block:: arduino

    virtual size_t write(const uint8_t *buffer, size_t size);

参数
======

* ``buffer`` 发送缓存 

* ``size`` 大小


返回值
========

写入的大小

示例说明
============

.. code-block:: arduino

    I2S.write(buff + sent, remain);

read
^^^^^^^^^^^^^^

描述
======

从I2S读数据

语法
======

.. code-block:: arduino

    int read(void* buffer, size_t size);

参数
======

* ``buffer`` 读到缓存

* ``size`` 大小



返回值
========

读到的大小

示例说明
============

.. code-block:: arduino

    I2S.read(buff, buffSz);

setBufferSize
^^^^^^^^^^^^^^

描述
======

设置I2S接受缓冲大小

语法
======

.. code-block:: arduino

    void setBufferSize(int bufferSize);

参数
======

* ``bufferSize`` 缓冲大小


返回值
========

无

示例说明
============

待补充...

getBufferSize
^^^^^^^^^^^^^^

描述
======

获取I2S接受缓冲大小

语法
======

.. code-block:: arduino

    int getBufferSize(void);


返回值
========

缓存大小

示例说明
============

.. code-block:: arduino

    int buffSz = I2S.getBufferSize();


例程 - i2s_play.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**音频播放**

以下是一个音频数据播放示例程序。它通过I2S接口将一段音频数据从缓冲区传输到外部音频设备。

程序中使用了I2S库和FFat库来实现音频数据的读取和传输，同时还调用了RT-Thread的相关函数来等待数据传输完成。

注意点包括：需要正确配置I2S接口的参数、处理音频数据的格式与采样率、正确使用rt_malloc和rt_free函数进行内存分配和释放、等待音频数据传输完成时需要使用rt_completion_wait函数等待回调函数的通知。

.. literalinclude:: ../../../libraries/I2S/examples/play/play.ino
    :language: arduino

例程 - i2s_record.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**录音**

以下是一个接收音频数据并保存文件实现录音的示例程序。它通过I2S接口从外部音频设备接收音频数据，并将其写入到SD卡中的文件中。

程序中使用了I2S库、FFat库（SD卡库）来实现音频数据的读取和存储，同时还调用了RT-Thread的相关函数来等待数据接收完成。

注意点包括：需要正确配置I2S接口的参数、处理音频数据的格式与采样率、正确使用rt_malloc_align和rt_free_align函数进行内存分配和释放、等待音频数据接收完成时需要使用rt_completion_wait函数等待回调函数的通知、需要正确挂载SD卡并打开指定的文件进行写操作。

.. literalinclude:: ../../../libraries/I2S/examples/record/record.ino
    :language: arduino
