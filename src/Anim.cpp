#include "epd2in13_V3.h" 
#include "epdpaint.h"
#include <TimeLib.h>
#include <bluefruit.h>


// --- 全局变量 ---
int prevSecond = -1;  
int refresh_count = 0; 
bool isFirstUpdate = true; 

// --- 颜色定义 ---
#define COLORED     0  // 黑色
#define UNCOLORED   1  // 白色

// --- 对象实例化 ---
Epd epd;
unsigned char image[4000]; 
Paint paint(image, 0, 0); 

// 辅助函数：居中X坐标
int getCenterX(const char* text, sFONT* font) {
    int textWidth = strlen(text) * font->Width;
    int x = (epd.width - textWidth) / 2;
    return (x < 0) ? 0 : x;
}

const char* getWeekStr(uint8_t dayOfWeek) {
    switch(dayOfWeek) {
        case 1: return "SUN";
        case 2: return "MON";
        case 3: return "TUE";
        case 4: return "WED";
        case 5: return "THU";
        case 6: return "FRI";
        case 7: return "SAT";
        default: return "DAY";
    }
}


// --- 核心：极简卡片风格 UI ---
void updateClockDisplay() {
  
  // 1. 画布清白
  paint.Clear(UNCOLORED);

  // ==========================================
  // Header 区域：日期与状态
  // ==========================================
  // 画一条粗线作为 Header 底部分割
  paint.DrawFilledRectangle(0, 30, 122, 33, COLORED);

  char buf[20];
  
  // 左上角：日期 (MM.DD)
  sprintf(buf, "%02d.%02d", month(), day());
  paint.DrawStringAt(5, 8, buf, &Font16, COLORED);

  // 右上角：星期
  const char* weekStr = getWeekStr(weekday());
  // 计算右对齐坐标
  int weekX = epd.width - (strlen(weekStr) * Font16.Width) - 5;
  paint.DrawStringAt(weekX, 8, weekStr, &Font16, COLORED);

  // ==========================================
  // Body 区域：时间卡片
  // ==========================================
  
  // 卡片 1：小时 (黑色实心块)
  // 位置：X:10, Y:45, W:102, H:60
  paint.DrawFilledRectangle(10, 45, 112, 105, COLORED);
  
  // 小时数字 (白色反色)
  sprintf(buf, "%02d", hour());
  // 居中计算
  int hX = (122 - (2 * Font24.Width)) / 2;
  // 伪粗体 + 居中
  paint.DrawStringAt(hX, 63, buf, &Font24, UNCOLORED);
  paint.DrawStringAt(hX+1, 63, buf, &Font24, UNCOLORED);

  // 装饰：在小时卡片左上角加个小标签
  paint.DrawStringAt(15, 50, "H", &Font12, UNCOLORED);


  // 卡片 2：分钟 (黑色线框)
  // 位置：X:10, Y:115, W:102, H:60
  // 画两个矩形形成框，或者直接用 DrawRectangle (它是空心的)
  // 为了框粗一点，我们画两层
  paint.DrawRectangle(10, 115, 112, 175, COLORED);
  paint.DrawRectangle(11, 116, 111, 174, COLORED); // 内缩1像素再画一次

  // 分钟数字 (黑色正色)
  sprintf(buf, "%02d", minute());
  // 居中计算
  int mX = (122 - (2 * Font24.Width)) / 2;
  paint.DrawStringAt(mX, 133, buf, &Font24, COLORED);
  paint.DrawStringAt(mX+1, 133, buf, &Font24, COLORED);

  // 装饰：在分钟卡片左上角加个小标签
  paint.DrawStringAt(15, 120, "M", &Font12, COLORED);

  // ==========================================
  // Footer 区域：秒与蓝牙
  // ==========================================
  
  // 秒 (底部大号数字，无框，极简)
  sprintf(buf, "%02d", second());
  int sX = (122 - (2 * Font24.Width)) / 2;
  paint.DrawStringAt(sX, 195, buf, &Font24, COLORED);
  
  // 秒下面的小横线装饰
  paint.DrawHorizontalLine(sX, 220, 2 * Font24.Width, COLORED);

  // 底部蓝牙状态小字
  if (Bluefruit.connected()) {
    paint.DrawStringAt(getCenterX("BLUETOOTH", &Font12), 235, "BLUETOOTH", &Font12, COLORED);
  } else {
    paint.DrawStringAt(getCenterX("...", &Font12), 235, "...", &Font12, COLORED);
  }

  // ==========================================
  // 刷新策略
  // 每 10 分钟 (600秒) 全刷，或者 00分00秒  整时全刷
  if (isFirstUpdate || refresh_count >= 600 || (minute() == 0 && second() == 0)) {
      Serial.println(">> FULL Refresh");
      epd.Init(FULL);       
      epd.Display(image);   
      epd.Init(PART);       
      refresh_count = 0;
      isFirstUpdate = false; 
  } else {
      epd.Init(PART);       
      epd.DisplayPart(image); 
      refresh_count++;
  }
}

void setupDisplay() {

  paint.Clear(UNCOLORED);

  for(int i = 0; i < 3; i++) {
    // paint.DrawFilledRectangle(0, 10+i*20, 122, 10+i*20+5, COLORED);
  }
  
// 大黑色矩形背景
  paint.DrawFilledRectangle(10, 80, 112, 170, COLORED);

 

  const char* t1 = "E-PAPER";

  const char* t2 = "CLOCK";

 

  paint.DrawStringAt(getCenterX(t1, &Font20), 100, t1, &Font20, UNCOLORED);

  paint.DrawStringAt(getCenterX(t2, &Font24), 130, t2, &Font24, UNCOLORED);

//   paint.DrawChinese(10, 10, 0, &chinese32, COLORED);
//   paint.DrawChinese(45, 10, 1, &chinese32, COLORED);
//   paint.DrawChinese(80, 10, 2, &chinese32, COLORED);

//   paint.DrawChinese(18, 10, 0, &chinese16, COLORED);

  const char* t3 = "Waiting BLE...";

  paint.DrawStringAt(getCenterX(t3, &Font12), 200, t3, &Font12, COLORED);

 

  epd.Display(image);

}