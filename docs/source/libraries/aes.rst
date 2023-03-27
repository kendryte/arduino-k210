######
AES
######

`K210` 有一个硬件 `AES` 加密模块, 支持 ``ECB``、 ``CBC``、 ``GCM`` 模式


begin
^^^^^^^^

描述
======

初始化

语法
======

- 1

.. code-block:: arduino

    static int begin(aes_cipher_mode_t mode, aes_kmode_t keyLen);

- 2

.. code-block:: arduino

    static int begin(aes_cipher_mode_t mode, aes_kmode_t keyLen, aes_iv_len_t ivLen);

参数
======

* ``mode``

    - AES_ECB ECB模式

    - AES_CBC CBC模式

    - AES_GCM GCM模式

* ``keyLen``

    - AES_128 Key 长度128Bit

    - AES_192 Key 长度192Bit

    - AES_256 Key 长度256Bit

* ``ivLen``
    
    - CBC模式时, 需要设置为 `IV_LEN_128`

    - GCM模式时, 需要设置为 `IV_LEN_96`


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    AES::begin(AES_ECB, AES_128);

end
^^^^^

描述
======

关闭 `AES` 模块

语法
======

.. code-block:: arduino

    static void end();


返回值
========

无

示例说明
============

.. code-block:: arduino

    AES::end();


setDataLen
^^^^^^^^^^^^

描述
======

设置需要处理的数据总长度

语法
======

.. code-block:: arduino

    static int setDataLen(size_t dataLen);

参数
======

* ``dataLen`` 数据长度


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    AES::setDataLen(16);

setKey
^^^^^^^^

描述
======

设置加解密使用的密钥

语法
======

.. code-block:: arduino

    static int setKey(const uint8_t *key, uint8_t len);

参数
======

* ``key`` 密钥

* ``len`` 密钥长度,需要与 ``begin`` 中设置的一致


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f};
    AES::setKey(key, 16);


setIV
^^^^^^^

描述
======

设置加解密使用的初始化向量(IV)

语法
======

.. code-block:: arduino

    static int setIV(const uint8_t *iv, uint8_t len);


* ``iv`` 初始化向量

* ``len`` 初始化向量长度,需要与 ``begin`` 中设置的一致



返回值
========

0：成功，其他值：失败

示例说明
============

待补充...

encrypt
^^^^^^^^^^

描述
======

加密数据

语法
======

.. code-block:: arduino

    static int encrypt(uint8_t *input, uint8_t *output, size_t len);

参数
======

* ``input`` 输入数据(未加密)

* ``output`` 输出数据(加密后)

* ``len`` 数据长度


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    uint8_t data[16] = "1234567890";
    uint8_t output[16];
    AES::encrypt(data, output, 16);

decrypt
^^^^^^^^^

描述
======

解密数据

语法
======

.. code-block:: arduino

    static int decrypt(uint8_t *input, uint8_t *output, size_t len);

参数
======

* ``input`` 输入数据(加密前)

* ``output`` 输出数据(解密后)

* ``len`` 数据长度


返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    AES::decrypt(output, data, 16);

gcmSetAAD
^^^^^^^^^^^

描述
======

`GCM` 模式下使用，获取附加消息

语法
======

.. code-block:: arduino

    static int gcmSetAAD(uint8_t *aad, uint8_t len);

参数
======

* ``aad`` 附加消息

* ``len`` 附加消息长度



返回值
========

0：成功，其他值：失败

示例说明
============

待补充...

gcmGetTag
^^^^^^^^^^

描述
======

`GCM` 模式下使用，获取附加消息产生的 `TAG`

语法
======

.. code-block:: arduino

    static int gcmGetTag(uint8_t *gcmTag);

参数
======

* ``gcmTag`` 附加消息产生的 `TAG`


返回值
========

0：成功，其他值：失败

示例说明
============

待补充...

例程 - aes_ecb_128.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**`AES_128` 加解密测试** 

以下程序是一个AES加解密示例。它使用ECB模式和128位密钥对16字节的数据进行加密和解密，输出加密后和解密后的结果。

在setup函数中，首先定义了一个16字节的密钥和16字节的明文数据，然后初始化AES实例，设置加密模式和密钥，接着调用AES::encrypt函数对数据进行加密，将结果输出，并清空明文数据。接下来，调用AES::decrypt函数对加密后的数据进行解密，将结果输出。

需要注意的是，程序中要使用K210的AES库，因此需要包含相应的头文件和使用命名空间K210。还需要确保明文数据长度为16字节，密钥长度为128位。同时，在使用完AES实例后，需要调用AES::end函数释放资源。

.. literalinclude:: ../../../libraries/K210_AES/examples/aes_ecb_128_test/aes_ecb_128_test.ino
    :language: arduino

