#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

// 定义 GPS 模块的串口
HardwareSerial GPS(1);  // 使用 UART1 作为 GPS 数据串口

// GPS 模块连接的引脚
#define GPS_RX_PIN 0  // GPS 模块 TX 接到的引脚
#define GPS_TX_PIN 1  // GPS 模块 RX 接到的引脚

TinyGPSPlus gps;  // 创建 TinyGPS++ 对象

void setup() {
    // 初始化串口监视器
    Serial.begin(9600);
    Serial.println("Starting GPS...");

    // 初始化 GPS 串口
    GPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void loop() {
    // 检查是否有 GPS 数据
    while (GPS.available()) {
        char c = GPS.read();  // 从 GPS 模块读取字符
        gps.encode(c);        // 使用 TinyGPS++ 解析数据

        // 输出解析后的常用信息
        if (gps.location.isUpdated()) {
            Serial.print("Latitude: ");
            Serial.print(gps.location.lat(), 6);  // 输出纬度
            Serial.print(", Longitude: ");
            Serial.print(gps.location.lng(), 6);  // 输出经度

            Serial.print(", Time: ");
            if (gps.time.isValid()) {
                Serial.printf("%02d:%02d:%02d", gps.time.hour(),
                              gps.time.minute(), gps.time.second());
            } else {
                Serial.print("Invalid");
            }

            Serial.print(", Satellites: ");
            Serial.print(gps.satellites.value());  // 输出卫星数量

            Serial.print(", Altitude: ");
            if (gps.altitude.isValid()) {
                Serial.print(gps.altitude.meters());
                Serial.print(" m");
            } else {
                Serial.print("Invalid");
            }

            Serial.println();
        } else {
            Serial.print(c);
        }
    }

    delay(500);
}
