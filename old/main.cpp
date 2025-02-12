#include <Arduino.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>
#include <Wire.h>

// 自定义 I2C 引脚
#define SDA 0
#define SCL 1

// 使用 I2C 接口初始化 OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

// GPS 模块参数
#define GPS_RX_PIN 2  // GPS 模块 TX 接到的引脚
#define GPS_TX_PIN 3  // GPS 模块 RX 接到的引脚
#define GPS_BAUD 9600

HardwareSerial GPS(1);  // 使用 UART1
TinyGPSPlus gps;

unsigned long lastGPSUpdate = 0;  // 上次 GPS 更新的时间
unsigned long gpsWaitStart = 0;   // GPS 等待的起始时间

void setup() {
    // 初始化串口
    Serial.begin(9600);  // 调试串口
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

    // 初始化 OLED
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 设置中文字体
    u8g2.setFontDirection(0);

    gpsWaitStart = millis();  // 初始化等待计时器
    Serial.println(F("系统初始化完成！"));
}

void loop() {
    // 读取 GPS 数据
    while (GPS.available()) {
        char c = GPS.read();
        gps.encode(c);  // 将数据传递给 TinyGPS++
    }

    // 更新显示内容
    u8g2.clearBuffer();

    if (gps.location.isUpdated()) {
        double latitude = gps.location.lat();
        double longitude = gps.location.lng();
        lastGPSUpdate = millis();  // 更新 GPS 数据获取时间

        Serial.print("纬度: ");
        Serial.print(latitude, 6);
        Serial.print(" 经度: ");
        Serial.println(longitude, 6);

        // 显示 GPS 数据
        u8g2.setCursor(0, 10);
        u8g2.print("纬度: ");
        u8g2.print(latitude, 6);

        u8g2.setCursor(0, 30);
        u8g2.print("经度: ");
        u8g2.print(longitude, 6);
    } else {
        unsigned long waitTime =
            (millis() - gpsWaitStart) / 1000;  // 计算等待的秒数

        u8g2.setCursor(0, 10);
        u8g2.print("等待GPS信号...");
        u8g2.setCursor(0, 30);
        u8g2.print("等待时间: ");
        u8g2.print(waitTime);
        u8g2.print(" 秒");
    }

    u8g2.sendBuffer();  // 更新 OLED 屏幕
    delay(500);         // 延迟 500 毫秒
}
