#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESP8266Audio.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3.h>

#define PIN_LED 8
#define PIN_BUTTON 9

#define I2S_LRC 0
#define I2S_BCLK 1
#define I2S_DOUT 2

const char* ssid = "ChinaNet-GSLh";
const char* password = "uhcxwszh";

const char* url = "http://192.168.2.30:3001/static/music.mp3";

// 创建 ESP8266Audio 所需的对象
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *httpStream;
AudioFileSourceBuffer* bufferedStream;
AudioOutputI2S *out;

void setup() {
    Serial.begin(9600);

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLDOWN);

    // 连接 WiFi
    WiFi.begin(ssid, password);
    Serial.print("正在连接 Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("连接成功");
    Serial.print("IP 地址：");
    Serial.println(WiFi.localIP());

    // 初始化 I2S 输出
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    out->SetGain(0.5); // 设置音量（范围：0.0 - 1.0）

    // 初始化音频流和解码器
    httpStream = new AudioFileSourceHTTPStream(url);
    bufferedStream = new AudioFileSourceBuffer(httpStream, 8192);
    mp3 = new AudioGeneratorMP3();
    mp3->begin(bufferedStream, out);
}

void loop() {
    // 持续播放音频
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            mp3->stop();
        }
    } else {
        Serial.println("音频播放完成或发生错误");
        delay(1000); // 避免频繁重试
    }
}
