#include <Arduino.h>
#include <HTTPClient.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <WiFi.h>

// 定义引脚
#define LED_PIN 8
#define BUTTON_PIN 0

// GPS 模块参数
#define GPS_RX_PIN 2  // GPS 模块 TX 接到的引脚
#define GPS_TX_PIN 3  // GPS 模块 RX 接到的引脚
#define GPS_BAUD 9600

// BLE 服务和特征的 UUID
#define SERVICE_UUID "38182fed-7e5a-4b03-9d17-67d8ebd094e1"
#define CHARACTERISTIC_UUID "e4a863f6-9993-449f-bf85-0ed9b108d539"

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

// 函数声明
void handleButton();
void saveWiFi(const String& ssid, const String& password);
void loadWiFi(String& ssid, String& password);

Preferences preferences;

HardwareSerial GPS(1);  // 使用 UART1
TinyGPSPlus gps;

bool inited = false;

// BLE 服务器回调类
class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) { Serial.println("设备已连接！"); }
  void onDisconnect(NimBLEServer* pServer) { Serial.println("设备已断开！"); }
};

// BLE 特征回调类
class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic) {
    Serial.println("读取数据...");
    pCharacteristic->setValue("Hello World!");
  }
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    Serial.println("写入数据...");
    String input = pCharacteristic->getValue().c_str();  // 更新字符串数据
    int startIdx = input.indexOf("$");
    int endIdx = input.indexOf("!");

    if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
      String command = input.substring(startIdx + 1, endIdx);
      // "$W:[MyWiFi][MyPassword]!"
      if (command.startsWith("W:")) {  // Wifi配置
        int ssidStart = input.indexOf("[", input.indexOf(':')) + 1;
        int ssidEnd = input.indexOf("]", ssidStart);
        if (ssidStart == 0 || ssidEnd == -1) {
          Serial.println("格式错误: SSID 解析失败");
          return;
        }
        String ssid = input.substring(ssidStart, ssidEnd);

        int passwordStart = input.indexOf("[", ssidEnd) + 1;
        int passwordEnd = input.indexOf("]", passwordStart);
        if (passwordStart == 0 || passwordEnd == -1) {
          Serial.println("格式错误: 密码解析失败");
          return;
        }
        String password = input.substring(passwordStart, passwordEnd);
        Serial.println("SSID: " + ssid + ", 密码: " + password);
        saveWiFi(ssid, password);
      }
    }
  }
};

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);  // 设置 LED 引脚为输出模式
  Serial.begin(9600);

  // 初始化 BLE
  NimBLEDevice::init("ESP32-C3");

  // 创建 BLE 服务器
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 创建服务
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // 创建特征
  NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // 启动服务
  pService->start();

  // 开启广告
  NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();

  String storedSSID, storedPassword;
  loadWiFi(storedSSID, storedPassword);
  Serial.println(storedSSID + ", " + storedPassword);
  if (storedSSID.length() > 0) {
    inited = true;
    WiFi.begin(storedSSID, storedPassword);
  }
}

void loop() {
  if (!inited) {
    unsigned long currentMillis = millis();  // 获取当前时间戳

    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;   // 更新上次闪烁时间
      ledState = !ledState;             // 切换 LED 状态
      digitalWrite(LED_PIN, ledState);  // 控制 LED
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