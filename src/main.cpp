#include <Arduino.h>
#include <SPI.h>
// 请确保引用的头文件名和你修改后的驱动文件名一致
#include "epd4in2b_V2.h" 
#include "epdpaint.h"

// --- 4.2寸屏幕参数 ---
// 分辨率：400 x 300
// 显存计算：400 * 300 / 8 = 15000 字节
#define EPD_WIDTH   400
#define EPD_HEIGHT  300

// 定义双缓冲区 (nRF52840 RAM 充足，可以放心开)
unsigned char imageBlack[15000];
unsigned char imageRed[15000];

// 创建对象
Epd epd;
Paint paintBlack(imageBlack, EPD_WIDTH, EPD_HEIGHT);
Paint paintRed(imageRed, EPD_WIDTH, EPD_HEIGHT);

extern const unsigned char bmp_data[]; 
extern const unsigned char bmp_data2[]; // 来自 imagedata.h

void setup() {
  Serial.begin(115200);
  // while(!Serial) delay(10); // 如果需要串口调试，可以取消注释等待连接
  // delay(1000); 
  Serial.println("=== 4.2inch SSD1683 Test Start ===");

  // 1. 初始化屏幕
  // 如果 Init 失败（返回非0），通常是接线问题或 BUSY 引脚状态判断反了
  if (epd.Init() != 0) {
      Serial.println("EPD Init Failed! Check wiring & BUSY pin logic.");
      while(1) delay(100); // 死循环卡住
  }
  Serial.println("EPD Init Success.");

  paintBlack.SetRotate(ROTATE_90);
  paintRed.SetRotate(ROTATE_90);

  // 2. 清屏 (刷白)
  // 这一步很重要，用于清除之前的残影，并测试 Clear 函数是否正常
  Serial.println("Clearing Screen...");
  epd.ClearFrame(); 
  // delay(500);

  // 3. 绘制黑色内容
  Serial.println("Drawing Black Image...");
  paintBlack.Clear(1); // 这里的 UNCOLORED 代表白色(0xFF)
  
  // 画一些图形和文字
  // paintBlack.DrawStringAt(10, 10, "Hello! 4.2 E-Paper", &Font24, 0);
  // paintBlack.DrawStringAt(10, 40, "SSD1683 Driver Test", &Font20, 0);
  // paintBlack.DrawRectangle(2, 2, 398, 298, 0); // 外边框
  // paintBlack.DrawLine(0, 0, 400, 300, 0);      // 对角线
  paintBlack.DrawStringAt(0, 0, "Rei Ayanami", &Font16, 0);
  paintBlack.DrawFilledRectangle(0, 18, 300, 20, 0);      // 
  paintBlack.DrawImage(0, 20, 300, 380, bmp_data, 0); 

  // 4. 绘制红色内容
  Serial.println("Drawing Red Image...");
  paintRed.Clear(0); // 这里 UNCOLORED 代表透明/无色(0x00 或 0xFF，取决于驱动实现)
  
  // // 画个红色的实心圆和文字
  paintRed.DrawStringAt(0, 0, "Rei Ayanami", &Font16, 1);
  paintRed.DrawImage(20, 280, 100, 100, bmp_data2, 1);
  // paintRed.DrawFilledCircle(200, 150, 50, 1);
  // paintRed.DrawStringAt(10, 80, "Red Color", &Font20, 1);
  // paintRed.DrawImage(250, 200, 112, 155, bmp_data, 0); 

  // 5. 发送数据并刷新
  // 这一步调用的是你刚刚修改过的 DisplayFrame
  Serial.println("Displaying Frame...");
  epd.DisplayFrame(imageBlack, imageRed);
  
  // 6. 进入休眠
  // 刷新完成后必须休眠，否则 SSD1683 芯片会持续发热并损坏屏幕
  Serial.println("Going to Sleep...");
  epd.Sleep();
  
  Serial.println("Test Done.");
}

void loop() {
  // 墨水屏不需要在 loop 里频繁刷新
  // 闪烁一下板载 LED 表示程序还在运行
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(1000);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(1000);
}