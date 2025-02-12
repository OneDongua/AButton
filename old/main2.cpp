#include <Arduino.h>
#include <Arduino_LSM6DS3.h>
#include <Wire.h>

bool inited = 0;

void setup() {
    Serial.begin(9600);
    Wire.begin(0, 1);
    Wire.setClock(100000);  // 设置 I2C 速度为 100kHz

    if (!IMU.begin()) {  // 使用默认的初始化方式
        Serial.println("初始化LSM6DS3失败!");
    } else {
        Serial.println("初始化LSM6DS3成功!");
        inited = 1;
    }
}

void loop() {
    if (inited) {
        float ax, ay, az;
        float gx, gy, gz;
        float temperature;

        // 读取加速度数据
        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(ax, ay, az);
            Serial.print("加速度(m/s²) X: ");
            Serial.print(ax * 9.8);
            Serial.print(" Y: ");
            Serial.print(ay* 9.8);
            Serial.print(" Z: ");
            Serial.println(az * 9.8);
        }

        // 读取陀螺仪数据
        if (IMU.gyroscopeAvailable()) {
            IMU.readGyroscope(gx, gy, gz);
            Serial.print("陀螺仪(°/s) X: ");
            Serial.print(gx);
            Serial.print(" Y: ");
            Serial.print(gy);
            Serial.print(" Z: ");
            Serial.println(gz);
        }

        // 读取温度数据
        if (IMU.temperatureAvailable()) {
            IMU.readTemperature(temperature);
            Serial.print("温度: ");
            Serial.print(temperature);
            Serial.println(" °C");
        }

        delay(500);
    } else {
        Serial.println("连接失败");
        delay(1000);
    }
}
