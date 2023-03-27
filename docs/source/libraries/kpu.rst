######
KPU
######

KPU
#########

K210具备神经网络加速器 `KPU` ，它可以在低功耗的情况下实现卷积神经网络计算，时时获取被检测目标的大小、坐标和种类，对人脸或者物体进行检测和分类。

KPU 具备以下几个特点：

    * 支持主流训练框架按照特定限制规则训练出来的定点化模型

    * 对网络层数无直接限制，支持每层卷积神经网络参数单独配置，包括输入输出通道数目、输入输 出行宽列高

    * 支持两种卷积内核 1x1 和 3x3

    * 支持任意形式的激活函数

    * 实时工作时最大支持神经网络参数大小为 5.5MiB 到 5.9MiB
    
    * 非实时工作时最大支持网络参数大小为（Flash 容量-软件体积）


load_kmodel
^^^^^^^^^^^^^

描述
======

加载kmodel模型文件，从文件系统或者Flash加载模型

语法
======

* 从内存buffer加载 

.. code-block:: arduino

    int load_kmodel(uint8_t *buffer, size_t size, const char *name = "default");

* 从SD卡文件系统加载 

.. code-block:: arduino

    int load_kmodel(fs::FS &fs, const char *name);

* 从flash加载 

.. code-block:: arduino

    int load_kmodel(uint32_t offset);

参数
========

* ``buffer`` 模型内存buffer, 
* ``size`` 模型大小 
* ``fs`` 文件系统，使用SD卡则为 `FFat`
* ``name`` 模型文件路径
* ``offset`` flash中模型存放地址

返回值
========

0：成功，-1：失败

示例说明
============

从SD卡加载模型

.. code-block:: arduino

    KPU_Base landmark;
    landmark.load_kmodel(FFat, "/KPU/face_detect_with_68landmark/landmark68.kmodel");

从flash加载模型

.. code-block:: arduino

    KPU_Base kpu1;
    kpu1.load_kmodel(0x300000);


run_kmodel
^^^^^^^^^^^^^

描述
======

加载模型之后，对输入图像进行推理计算

语法
======

* 使用r8g8b8内存中的图运行

.. code-block:: arduino

    int run_kmodel(uint8_t *r8g8b8, int w, int h, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);

* 使用Image图运行

.. code-block:: arduino

    int run_kmodel(Image *img, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);

参数
========

* ``r8g8b8`` r8g8b8格式源图像内存

* ``w`` 图像宽

* ``h`` 图像高

* ``dam_ch`` DMA通道

* ``img`` Image图像类输入

返回值
========

0：成功，-1：失败

示例说明
============

.. code-block:: arduino

    img_128x128 = new Image(128, 128, IMAGE_FORMAT_R8G8B8, true);
    landmark.run_kmodel(img_128x128)


get_result
^^^^^^^^^^^^^^

描述
======

获取模型运行结果

语法
======

.. code-block:: arduino

    int get_result(uint8_t **data, size_t *count, uint32_t startIndex = 0);

参数
========

* ``src`` 源图像

* ``dst`` 生成的图像

* ``create`` 是否为生成的图像创建内存

返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    float *result;
    size_t output_size;
    landmark.get_result((uint8_t **)&result, &output_size)


end
^^^^^^^^^^^^^^

描述
======

去初始化kpu，释放模型内存

语法
======

.. code-block:: arduino

    void end();

参数
========

无

返回值
========

无

示例说明
============

.. code-block:: arduino

    landmark.end();

---------------

KPU_Activation
#####################

该类包含以下激活函数，用于处理模型运行推理后得到的数据。

sigmoid
^^^^^^^^^^^^^^

描述
======

将数据归一化到[0, 1]范围

语法
======

.. code-block:: arduino

    static float sigmoid(float x);

参数
========

* ``x`` 输入浮点数

返回值
========

处理后的浮点数

示例说明
============

.. code-block:: arduino

    float th = KPU_Activation::sigmoid(10.4);


softmax
^^^^^^^^^^^^^^

描述
======

将数据归一化到[0, 1]范围

语法
======

.. code-block:: arduino

    static void softmax(float *x, float *dx, uint32_t len);

参数
========

* ``x`` 输入浮点数组

* ``dx`` 输出浮点数组

* ``len`` 数组长度

返回值
========

无

示例说明
============

.. code-block:: arduino

    float fea[192]={0};
    float sm_fea[192];
    static void softmax(fea, sm_fea, 192);

---------------

KPU_Yolo2
##################

`KPU_Yolo2` 继承自 `KPU` ，用于yolo模型的使用。

begin
^^^^^^^^^^^^^^

描述
======

初始化yolo

语法
======

.. code-block:: arduino

    int begin(float *anchor, int anchor_num, float threshold = 0.5, float nms_value = 0.3);

参数
========

* ``anchor`` 锚点参数与模型参数一致，同一个模型这个参数是固定的，和模型绑定的（训练模型时即确定了）， 不能改成其它值。

* ``anchor_num`` 锚点参数数组长度。

* ``threshold`` 概率阈值， 只有是这个物体的概率大于这个值才会输出结果， 取值范围：[0, 1]，默认值为0.5

* ``nms_value`` box_iou 门限, 为了防止同一个物体被框出多个框，当在同一个物体上框出了两个框，这两个框的交叉区域占两个框总占用面积的比例 如果小于这个值时， 就取其中概率最大的一个框，默认值为0.3

返回值
========

0：成功，-1：失败

示例说明
============

.. code-block:: arduino

    KPU_Yolo2 yolo2;
    yolo2.begin(anchor, sizeof(anchor) / sizeof(float));


end
^^^^^^^^^^^^^^

描述
======

去初始化yolo，释放内存

语法
======

.. code-block:: arduino

    void end();

参数
========

无

返回值
========

无

示例说明
============

.. code-block:: arduino

    KPU_Yolo2 yolo2;
    yolo2.end();


run
^^^^^^^^^^^^^

描述
======

加载模型之后，对输入图像进行推理计算

语法
======

* 使用r8g8b8内存中的图运行
    ``int run(uint8_t *r8g8b8, int w, int h, obj_info_t *info, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);``

* 使用Image图运行
    ``int run(Image *img, obj_info_t *info, dmac_channel_number_t dam_ch = DMAC_CHANNEL_MAX);``

参数
========

* ``r8g8b8`` r8g8b8格式源图像内存

* ``w`` 图像宽

* ``h`` 图像高

* ``info`` 检测矩形框结果输出

.. code-block:: arduino

    #define REGION_LAYER_MAX_OBJ_NUM (10)
    typedef struct
    {
        uint32_t obj_number;

        struct
        {
            uint32_t x;
            uint32_t y;
            uint32_t w;
            uint32_t h;
            uint32_t class_id;
            float prob;
        } obj[REGION_LAYER_MAX_OBJ_NUM];
    } obj_info_t;

* ``dam_ch`` DMA通道

* ``img`` Image图像类输入

返回值
========

0：成功，-1：失败

示例说明
============

.. code-block:: arduino

    obj_info_t info;
    yolo2.run(img_ai, &info, DMAC_CHANNEL5);

---------------

KPU_Face
##################

`KPU_Face` 继承自 `KPU` ，增加了人脸识别要用到的相关方法。

使用该模块需要先引用以下头文件：

.. code-block:: arduino

    #include "KPU_Face.h"

calc_feature
^^^^^^^^^^^^^^

描述
======

计算人脸特征值

语法
======

.. code-block:: arduino

    int calc_feature(float *feature);

参数
========

* ``feature`` 传入一个数组指针，获取计算的特征值结果。

返回值
========

0：成功，-1：失败

示例说明
============

.. code-block:: arduino

    KPU_Face face;
    float feature[192];
    face.calc_feature(feature);


compare_feature
^^^^^^^^^^^^^^^^^^^

描述
======

比对两个人脸特征值，计算相似分数。

语法
======

.. code-block:: arduino

    static float compare_feature(float *f1, float *f2, size_t length);

参数
========

* ``f1`` 参与比对的特征值数组。

* ``f2`` 参与比对的特征值数组。

* ``length`` 特征值数组长度。

返回值
========

返回相似分数[0-100]，越接近100则说明相似度越高。

示例说明
============

.. code-block:: arduino

    float tmp_score = face.compare_feature(feature, persion_info_save[p].feature, FEATURE_LEN);


calc_affine_transform
^^^^^^^^^^^^^^^^^^^^^^^^

描述
======

根据两组人脸关键点算出变换矩阵。

语法
======

.. code-block:: arduino

    static int calc_affine_transform(uint16_t *src, uint16_t *dst, uint16_t cnt, float* TT);

参数
========

* ``src`` 需要修正的人脸关键点。

* ``dst`` 对齐的目标人脸关键点。

* ``TT`` 3*3二维数组 `TT[3][3]`。

返回值
========

0：成功，-1：失败

示例说明
============

.. code-block:: arduino

    float T[3][3];
    face.calc_affine_transform((uint16_t*)face_key_point, (uint16_t*)dst_point, 5, (float*)T);


apply_affine_transform
^^^^^^^^^^^^^^^^^^^^^^^^

描述
======

相似变换修正人脸图。

语法
======

.. code-block:: arduino

    static int apply_affine_transform(Image * src, Image* dst, float* TT);

参数
========

* ``src`` 需要修正的人脸图像。

* ``dst`` 修正后的人脸图像。

* ``TT`` 根据 `calc_affine_transform` 计算得到的3*3二维数组 `TT[3][3]`。

返回值
========

0：成功，其他值：失败

示例说明
============

.. code-block:: arduino

    float T[3][3];
    face.calc_affine_transform((uint16_t*)face_key_point, (uint16_t*)dst_point, 5, (float*)T);
    face.apply_affine_transform(img_ai, img_128x128, (float*)T);    

---------------

例程 - kpu_face_attribute.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**人脸属性检测**

以下是一个基于K210芯片的人脸检测和属性识别应用。

它通过摄像头拍摄照片，运行YOLOv2模型进行人脸检测，并在检测到人脸的图像上裁剪缩放得到人脸抠图送给人脸关键点检测模型，通过得到的关键点进行仿射变换得到矫正图像送给人脸属性检测模型，最终在LCD上显示出检测结果。

该程序中包含了许多注意点，例如需要初始化LCD、文件系统、摄像头等设备；需要对图像进行裁剪、缩放和仿射变换等操作；还需要获取模型运行结果并进行解析和处理。\
此外，为了提高检测的准确性，程序还对检测框进行了扩展，并使用仿射变换矫正了人脸属性模型需要的输入图像。

.. literalinclude:: ../../../libraries/K210_KPU/examples/kpu_face_attribute/kpu_face_attribute.ino
    :language: arduino

