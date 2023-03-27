#include "Arduino.h"

#include "I2S.h"
#include "FFat.h"

#include "k210-hal.h"
#include "rtdevice.h"

I2SClass I2S(0, 0, 34, 32, 33);

struct rt_completion _rxdone;

// I2S接收完成回调函数
static void _i2s_receive_done(void *ctx)
{
    struct rt_completion *c = (struct rt_completion *)ctx;

    rt_completion_done(c);
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.printf("Hello World\r\n");

    // 获取I2S缓冲区大小
    int buffSz = I2S.getBufferSize();
    // 分配I2S缓冲区
    uint8_t *buff = (uint8_t *)rt_malloc_align(buffSz, 8);
    if(!buff) {
        Serial.printf("Malloc failed");
        return;
    }

    // 初始化 完成标志
    rt_completion_init(&_rxdone);

    // 初始化I2S
    I2S.begin(I2S_PHILIPS_MODE, 16000, 16);
    I2S.onReceive(_i2s_receive_done, &_rxdone);

    // 初始化FFat
    if(!FFat.begin()){
        Serial.printf("FFat Mount Failed");
        I2S.end();
        return;
    }

    // 打开文件
    File file = FFat.open("/16K.raw", FILE_WRITE);
    if(!file){
        Serial.printf("- failed to open file for writing");
        I2S.end();
        FFat.end();
        return;
    }

    // 读取I2S数据并写入文件
    for(int i = 0; i < 1024; i++) {
        int readSz = I2S.read(buff, buffSz);

        if(0x00 == readSz) {
            rt_completion_wait(&_rxdone, RT_WAITING_FOREVER);
            continue;
        }

        size_t w = file.write(buff, readSz);

        Serial.printf("read %d, write %ld\r\n", readSz, w);
    }

    // 刷新文件并关闭
    file.flush();
    Serial.printf("file size %ld\r\n", file.size());
    file.close();

    // 结束FFat和I2S
    FFat.end();
    I2S.end();

    // 释放I2S缓冲区
    rt_free_align(buff);

    Serial.printf("End\r\n");
}

// 循环函数
void loop() {

}
