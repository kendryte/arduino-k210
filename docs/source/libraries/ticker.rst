############
Ticker
############

Arduino Ticker 库允许您轻松创建 Ticker 回调，这些回调可以在预定的时间间隔内调用函数。利用Ticker库，我们可以让K210定时调用某一个函数。
使用不同API您可以设定单次或重复运行。该库使用硬件定时器的中断，尽可能控制回调函数代码量大小。



attach
^^^^^^^^^^

描述
======

给 `Ticker` 设定回调函数以及调用间隔时间，回调重复运行。

语法
======

- 1.回调函数不带参

.. code-block:: arduino

    void attach(float seconds, callback_t callback);

- 2.回调函数带参数

.. code-block:: arduino

    void attach(float seconds, void (*callback)(TArg), TArg arg);

参数
======

* ``seconds`` 调用间隔时间，单位:秒

* ``callback`` 回调函数

* ``arg`` 回调函数的参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    void setPin(int state) {
        digitalWrite(LED_PIN, state);
    }
    Ticker tickerSetLow;
    tickerSetLow.attach(2, setPin, 0);

attach_ms
^^^^^^^^^^

描述
======

给 `Ticker` 设定回调函数以及调用间隔时间，回调重复运行。

语法
======

- 1.回调函数不带参

.. code-block:: arduino

    void attach_ms(uint32_t milliseconds, callback_t callback);

- 2.回调函数带参数

.. code-block:: arduino

    void attach_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg);

参数
======

* ``milliseconds`` 调用间隔时间，单位:毫秒

* ``callback`` 回调函数

* ``arg`` 回调函数的参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    void setPin(int state) {
        digitalWrite(LED_PIN, state);
    }
    Ticker tickerSetLow;
    tickerSetLow.attach_ms(25, setPin, 0);


once
^^^^^^^^^^

描述
======

给 `Ticker` 设定回调函数以及调用间隔时间，回调仅运行一次。

语法
======

- 1.回调函数不带参

.. code-block:: arduino

    void once(float seconds, callback_t callback);

- 2.回调函数带参数

.. code-block:: arduino

    void once(float seconds, void (*callback)(TArg), TArg arg);

参数
======

* ``seconds`` 调用间隔时间，单位:秒

* ``callback`` 回调函数

* ``arg`` 回调函数的参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    void change() {
        blinkerPace = 0.5;
    }
    Ticker changer;
    changer.once(30, change);

once_ms
^^^^^^^^^^

描述
======

给 `Ticker` 设定回调函数以及调用间隔时间，回调仅运行一次。

语法
======

- 1.回调函数不带参

.. code-block:: arduino

    void once_ms(uint32_t milliseconds, callback_t callback);

- 2.回调函数带参数

.. code-block:: arduino

    void once_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg);

参数
======

* ``milliseconds`` 调用间隔时间，单位:毫秒

* ``callback`` 回调函数

* ``arg`` 回调函数的参数


返回值
========

无

示例说明
============

.. code-block:: arduino

    void change() {
        blinkerPace = 0.5;
    }
    Ticker changer;
    changer.once_ms(30, change);

detach
^^^^^^^^^^

描述
======

停止定时执行函数。

语法
======

.. code-block:: arduino

    void detach();


返回值
========

无

示例说明
============

.. code-block:: arduino

    Ticker blinker;
    blinker.detach();

active
^^^^^^^^^^

描述
======

检测定时器是否使能，是则返回True。

语法
======

.. code-block:: arduino

    bool active();


返回值
========

是否使能

示例说明
============

.. code-block:: arduino

    Ticker blinker;
    bool en = blinker.active();

例程 - Blinker.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**使用 `Ticker` 控制led闪烁**

以下是一个LED闪烁控制程序，它通过定时器和GPIO控制LED的亮灭状态。具体实现包括三个定时器：blinker、toggler和changer。

首先，在setup()函数中，我们配置了LED接口为输出模式，并使用toggler.attach()函数周期性地调用toggle()函数来让LED在固定时间间隔内闪烁。同时，我们使用changer.once()函数延迟一段时间后调用change()函数，从而改变blinkerPace的值，使LED闪烁的速度更快。

然后，在toggle()函数中，我们通过isBlinking变量来判断当前是否正在闪烁LED，如果是，则通过blinker.detach()函数停止闪烁；如果不是，则通过blinker.attach()函数开始闪烁LED。

最后，在blink()函数中，我们使用digitalWrite()函数来改变LED的状态。

需要注意的是，该程序中使用了多个定时器，因此需要避免定时器之间的冲突或重叠。同时，由于涉及到硬件控制，必须小心处理，以避免损坏设备。

.. literalinclude:: ../../../libraries/Ticker/examples/Blinker/Blinker.ino
    :language: arduino


