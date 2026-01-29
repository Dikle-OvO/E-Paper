#include <Arduino.h>
#include "epd4in2b_V2.h"
#include "epdpaint.h"
#include <U8g2_for_Adafruit_GFX.h>

// ================== 屏幕参数定义 ==================
#define EPD_WIDTH   400
#define EPD_HEIGHT  300

// ================== 终末地风格配置 ==================
// 时间设置 (在此处修改测试时间)
#define CURRENT_YEAR      2026
#define CURRENT_MONTH     1
#define TODAY_DAY         29

// 布局参数
#define MARGIN_X          15
#define MARGIN_Y          15
#define HEADER_HEIGHT     90    // 头部区域高度
#define CELL_W            ((EPD_WIDTH - 2 * MARGIN_X) / 7) // 单元格宽度
#define CELL_H            32    // 单元格高度
#define GRID_START_Y      105   // 日期网格起始Y

// 字体定义 (确保U8g2库包含这些字体，否则可替换为类似大小的字体)
// 用于巨大的月份数字
#define FONT_MEGA_NUM     u8g2_font_logisoso42_tn 
// 用于标题英文/年份 (Helvetica Bold)
#define FONT_TECH_BOLD    u8g2_font_helvB10_tf    
// 用于装饰性小字 (极小)
#define FONT_TECH_TINY    u8g2_font_u8glib_4_tf   
// 用于日期/星期 (中等无衬线)
#define FONT_MAIN         u8g2_font_helvB10_tf    
// 用于日期数字
#define FONT_DAY_NUM      u8g2_font_helvB12_tn    
// 用于日期数字
#define Chinese_FONT      u8g2_font_wqy15_t_gb2312a 

// ================== 类定义 ==================

// 桥梁类：连接 Paint 和 U8g2
class Paint_GFX_Wrapper : public Adafruit_GFX {
public:
    Paint* _paint;
    Paint_GFX_Wrapper(Paint* paint, int16_t w, int16_t h) : Adafruit_GFX(w, h), _paint(paint) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        _paint->DrawPixel(x, y, color); 
    }
};

// ================== 全局对象 ==================
unsigned char imageBlack[15000]; // 视具体RAM大小调整，ESP32通常够用
unsigned char imageRed[15000];

Epd epd;
Paint paintBlack(imageBlack, EPD_WIDTH, EPD_HEIGHT);
Paint paintRed(imageRed, EPD_WIDTH, EPD_HEIGHT);
Paint_GFX_Wrapper gfx_black(&paintBlack, EPD_WIDTH, EPD_HEIGHT);
Paint_GFX_Wrapper gfx_red(&paintRed, EPD_WIDTH, EPD_HEIGHT);
U8G2_FOR_ADAFRUIT_GFX u8g2;

// ================== 辅助工具函数 ==================

// 获取某月天数
int getDaysInMonth(int year, int month) {
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
        else return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

// 获取某年某月1号是星期几 (0=周日, 1=周一 ...)
// 使用蔡勒公式
int getFirstDayOfWeek(int year, int month) {
    if (month < 3) { month += 12; year -= 1; }
    int K = year % 100;
    int J = year / 100;
    int f = 1 + 13 * (month + 1) / 5 + K + K / 4 + J / 4 + 5 * J;
    return (f + 6) % 7; 
}

// 绘制虚线 (Tech风格核心元素)
// color: 0=黑, 1=红 (对应Paint的颜色定义)
void drawDottedLine(Paint &p, int x1, int y1, int x2, int y2, int color) {
    int length = (x1 == x2) ? abs(y2 - y1) : abs(x2 - x1);
    bool vertical = (x1 == x2);
    
    for (int i = 0; i < length; i += 4) { // 虚线间隔4像素
        if (vertical) p.DrawPixel(x1, min(y1, y2) + i, color);
        else p.DrawPixel(min(x1, x2) + i, y1, color);
    }
}

// 绘制角标 (Corner Brackets)
void drawBracket(int x, int y, int w, int h, int len) {
    paintBlack.DrawHorizontalLine(x, y, len, 0);      
    paintBlack.DrawVerticalLine(x, y, len, 0);       
    paintBlack.DrawHorizontalLine(x + w - len, y + h, len, 0); 
    paintBlack.DrawVerticalLine(x + w, y + h - len, len, 0); 
}

// ================== UI 绘制模块 ==================

// 1. 绘制头部信息
void drawHeader() {
    // A. 巨大的月份数字 (左上角)
    u8g2.begin(gfx_black);
    u8g2.setFont(FONT_MEGA_NUM);
    u8g2.setForegroundColor(0);
    u8g2.setBackgroundColor(1);
    u8g2.setFontMode(1); // 透明背景
    
    char monthStr[5];
    sprintf(monthStr, "%02d", CURRENT_MONTH); 
    u8g2.setCursor(MARGIN_X, 65);
    u8g2.print(monthStr);

    u8g2.setForegroundColor(0);
    u8g2.setBackgroundColor(1);
    // B. 年份与系统文字 (紧挨月份)
    u8g2.setFont(FONT_TECH_BOLD);
    u8g2.setCursor(MARGIN_X + 75, 45);
    u8g2.print(CURRENT_YEAR);

    u8g2.setFont(FONT_TECH_TINY);
    u8g2.setCursor(MARGIN_X + 75, 60);
    u8g2.print("// SYS.CALENDAR.DAEMON_V2");
    u8g2.setCursor(MARGIN_X + 75, 68);
    u8g2.print("STATUS: ONLINE [SYNCED]");

    // C. 顶部装饰条 (右上角)
    // 黑色实心条 + 红色警告块
    int barX = EPD_WIDTH - 120;
    paintBlack.DrawFilledRectangle(barX, 20, EPD_WIDTH - MARGIN_X, 28, 0);
    // 红色的一小段
    paintRed.DrawFilledRectangle(EPD_WIDTH - 40, 20, EPD_WIDTH - MARGIN_X, 28, 1);
    
    // 装饰条下方的文字
    u8g2.setFont(FONT_TECH_TINY);
    u8g2.setCursor(barX, 38);
    u8g2.print("ENERGY_LVL");
    u8g2.setCursor(EPD_WIDTH - 50, 38);
    u8g2.print("100%");

    // D. 主分割线 (由实线、虚线、红线组成)
    int lineY = 80;
    // 左侧实线
    paintBlack.DrawFilledRectangle(0, lineY, 200, lineY + 2, 0);
    // 中间红线
    paintRed.DrawHorizontalLine(200, lineY + 1, 100,  1);
    // 右侧虚线
    drawDottedLine(paintBlack, 305, lineY + 1, EPD_WIDTH, lineY + 1, 0);
}

// 2. 绘制星期栏 (反白样式)
void drawWeekHeader() {
    int barY = GRID_START_Y - 18;
    
    // 黑色背景条
    paintBlack.DrawFilledRectangle(MARGIN_X, barY, EPD_WIDTH - MARGIN_X, barY + 18, 0);
    
    // 英文缩写
    const char* weekEn[7] = { "日", "一", "二", "三", "四", "五", "六" };
    
    u8g2.begin(gfx_black);
    u8g2.setFont(Chinese_FONT); // 使用极小字体或粗体
    u8g2.setFontMode(0);          // 不透明模式 (打底)
    u8g2.setForegroundColor(1);         // 设置绘制色为白
    u8g2.setBackgroundColor(0);   // 设置背景色为黑

    for (int i = 0; i < 7; i++) {
        // 简单居中
        int x = MARGIN_X + i * CELL_W;
        int txtX = x + (CELL_W - 10) / 2; // 估算居中
        u8g2.setCursor(txtX, barY + 15);
        u8g2.print(weekEn[i]);
    }
    
    // 恢复默认设置
    u8g2.setForegroundColor(1);
    u8g2.setBackgroundColor(0);
}

// 3. 绘制日期网格 (极简 + 装饰)
void drawGrid() {
    u8g2.setFont(FONT_DAY_NUM);
    u8g2.setFontMode(1);

    int days = getDaysInMonth(CURRENT_YEAR, CURRENT_MONTH);
    int startDayWeek = getFirstDayOfWeek(CURRENT_YEAR, CURRENT_MONTH);

    for (int i = 1; i <= days; i++) {
        int idx = startDayWeek + i - 1;
        int row = idx / 7;
        int col = idx % 7;

        int x = MARGIN_X + col * CELL_W;
        int y = GRID_START_Y + row * CELL_H;

        // 绘制辅助竖线 (刻度感)
        // paintBlack.DrawVerticalLine(x, y, 8, 0);

        // 文字居中计算
        char numStr[3];
        sprintf(numStr, "%d", i);
        int strW = u8g2.getUTF8Width(numStr);
        int txtX = x + (CELL_W - strW) / 2;
        int txtY = y + 22;

        u8g2.setForegroundColor(0); // 黑字
        u8g2.setBackgroundColor(1);

        // --- 特殊日期逻辑 ---
        
        if (i == TODAY_DAY) {
            // [今日]: 红色实心块 + 延伸线条
            
            // 1. 红色背景块
            paintRed.DrawFilledRectangle(x + 5, y + 4, x + CELL_W - 5, y + 26, 1);
            
            // 2. 装饰线 (连接到左侧)
            paintBlack.DrawLine(x + 5, y + 15, x - 2, y + 15, 0);
            
            // 3. 写字
            // 这里采用：黑色实心块(反白字) + 红色下划线，这是比较稳妥的显示方式
            // paintBlack.DrawFilledRectangle(x + 5, y + 4, x + CELL_W - 5, y + 26, 0);
            
            u8g2.begin(gfx_red);
            u8g2.setForegroundColor(0); // 白
            u8g2.setCursor(txtX, txtY);
            u8g2.print(numStr);
            u8g2.setForegroundColor(0); // 恢复黑字
            u8g2.setBackgroundColor(1);

            // 加上红色角标表示"Active"
            // paintRed.DrawFilledRectangle(x + CELL_W - 10, y + 4, x + CELL_W - 5, y + 9, 1);

        } else if (col == 0 || col == 6) {
            // [周末]: 黑色字 + 红色小三角
            u8g2.begin(gfx_black);
            u8g2.setCursor(txtX, txtY);
            u8g2.print(numStr);
            
            // 右下角红色斜线
            paintRed.DrawHorizontalLine(x + CELL_W - 8, y + 24, 4, 1);
            paintRed.DrawLine(x + CELL_W - 6, y + 22, x + CELL_W - 4, y + 24, 1);
            
        } else {
            // [普通]: 纯黑字
            u8g2.begin(gfx_black);
            u8g2.setCursor(txtX, txtY);
            u8g2.print(numStr);
        }
    }
}

// 4. 底部装饰 (Footer)
void drawFooter() {
    int footerY = EPD_HEIGHT - 20;
    
    // 底部左侧小字
    u8g2.begin(gfx_black);
    u8g2.setFont(FONT_TECH_TINY);
    u8g2.setCursor(MARGIN_X, footerY);
    u8g2.print("TERMINAL_ID: 0x4A2F");
    
    // 底部右侧装饰码
    u8g2.setCursor(EPD_WIDTH - 120, footerY);
    u8g2.print("NO.77 // SECTOR_09");
    
    // 底部红线
    paintRed.DrawHorizontalLine(MARGIN_X, footerY + 5, EPD_WIDTH - 2 * MARGIN_X, 1);
}

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

    // 3. 绘制各个模块
    drawHeader();
    drawWeekHeader();
    drawGrid();
    drawFooter();
    
    // 4. 整体装饰：加上外围的大括号
    drawBracket(5, 5, EPD_WIDTH - 10, EPD_HEIGHT - 10, 20);

    // 5. 推送显示
    Serial.println("Displaying frame...");
    epd.DisplayFrame(imageBlack, imageRed);
    
    // 6. 休眠
    epd.Sleep();
}

void loop() {
    // 静态显示，无需 Loop
}