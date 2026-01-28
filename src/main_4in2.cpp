#include <Arduino.h>
#include "epd4in2b_V2.h"
#include "epdpaint.h"
#include <U8g2_for_Adafruit_GFX.h>

#define EPD_WIDTH   400
#define EPD_HEIGHT  300

// 日历相关配置
#define CAL_TITLE_FONT    u8g2_font_wqy16_t_gb2312  // 年月标题字体
#define CAL_WEEK_FONT     u8g2_font_wqy16_t_gb2312  // 星期字体
#define CAL_DAY_FONT      u8g2_font_fub17_tn  // 日期字体
#define TODAY_DAY         28                        // 测试用今日日期（可替换为真实时间）
#define CURRENT_MONTH     1                        // 当前月份
#define CURRENT_YEAR      2026                     // 当前年份

// 星期文字定义
const char* weekText[7] = { "日", "一", "二", "三", "四", "五", "六" };

// 这是一个“桥梁”类
class Paint_GFX_Wrapper : public Adafruit_GFX {
public:
    Paint* _paint; // 指向你原本的 Paint 对象

    // 构造函数：初始化父类 Adafruit_GFX，并保存 Paint 指针
    Paint_GFX_Wrapper(Paint* paint, int16_t w, int16_t h) : Adafruit_GFX(w, h), _paint(paint) {}

    // 必须实现这个虚函数
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        _paint->DrawPixel(x, y, color); 
    }
};

// 全局变量（保留原有结构）
unsigned char imageBlack[15000];
unsigned char imageRed[15000];
Epd epd;
Paint paintBlack(imageBlack, EPD_WIDTH, EPD_HEIGHT);
Paint paintRed(imageRed, EPD_WIDTH, EPD_HEIGHT);
Paint_GFX_Wrapper gfx_black(&paintBlack, EPD_WIDTH, EPD_HEIGHT);
Paint_GFX_Wrapper gfx_red(&paintRed, EPD_WIDTH, EPD_HEIGHT);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// 获取当月总天数
int getDaysInMonth(int year, int month) {
    // 处理12月的特殊情况
    if (month > 12) {
        month = 1;
        year++;
    }
    
    // 大月
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        return 31;
    }
    // 小月
    else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    // 二月（闰年判断）
    else {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        } else {
            return 28;
        }
    }
}

// 绘制日历标题（年月）
void drawCalendarTitle() {
    u8g2.setFont(CAL_TITLE_FONT);
    u8g2.setFontMode(1);
    u8g2.setForegroundColor(0);
    u8g2.setBackgroundColor(1);
    
    // 拼接年月文本
    char title[20];
    sprintf(title, "%d年%d月", CURRENT_YEAR, CURRENT_MONTH);
    
    // 计算居中坐标
    int16_t titleWidth = u8g2.getUTF8Width(title);
    int16_t x = (EPD_WIDTH - titleWidth) / 2;
    int16_t y = 40;
    
    u8g2.setCursor(x, y);
    u8g2.print(title);
}

// 日历相关配置新增（需添加到全局配置区）
#define CAL_HEADER_Y      70          // 星期栏Y坐标
#define CAL_HEADER_H      25          // 星期栏高度
#define CAL_DAY_W         (EPD_WIDTH / 7)  // 每列宽度（基础值）
#define _week_1st         0           // 星期起始偏移（0=周日开始，1=周一开始）

void drawWeekHeader() {
    // 初始化基础配置
    u8g2.setFont(CAL_WEEK_FONT);
    u8g2.setFontMode(1);          // 透明背景
    u8g2.setFontDirection(0);     // 正常方向
    
    // 计算整体居中偏移（核心：和参考逻辑一致）
    int16_t headerOffsetX = (EPD_WIDTH - 7 * CAL_DAY_W) / 2;
    
    // 遍历绘制7个星期列
    for (int i = 0; i < 7; i++) {
        // 1. 计算当前列的真实星期索引（支持星期起始偏移）
        int realWeekIdx = (i + _week_1st) % 7;
        
        // 2. 确定是否为周末（日/六）
        bool isWeekend = (realWeekIdx == 0 || realWeekIdx == 6);
        
        // 3. 计算背景框坐标
        int16_t boxX1 = headerOffsetX + i * CAL_DAY_W;
        int16_t boxY1 = CAL_HEADER_Y;
        int16_t boxX2 = boxX1 + CAL_DAY_W;
        int16_t boxY2 = CAL_HEADER_Y - CAL_HEADER_H;
        
        // 4. 绘制星期栏背景（完全对齐你的DrawFilledRectangle逻辑）
        if (isWeekend) {
            // 周末：红色背景（paintRed绘制，1=红色）
            paintRed.DrawFilledRectangle(boxX1, boxY1, boxX2, boxY2, 1);
        } else {
            // 工作日：黑色背景（paintBlack绘制，0=黑色）
            paintBlack.DrawFilledRectangle(boxX1, boxY1, boxX2, boxY2, 0);
        }
        
        // 5. 计算文字坐标（修正Y轴方向，和日期文字逻辑一致）
        int16_t textWidth = u8g2.getUTF8Width(weekText[realWeekIdx]);
        int16_t textX = boxX1 + (CAL_DAY_W - textWidth) / 2;
        int16_t textY = CAL_HEADER_Y-(CAL_HEADER_H - textWidth) / 2 ; 
        
        // 5. 绘制星期文字（根据背景色切换图层）
        if (isWeekend) {
            // 周末：红色背景+白色文字（切换到红色图层）
            u8g2.begin(gfx_red);
            u8g2.setFont(CAL_WEEK_FONT);
            u8g2.setFontMode(1);
            u8g2.setForegroundColor(0);
            u8g2.setBackgroundColor(1);
        } else {
            // 普通：黑色背景+黑色文字（黑色图层）
            u8g2.begin(gfx_black);
            u8g2.setFont(CAL_WEEK_FONT);
            u8g2.setFontMode(1);
            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);
        }
        u8g2.setCursor(textX, textY);
        u8g2.print(weekText[realWeekIdx]);
    
        
    }
    
    // 绘制完成后切回黑色图层，避免影响后续绘制
    u8g2.begin(gfx_black);
    u8g2.setFont(CAL_WEEK_FONT);
    u8g2.setFontMode(1);
    u8g2.setForegroundColor(0);
    u8g2.setBackgroundColor(1);
}

// 绘制日期网格
void drawCalendarDays() {
    u8g2.setFont(CAL_DAY_FONT);
    u8g2.setFontMode(1);
    u8g2.setForegroundColor(0);
    u8g2.setBackgroundColor(1);
    
    int colWidth = EPD_WIDTH / 7;   // 每列宽度
    int rowHeight = 40;             // 每行高度
    int startY = 100;               // 日期区域起始Y坐标
    int daysInMonth = getDaysInMonth(CURRENT_YEAR, CURRENT_MONTH);
    
    // 模拟当月第一天是周三（可替换为真实计算）
    int firstDayWeek = 4;  // 0=周日, 1=周一...6=周六
    
    // 绘制日期
    for (int day = 1; day <= daysInMonth; day++) {
        // 计算日期所在行列
        int dayIndex = firstDayWeek + day - 1;
        int col = dayIndex % 7;
        int row = dayIndex / 7;
        
        // 计算绘制坐标
        char dayStr[3];
        sprintf(dayStr, "%d", day);
        int16_t textWidth = u8g2.getUTF8Width(dayStr);
        int16_t x = col * colWidth + (colWidth - textWidth) / 2;
        int16_t y = startY + row * rowHeight;
        
        // 周末日期用红色
        if (col == 0 || col == 6) {
            u8g2.begin(gfx_red);
            u8g2.setFont(CAL_DAY_FONT);
            u8g2.setFontMode(1);
            u8g2.setForegroundColor(1);
            u8g2.setBackgroundColor(0);
            u8g2.setCursor(x, y);
            u8g2.print(dayStr);
            u8g2.begin(gfx_black);
            u8g2.setFont(CAL_DAY_FONT);
            u8g2.setFontMode(1);
            u8g2.setForegroundColor(0);
            u8g2.setBackgroundColor(1);
        } 
        // 今日日期特殊标注
        else if (day == TODAY_DAY) {
            // 先画红色背景框
            paintRed.DrawFilledRectangle(x, y, x+25, y-18, 1);
            // 白色文字
            u8g2.begin(gfx_red);
            u8g2.setForegroundColor(0);
            u8g2.setCursor(x, y);
            u8g2.print(dayStr);
        }
        // 普通日期
        else {
            u8g2.begin(gfx_black);
            u8g2.setForegroundColor(0);
            u8g2.setBackgroundColor(1);
            u8g2.setCursor(x, y);
            u8g2.print(dayStr);
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // 初始化墨水屏
    if (epd.Init() != 0) {
        Serial.println("墨水屏初始化失败！");
        while (1);
    }
    
    // 初始化Paint
    paintBlack.SetRotate(ROTATE_0);
    paintBlack.Clear(1);  // 黑色图层清空为白色
    paintRed.SetRotate(ROTATE_0);
    paintRed.Clear(0);    // 红色图层清空为黑色
    
    // 绑定U8g2到黑色图层
    u8g2.begin(gfx_black);
    
    // 绘制日历
    drawCalendarTitle();   // 绘制年月标题
    drawWeekHeader();      // 绘制星期栏
    drawCalendarDays();    // 绘制日期
    
    // 刷新屏幕（使用黑红双数组）
    epd.DisplayFrame(imageBlack, imageRed);
    
    // 休眠以节省电量
    epd.Sleep();
}

void loop() {
    // 无需循环刷新，墨水屏保持显示
}