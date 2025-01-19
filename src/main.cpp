#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// 定义 LED 与 按键引脚
#define LED_PIN 8
#define BUTTON_PIN 9

const char *ssid = "ChinaNet-GSLh";
const char *password = "uhcxwszh";

const String url = "http://192.168.2.30:3001/";

// 定义 LED 逻辑值
int led_logic = 0;
// 判断 LED 的状态是否改变过
bool status = false;

void connectWiFi();
void upload();

void setup()
{
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  connectWiFi();
}

void loop()
{
  // 按键消抖
  if (digitalRead(BUTTON_PIN))
  {
    // 睡眠 10ms，如果依然为高电平，说明抖动已消失。
    delay(10);
    if (digitalRead(BUTTON_PIN) && !status)
    {
      led_logic = !led_logic;
      digitalWrite(LED_PIN, led_logic);
      upload();
      // led 的状态发生了变化，即使我持续按着按键，LED 的状态也不应该改变。
      status = !status;
    }
    else if (!digitalRead(BUTTON_PIN))
    {
      status = false;
    }
  }
}

void upload()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // 创建 HTTPClient 对象
    HTTPClient http;

    // 发送GET请求
    http.begin(url + "upload");
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(1000);

    int httpCode = http.POST("{\"msg\": \"Hello from ESP32: " + String(led_logic ? "OFF" : "ON") + "\"}");

    if (httpCode > 0)
    {
      // 获取响应状态码
      Serial.printf("HTTP 状态码: %d", httpCode);
      Serial.print(" ");

      // 获取响应正文
      String response = http.getString();
      Serial.print("响应数据：");
      Serial.println(response);
    }
    else
    {
      Serial.print("错误：");
      Serial.println(http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi未连接");
  }
}

void connectWiFi()
{
  // 连接 WiFi
  WiFi.begin(ssid, password);

  Serial.print("正在连接 Wi-Fi");

  // 检测是否连接成功
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("连接成功");
  Serial.print("IP 地址：");
  Serial.println(WiFi.localIP());
}