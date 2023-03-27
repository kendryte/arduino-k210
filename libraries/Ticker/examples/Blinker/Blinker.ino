#include <Arduino.h>
#include <Ticker.h>

// attach a LED to pPIO 35
#define LED_PIN 35

Ticker blinker; // 定义一个Ticker对象，用于控制LED闪烁
Ticker toggler; // 定义一个Ticker对象，用于控制LED闪烁的开关
Ticker changer; // 定义一个Ticker对象，用于控制LED闪烁的速度变化
float blinkerPace = 0.1;  //seconds // 定义闪烁的时间间隔为0.1秒
const float togglePeriod = 5; //seconds // 定义LED闪烁的周期为5秒

void change() { // 定义一个函数，用于改变LED闪烁的速度
  blinkerPace = 0.5;
}

void blink() { // 定义一个函数，用于LED闪烁
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void toggle() { // 定义一个函数，用于控制LED闪烁的开关
  static bool isBlinking = false;
  if (isBlinking) {
    blinker.detach(); // 如果LED正在闪烁，就停止闪烁
    isBlinking = false;
  }
  else {
    blinker.attach(blinkerPace, blink); // 如果LED没有在闪烁，就开始闪烁
    isBlinking = true;
  }
  // 确保LED在闪烁开关切换后处于开启状态
  digitalWrite(LED_PIN, LOW);  //make sure LED on on after toggling (pin LOW = led ON)
}

void setup() {
  pinMode(LED_PIN, OUTPUT); // 设置LED引脚为输出模式
  toggler.attach(togglePeriod, toggle); // 设置LED闪烁的周期
  changer.once(30, change); // 设置LED闪烁的速度变化
}

void loop() {
  
}
