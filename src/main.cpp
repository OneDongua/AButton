#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <WiFi.h>

// 定义引脚
#define LED_PIN 8
#define BUTTON_PIN 0

// GPS 模块参数
#define GPS_RX_PIN 20  // GPS 模块 TX 接到的引脚 是TX！！！反过来定义的！！！
#define GPS_TX_PIN 21  // GPS 模块 RX 接到的引脚 是RX！！！
#define GPS_BAUD 9600

// 按键相关参数
#define SHORT_PRESS_TIME 2000  // 短按阈值，单位毫秒
#define LONG_PRESS_TIME 10000  // 长按阈值，单位毫秒
#define DEBOUNCE_TIME 10       // 去抖动时间，单位毫秒

// 按键状态变量
unsigned long lastDebounceTime = 0;  // 上一次按键状态改变的时间
unsigned long buttonPressTime = 0;   // 按钮按下的时间
bool lastButtonState = LOW;          // 上一个按钮状态
bool buttonState = LOW;              // 当前按钮状态
bool buttonPressed = false;          // 按钮是否被按下

// LED 相关变量
unsigned long previousMillis = 0;  // 记录上次 LED 状态改变的时间戳
bool ledState = false;             // 记录 LED 当前状态

// GPS 相关变量
HardwareSerial GPS(1);  // 使用 UART1
TinyGPSPlus gps;
bool gpsUpdated = false;

// 函数声明
void handleButton();
void saveWiFi(const String& ssid, const String& password);
void loadWiFi(String& ssid, String& password);
void sendPostRequest();

Preferences preferences;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);  // 设置 LED 引脚为输出模式
  Serial.begin(9600);
  GPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  WiFi.begin("ChinaNet-GSLh", "uhcxwszh");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();  // 获取当前时间戳

    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;   // 更新上次闪烁时间
      ledState = !ledState;             // 切换 LED 状态
      digitalWrite(LED_PIN, ledState);  // 控制 LED
    }
  } else {
    unsigned long currentMillis = millis();  // 获取当前时间戳

    if (currentMillis - previousMillis >= 500) {
      previousMillis = currentMillis;  // 更新上次闪烁时间

      // 更新 gps 数据
      while (GPS.available()) {
        char c = GPS.read();  // 从 GPS 模块读取字符
        gps.encode(c);        // 使用 TinyGPS++ 解析数据

        if (!gpsUpdated && gps.location.isUpdated()) {
          // 第一次更新GPS数据
          gpsUpdated = true;
          Serial.println("GPS已接收到第一次更新！");
        }
      }

      if (!gpsUpdated) {
        ledState = !ledState;             // 切换 LED 状态
        digitalWrite(LED_PIN, ledState);  // 控制 LED
      }
    }
  }

  handleButton();
}

void handleButton() {
  bool reading = !digitalRead(BUTTON_PIN);  // 读取按钮状态

  // 去抖动处理
  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // 如果状态变化，记录当前时间
  }

  // 如果状态稳定超过去抖动时间
  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
    if (reading != buttonState) {
      buttonState = reading;

      // 按钮状态改变时
      if (buttonState == HIGH) {
        // 按钮被按下
        buttonPressTime = millis();  // 记录按下时间
        buttonPressed = true;
      } else {
        // 按钮释放
        if (buttonPressed) {
          unsigned long pressDuration = millis() - buttonPressTime;
          if (pressDuration < SHORT_PRESS_TIME) {
            Serial.println("短按");
            sendPostRequest();
          } else if (pressDuration >= SHORT_PRESS_TIME &&
                     pressDuration < LONG_PRESS_TIME) {
            Serial.println("长按");
          } else if (pressDuration >= LONG_PRESS_TIME) {
            Serial.println("超长按");
          }
          buttonPressed = false;
        }
      }
    }
  }
  // 记住当前按钮状态
  lastButtonState = reading;
}

void saveWiFi(const String& ssid, const String& password) {
  preferences.begin("wifi", false);  // 打开命名空间 "wifi"，false 代表可写
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();  // 关闭存储
  Serial.println("WiFi 凭据已保存！");
}

void loadWiFi(String& ssid, String& password) {
  preferences.begin("wifi", true);                   // 以只读模式打开存储
  ssid = preferences.getString("ssid", "");          // 读取 SSID，默认值为空
  password = preferences.getString("password", "");  // 读取密码
  preferences.end();                                 // 关闭存储
}

void sendPostRequest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接，无法发送HTTP请求！");
    return;
  }
  double latitude, longitude, altitude;
  if (gps.location.isValid()) {
    // 输出解析后的常用信息
    Serial.print("Latitude: ");
    Serial.print(gps.location.lat(), 6);  // 输出纬度
    Serial.print(", Longitude: ");
    Serial.print(gps.location.lng(), 6);  // 输出经度
    Serial.print(", Altitude: ");
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters());
      Serial.print(" m");
    } else {
      Serial.print("高度无效");
    }
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;

    Serial.println();
  } else {
    Serial.println("无可用GPS数据，使用默认值");
    latitude = 23.074334;
    longitude = 114.415251;
    altitude = 20.00;
  }
  HTTPClient http;
  http.setTimeout(5000);  // 设置超时时间为5秒
  String url = "http://192.168.2.185:3001/api/notification/875658697@qq.com";

  JsonDocument doc;
  doc["content"] = "新急救消息";
  doc["from"] = "hardware";
  JsonObject location = doc["location"].to<JsonObject>();
  location["latitude"] = latitude;
  location["longitude"] = longitude;
  location["altitude"] = altitude;

  String jsonData;
  serializeJson(doc, jsonData);

  // 配置 HTTP 客户端
  http.begin(url);

  // 设置请求头
  http.addHeader("Content-Type", "application/json");

  // 发送 POST 请求
  int httpResponseCode = http.POST(jsonData);

  // 检查响应状态码
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("错误代码: ");
    Serial.println(httpResponseCode);
  }

  // 结束 HTTP 客户端
  http.end();
}