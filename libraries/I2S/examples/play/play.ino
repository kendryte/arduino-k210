#include "Arduino.h" // 引入Arduino库

#include "I2S.h" // 引入I2S库
#include "FFat.h" // 引入FFat库

#include "k210-hal.h" // 引入k210-hal库
#include "rtdevice.h" // 引入rtdevice库

#define USE_LOCAL_WAV       (0) // 定义USE_LOCAL_WAV为0

#if USE_LOCAL_WAV // 如果USE_LOCAL_WAV为真
#include "incbin.h" // 引入incbin库
INCBIN(wav, "./vip.wav"); // 将wav文件读入内存
#else // 如果USE_LOCAL_WAV为假
#define gwavSize    (16000 * 2 * 5) // 定义gwavSize为16000 * 2 * 5
#endif // USE_LOCAL_WAV

I2SClass I2S(0, 0, 18, 35, 33); // 初始化I2S

struct rt_completion _txdone; // 定义rt_completion类型的变量_txdone

static void _i2s_transmit_done(void *ctx) // 定义_i2s_transmit_done函数
{
    struct rt_completion *c = (struct rt_completion *)ctx;

    rt_completion_done(c);
}

void setup() {
    Serial.begin(115200); // 初始化串口通信
    while (!Serial) { // 如果串口没有连接
        ;
    }

    Serial.printf("Hello World\r\n"); // 输出Hello World

    pinMode(11, OUTPUT); // 设置11号引脚为输出
    digitalWrite(11, HIGH); // 将11号引脚输出高电平

    rt_completion_init(&_txdone); // 初始化_txdone

    I2S.begin(I2S_RIGHT_JUSTIFIED_MODE, 16000, 16); // 初始化I2S
    I2S.onTransmit(_i2s_transmit_done, &_txdone); // I2S传输完成

#if USE_LOCAL_WAV // 如果USE_LOCAL_WAV为真
    uint8_t *buff = (uint8_t *)gwavData; // 定义uint8_t类型的指针buff，指向gwavData
#else // 如果USE_LOCAL_WAV为假
    uint8_t *buff = (uint8_t *)rt_malloc(gwavSize); // 定义uint8_t类型的指针buff，指向gwavSize
    if(NULL == buff) // 如果buff为空
    {
        Serial.printf("malloc failed"); // 输出malloc failed
        return;
    }
    for(size_t i = 0; i < gwavSize; i++)
    {
        buff[i] = (uint8_t)random(0, 255); // 将随机数赋值给buff[i]
    }
#endif // USE_LOCAL_WAV

    size_t remain, write, sent = 0, total = gwavSize;

    Serial.printf("wav size %ld\r\n", total); // 输出wav size

    do { // 循环
        remain = (total - sent); // 计算remain
        write = I2S.write(buff + sent, remain); // 将buff + sent写入I2S，返回写入的字节数
        sent += write; // 计算sent

        if(0 == write) { // 如果write为0
            rt_completion_wait(&_txdone, RT_WAITING_FOREVER); // 等待_txdone完成
        }
    } while (sent < total); // 如果sent小于total，继续循环

    Serial.printf("End\r\n"); // 输出End

    I2S.end(); // 结束I2S,释放硬件资源

#if USE_LOCAL_WAV // 如果USE_LOCAL_WAV为真
    rt_free(buff); // 释放buff
#endif //USE_LOCAL_WAV
}

void loop()
{
    rt_thread_delay(RT_TICK_PER_SECOND); // 延时
}
