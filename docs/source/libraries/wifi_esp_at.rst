############
WiFiEspAT
############

如需使用esp wifi作为网络模块，可以使用Arduino库管理器安装WiFiEspAT，该库可以适用于esp8266或esp32系列wifi模块, esp32c3 esp32s2等，需要对esp模组烧录对应版本的AT固件，详情查阅WiFiEspAT库说明： WiFiEspAT源码_。

该库通过 ESP8266 或 ESP32 AT 命令创建标准的 Arduino WiFi 网络 API。Arduino WiFi网络API由Arduino WiFi库建立，并由Arduino WiFi101和Arduino WiFiNINA库增强。
该库快速可靠。它可以在高波特率下与AT固件通信，而无需流量控制，仅受UART在所选速度下的可靠性的限制。
该库适用于所有Arduino MCU架构。



例程 - WebServer.ino
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**WebServer测试**

以下是一个基于Arduino的WiFi Web服务器程序，它使用WiFiEspAT库通过ESP8266或esp32模块连接到WiFi网络，并在本地主机上启动Web服务器，接收HTTP请求并返回相应的HTML页面。

在该程序中，我们首先声明了一个WiFiServer对象和WiFiClient对象，通过WiFiEspAT库初始化并连接到WiFi网络。然后，我们在setup()函数中等待与WiFi网络的连接，并启动Web服务器，将设备IP地址显示在串行端口上。\
在loop()函数中，我们使用server.available()函数来检测是否有客户端连接到Web服务器，并读取客户端发送的HTTP请求，并根据请求内容生成相应的HTML响应结果，最后将其作为HTTP响应返回给客户端。

需要注意的是，在使用WiFiEspAT库时，需要根据具体情况调整波特率、串口引脚和硬件连接；同时，在进行Web开发时，需要考虑安全问题并遵守相关规定，以避免出现潜在的漏洞和安全隐患。\
此外，在编写Web服务器程序时，还需要注意HTTP协议格式和数据解析方法，以确保程序正确性和稳定性。

测试以下代码需要设置 `Enable Rt-Thread Console:` 选项为Disable，并配置自己的串口通信引脚连接到esp模组。更改为自己的wifi ssid与password。

.. code-block:: arduino

    // 引入WiFiEspAT库
    #include <WiFiEspAT.h>

    // 定义串口1
    HardwareSerial Serial1(1);
    #define AT_BAUD_RATE 115200

    // 定义WiFi网络名称和密码
    const char ssid[] = "wifissid";    // your network SSID (name)
    const char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)

    // 定义WiFi服务器
    WiFiServer server(80);

    void setup() {
        // 初始化串口
        Serial.begin(115200);
        while (!Serial);

        // 初始化WiFi模块
        Serial1.begin(AT_BAUD_RATE, SERIAL_8N1, 10, 11);
        WiFi.init(Serial1);

        // 检查WiFi模块是否正常
        if (WiFi.status() == WL_NO_MODULE) {
            Serial.println("Communication with WiFi module failed!");
            // don't continue
            while (true);
        }

        // 连接WiFi网络
        int status = WiFi.begin(ssid, pass);
        Serial.println();

        if (status != WL_CONNECTED) {
            Serial.println("Failed to connect to AP");
        } else {
            Serial.println("You're connected to the network");
        }
        
        // 等待连接到WiFi网络
        Serial.println("Waiting for connection to WiFi");
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print('.');
        }
        Serial.println();

        // 开启WiFi服务器
        server.begin();

        // 获取本地IP地址
        IPAddress ip = WiFi.localIP();
        Serial.println();
        Serial.println("Connected to WiFi network.");
        Serial.print("To access the server, enter \"http://");
        Serial.print(ip);
        Serial.println("/\" in web browser.");
    }

    // 循环处理客户端请求
    void loop() {
        // 等待客户端连接
        WiFiClient client = server.available();
        if (client) {
            IPAddress ip = client.remoteIP();
            Serial.print("new client ");
            Serial.println(ip);

            // 处理客户端请求
            while (client.connected()) {
                if (client.available()) {
                    String line = client.readStringUntil('\n');
                    line.trim();
                    Serial.println(line);

                    // 如果已经到达HTTP头的末尾（空行），则开始回复
                    if (line.length() == 0) {
                        // 发送标准HTTP响应头
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/html");
                        client.println("Connection: close");  // 完成响应后关闭连接
                        client.println("Refresh: 5");  // 每5秒自动刷新页面
                        client.println();
                        client.println("<!DOCTYPE HTML>");
                        client.println("<html>");
                        // 输出模拟输入引脚的值
                        for (int analogChannel = 0; analogChannel < 4; analogChannel++) {
                            int sensorReading = analogRead(analogChannel);
                            client.print("analog input ");
                            client.print(analogChannel);
                            client.print(" is ");
                            client.print(sensorReading);
                            client.println("<br />");
                        }
                        client.println("</html>");
                        client.flush();
                        break;
                    }
                }
            }

            // 关闭连接
            client.stop();
            Serial.println("client disconnected");
        }
    }
    // 循环处理客户端请求结束


.. _WiFiEspAT源码: https://github.com/JAndrassy/WiFiEspAT
