#include "Arduino.h" // 引入Arduino库

#include "SHA256.h" // 引入SHA256库

using namespace K210; // 使用K210命名空间

void setup() // 初始化函数
{
    uint8_t sha256[32]; // 定义SHA256哈希值数组
    uint8_t data[] = "012345678901234567890123456789"; // 定义待哈希数据

    Serial.begin(115200); // 初始化串口通信
    while (!Serial) { // 等待串口连接
        ;
    }

    Serial.printf("Sha256 Test\n"); // 输出测试信息

    SHA256::begin(30); // 初始化SHA256哈希函数
    SHA256::update(data, 30); // 更新哈希值
    SHA256::digest(sha256); // 计算哈希值
    SHA256::end(); // 结束哈希函数

    /* 276FADFC9EDC49F5F9AF96D97636731DEF7525D4BFA16BC07699534873A474CC */
    Serial.printf("sha256:"); // 输出哈希值
    for(int i = 0; i < 32; i++) { // 循环输出哈希值
        Serial.printf("%02X", sha256[i]); // 输出哈希值
    }
    Serial.printf("\r\n"); // 换行
}

void loop() // 循环函数
{

}
