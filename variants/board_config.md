# 开发板适配 | Board Configuration

本指南说明如何把自己的开发板适配进arduino项目。

适配一个新的板子，主要是改动两个文件：`variants/xxx/pins_arduino.h` 和 `boards.txt`。   

1. 引脚配置

    由于开发板定义的lcd，camera等外设不一定相同，为了保证引脚对应的各种外设驱动模块可以匹配，就需要在`variants/xxx/pins_arduino.h`文件中定义。

    给自己的开发板起一个名字，并新建目录位于`variants/`下。可以仿照`canaan_k1`的配置，拷贝他下面的`pins_arduino.h`到自己板子目录下，文件中定义了Serial调试口，LCD，SD卡，Camera等外设的引脚，按需修改成自己板子的配置。

2. `boards.txt`新增板型配置

    引脚定义配置文件修改好后，需要把定义的板子加入到项目根目录的`boards.txt`文件中，打开`boards.txt`文件可以看到其中已经定义好的的两个板子，可以直接复制其中一个添加到下面，然后修改主要部分。

    以canaan_k1板子为例，重点说明以下几个配置项：

    - k1.name=Canaan K1
        这部分是板子的名字，按需要改成自己定义的。这部分就是arduino IDE中板子选择项展示的名字。

    - k1.build.variant=canaan_k1
        指定variant名字，这部分填写的内容必须保证和`variant/`目录下你新建的那个板子目录名一致。这样arduino才可以找到对应的`pins_arduino.h`

    - k1.build.board=CANAAN_K1
        ARDUINO_BOARD宏定义名称。

    - k1.build.burn_board=dan
        烧录脚本工具的板型选择配置。


