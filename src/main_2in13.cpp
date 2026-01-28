#include <Arduino.h>
#include <SPI.h>
#include <bluefruit.h>
#include <TimeLib.h>
#include <Wire.h>
#include "epd2in13_V3.h" 
#include "epdpaint.h"

#include <Adafruit_LittleFS.h>

// --- 蓝牙对象 ---
BLEClientService        ctsSvc(UUID16_SVC_CURRENT_TIME);
BLEClientCharacteristic ctcChar(UUID16_CHR_CURRENT_TIME);

// --- 时间结构体 ---
struct cts_time_t {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t day_of_week;
  uint8_t fractions256;
  uint8_t adjust_reason;
} __attribute__((packed));

extern Epd epd;
extern Paint paint; 
extern bool isFirstUpdate; 
extern int prevSecond;  
extern BLEUart bleuart;

// --- 函数声明 ---
void setupDisplay();
void updateClockDisplay();

void setupBLE();
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len);
void pair_callback(uint16_t conn_handle, uint8_t auth_status);
void handleBLEUart(void);

extern const unsigned char gImage_test[2480];
extern unsigned char image[4000]; 

void setup() {
  Serial.begin(115200);

  epd.Init(FULL); 
  epd.Clear(); 
  Serial.println("Initializing display...");

  paint.SetWidth(epd.width);
  paint.SetHeight(epd.height);
  paint.SetRotate(ROTATE_0);

  setupDisplay(); 
  setupBLE();
  
}

void loop() {
  handleBLEUart();
  if (second() != prevSecond && year() > 2000) {
    prevSecond = second();
    updateClockDisplay();
  }
}

// --- 蓝牙部分 ---
void setupBLE() {
  Bluefruit.begin(1, 0); 
  Bluefruit.setName("EClock"); 

  Bluefruit.autoConnLed(false); 
  Bluefruit.Security.begin(); 
  Bluefruit.Security.setIOCaps(false, false, false); 
  Bluefruit.Security.setPairCompleteCallback(pair_callback); 
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  ctsSvc.begin();
  ctcChar.setNotifyCallback(notify_callback);
  ctcChar.begin();
  bleuart.begin();// 初始化 UART 服务

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart); // 【新增】广播 UART 服务
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); 
  Bluefruit.Advertising.start(0); 
}

void connect_callback(uint16_t conn_handle) {
  Serial.println("Connected!");

  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  // 如果还没有绑定过，这会触发手机弹出配对窗口。
  // 如果已经绑定过，这会加密链路。
  if (!connection->bonded()) {
    Serial.println("Requesting Pairing...");
    connection->requestPairing();
  } else {
    Serial.println("Already Bonded, waiting for security...");
    // 如果已经绑定，会自动进入加密状态，我们在 connection_secured_callback 里处理
  }

  if (ctsSvc.discover(conn_handle)) {
    if (ctcChar.discover()) {
      ctcChar.enableNotify(); 
      cts_time_t timeData;
      if (ctcChar.read(&timeData, sizeof(timeData)) >= 7) {
         setTime(timeData.hour, timeData.minute, timeData.second, 
                 timeData.day, timeData.month, timeData.year);
         isFirstUpdate = true;
         updateClockDisplay(); 
      }
    }
  }
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Disconnected");
}

void notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len >= 7) {
    cts_time_t* t = (cts_time_t*)data;
    setTime(t->hour, t->minute, t->second, t->day, t->month, t->year);
  }
}

void pair_callback(uint16_t conn_handle, uint8_t auth_status) {
  if (auth_status == BLE_GAP_SEC_STATUS_SUCCESS) Serial.println("Paired");
}