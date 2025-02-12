#include <Arduino.h>
#include <LSM6DS3.h>
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

unsigned long gpsWaitStart = 0;  // GPS 等待的起始时间

int adjustTimezone(int hour, int offset);

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

    // 初始化 I2C
    Wire.begin(SDA, SCL);
    Wire.setClock(100000);  // 设置 I2C 速度为 100kHz
    IMU.begin();            // LSM6DS3 使用默认的初始化方式

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
        double altitude = -1;
        int hour = -1;
        int minute = -1;
        int second = -1;
        if (gps.altitude.isValid()) {
            altitude = gps.altitude.meters();
        }
        if (gps.time.isValid()) {
            hour = gps.time.hour();
            minute = gps.time.minute();
            second = gps.time.second();
        }

        // 串口输出 GPS 数据
        Serial.print("纬度: ");
        Serial.print(latitude, 6);

        Serial.print(" 经度: ");
        Serial.println(longitude, 6);

        Serial.print(" 时间(UTC): ");
        if (hour != -1) {
            Serial.printf("%02d:%02d:%02d", hour, minute, second);
        } else {
            Serial.print("NaN");
        }

        Serial.print(" 卫星数量: ");
        Serial.print(gps.satellites.value());

        Serial.print(" 海拔: ");
        if (altitude != -1) {
            Serial.print(altitude);
            Serial.print(" m");
        } else {
            Serial.print("NaN");
        }

        Serial.println();

        // 显示 GPS 数据
        u8g2.setCursor(0, 10);
        u8g2.printf("纬度: %.6f", latitude);
        u8g2.setCursor(0, 23);
        u8g2.printf("经度: %.6f", longitude);
        u8g2.setCursor(0, 36);
        u8g2.printf("海拔: %.2f m", altitude);
        u8g2.printf(" 时间: %02d:%02d:%02d", adjustTimezone(hour, 8), minute,
                    second);
    } else {
        unsigned long waitTime =
            (millis() - gpsWaitStart) / 1000;  // 计算等待的秒数

        u8g2.setCursor(0, 10);
        u8g2.print("等待GPS信号...");
        u8g2.setCursor(0, 23);
        u8g2.print("等待时间: ");
        u8g2.print(waitTime);
        u8g2.print(" 秒");
    }

    float ax, ay, az;
    float gx, gy, gz;
    float temperature;

    // 读取加速度数据
    if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(ax, ay, az);
        Serial.printf("加速度(m/s²) X: %f Y: %f Z: %f\n", ax * 9.8, ay * 9.8,
                      az * 9.8);
    }

    // 读取陀螺仪数据
    if (IMU.gyroscopeAvailable()) {
        IMU.readGyroscope(gx, gy, gz);
        Serial.printf("陀螺仪(°/s) X: %f Y: %f Z: %f\n", gx, gy, gz);
    }

    // 读取温度数据
    if (IMU.temperatureAvailable()) {
        IMU.readTemperature(temperature);
        Serial.printf("温度: %f °C\n", temperature);
    }

    u8g2.setCursor(0, 49);
    u8g2.printf("加速度X:%.2f Y:%.2f Z:%.2f", ax * 9.8, ay * 9.8, az * 9.8);

    u8g2.setCursor(0, 62);
    u8g2.printf("陀螺仪X:%.2f Y:%.2f Z:%.2f", gx, gy, gz);

    /* u8g2.setCursor(0, 0);
    u8g2.printf("温度: %.2f °C", temperature); */

    u8g2.sendBuffer();  // 更新 OLED 屏幕
    delay(500);         // 延迟 500 毫秒
}

int adjustTimezone(int hour, int offset) {
    hour += offset;
    if (hour >= 24) {
        hour -= 24;  // 跨过一天
    } else if (hour < 0) {
        hour += 24;  // 回到前一天
    }
    return hour;
}