########
快速上手
########

本篇将说明如何安装 Arduino-K210 支持，以及快速使用。

安装Arduino
##################

.. note::
    从Arduino IDE版本1.6.4开始，Arduino允许安装第三方平台 使用板管理器的包。我们有适用于Windows，macOS和Linux的软件包。

要使用开发板管理开始安装过程，请执行以下步骤：

#. Arduino IDE支持Windows，Linux， macOS三种操作系统，根据你的系统到 arduino.cc网站_ 下载对应安装包进行安装，建议使用1.8.X以上版本。

#. 启动Arduino并打开 ``文件`` > ``首选项`` 窗口。

    .. figure:: ../_static/install_guide_preferences.jpeg
        :align: center
        :width: 600
        :figclass: align-center


#. 在 ``附加开发板管理器网址:`` 字段中输入以下版本链接之一。您可以添加多个URL，并用逗号分隔它们。

    - Stable release link::

        https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_index.json

    - Development release link::

        https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_dev_index.json


#. 从 ``工具`` > ``开发板`` 菜单中打开 ``开发板管理器`` 并安装 *k210* 平台。

    .. figure:: ../_static/install_k210_pkg.png
        :align: center
        :width: 600
        :figclass: align-center


#. 安装后不要忘记从 ``工具`` > ``开发板`` 菜单中选择您的 *k210* 开发板。

    .. figure:: ../_static/select_board.png
        :align: center
        :width: 600
        :figclass: align-center


到此Arduino-K210开发环境便已经安装完成，下面就可以尝试简单使用下了。

使用
##########

运行一个示例
=============

#. 将K210板子插入到电脑USB

#. 在进行开发编译代码之前，我们还需要确认下对板子的配置，如下图配置项：

    .. figure:: ../_static/board_config.png
        :align: center
        :width: 400
        :figclass: align-center

    这里我们先按默认配置来，注意更改成自己板子占用的串口，后面在详细说明每个配置项的作用。

#. 打开 ``文件`` > ``示例`` 找到k210的例子，选择 ``Ticker`` > ``Arguments`` ，该示例演示了使用定时器控制led闪烁。

    .. figure:: ../_static/ticker_example.png
        :align: center
        :width: 400
        :figclass: align-center


#. 点击 ``上传`` 按钮，会执行编译并烧录程序到板子。

    .. figure:: ../_static/example_upload.png
        :align: center
        :width: 700
        :figclass: align-center
    
    出现上传成功提示，就可以看到板子上的led在闪烁了。到此为止就完成了示例代码的运行演示。


如果想要自己在示例代码基础上修改，可以点击 ``保存`` 按钮，把代码保存到自己的项目路径下，然后在进行修改编译上传。


社区库使用
=============

我们知道Arduino之所以成功正是因为它有众多社区提供的三方库，使用三方库可以大大简化我们的开发工作量，快速实现想要的功能。

#.  从 ``工具`` > ``管理库`` 菜单中打开 ``库管理器`` ，在搜索框直接搜索想要找的库，例如 ``Adafruit GFX`` 。找到后安装即可。

#. 从 ``项目`` > ``加载库`` 菜单中加载要使用的库，就会自动在代码编辑页面顶部加入该库的头文件，以便我们调用库的API。


开发板配置项
=============


* CPU Clock Frequency
    
    设置CPU时钟运行频率，一般保持400M默认即可。

* Burn Serial Buadrate

    设置串口烧录程序时的速率，如果经常烧录失败可以适当调低。

* Enable Rt-Thread Console

    是否开启Rt-Thread的调试打印。

    - 使能后烧录串口可以看到rtt系统的打印信息，使用 ``rt_kprintf`` 接口输出打印。
    - 禁止后Arduino默认的 ``Serial`` 串口可以使用烧录口对外通信， ``Serial.print`` 可以正常输出。

* Rt-Thread Main Thread Stack Size

    调整Rt-Thread主线程的堆栈大小，一般保持默认，如果写的应用程序过大时会报错堆栈内存不够，此时可以调大。

* Only Support KMODEL V3

    是否只支持 KMODEL V3，默认设置是禁止的（支持V4），如过想裁剪固件大小减少内存占用，且确定自己没有使用KMODEL V3版本以上模型的情况下可以启用该项。

* 端口

    选择开发板的烧录串口。


编译好的二进制程序分发
=========================

如果你想把编译好程序的bin文件分享给他人使用，参考以下说明：

* 点击 ``验证`` 或者 ``烧录`` 按钮，确保程序可以正常编译。

* 点击 ``项目`` > ``导出已编译的二进制文件`` 会将刚刚编好的bin文件导出到你的程序项目路径下。
    *注意：* 如果打开的是示例代码，则需要先保存项目文件到自己的项目路径下，才可以成功导出bin。



.. _arduino.cc网站: https://www.arduino.cc/en/software
