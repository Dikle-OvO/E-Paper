#include <Arduino.h>
#include "epd4in2b_V2.h"
#include "epdpaint.h"
#include <U8g2_for_Adafruit_GFX.h>
#include "InkCalendar.h"

// ================== 全局对象 ==================
unsigned char imageBlack[15000]; // 视具体RAM大小调整，ESP32通常够用
unsigned char imageRed[15000];

Epd epd;
Paint paintBlack(imageBlack, EPD_WIDTH, EPD_HEIGHT);
Paint paintRed(imageRed, EPD_WIDTH, EPD_HEIGHT);
extern const unsigned char bmp_data2[]; // 来自 test.c

// ================== 主程序 ==================

void setup() {
    Serial.begin(115200);
    
    // 1. 初始化屏幕
    if (epd.Init() != 0) {
        Serial.print("e-Paper init failed");
        return;
    }

    // 2. 清空缓冲区
    // 1=White, 0=Black/Red
    paintBlack.Clear(1); 
    paintRed.Clear(0);   

    int centared_x = (EPD_WIDTH - 240) / 2;
    int centared_y = (EPD_HEIGHT - 241) / 2;
    paintBlack.DrawImage(centared_x, centared_y, 240, 241, bmp_data2, 0);
    epd.DisplayFrame(imageBlack, NULL);
    
    delay(5000);
    paintBlack.Clear(1); 
    // 3. 绘制日历内容
    calendarInit();

    // 4. 推送显示
    Serial.println("Displaying frame...");
    epd.DisplayFrame(imageBlack, imageRed);
    
    // 5. 休眠
    epd.Sleep();
}

void loop() {
    // 静态显示，无需 Loop
}