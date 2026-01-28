#ifndef INK_CALENDAR_H
#define INK_CALENDAR_H

#include <Arduino.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <TimeLib.h>   // 建议使用 TimeLib 处理时间，或者直接用 struct tm
#include <ctime>       // 提供 struct tm 的完整定义
// #include "holiday.h" // 如果需要离线节假日，需要保留这个并在cpp中处理

// 定义显示用的字体引用
#define FONT_TEXT u8g2_font_wqy16_t_gb2312 
#define FONT_SUB u8g2_font_wqy12_t_gb2312 
#define FONT_NUM u8g2_font_fub25_tn

// --- 数据结构定义 ---

// 1. 配置参数 (不会经常变的)
struct CalendarConfig {
    int weekStartDay = 0;   // 0=周日, 1=周一
    bool rotate = false;    // 是否旋转
    String tagDaysStr;      // 特殊标记日期字符串
    // 可以添加更多配置，如是否显示农历等
};

// 2. 运行时数据 (每次刷新可能变的)
struct CalendarData {
    struct tm currentTime;  // 当前时间 (年月日时分秒)
    int voltage = 0;        // 电池电压 (mV)，不需要显示传0
    
    // 如果有本地温湿度传感器 (DHT11/SHT30)，填入这里
    bool hasSensorData = false; 
    float temperature = 0.0;
    float humidity = 0.0;
    
    // 倒数日数据
    String cdLabel;
    String cdTargetDate; // 格式 YYYYMMDD
};

class InkCalendar {
public:
    // 构造函数：传入显示驱动对象的指针
    InkCalendar(U8G2_FOR_ADAFRUIT_GFX *u8g2);

    // 初始化
    void begin();

    // 更新配置
    void setConfig(CalendarConfig config);

    // 核心绘制函数：传入当前数据，绘制一帧
    void draw(const CalendarData &data);

private:
    U8G2_FOR_ADAFRUIT_GFX *u8g2;
    CalendarConfig _config;
    CalendarData _currentData;

    // 内部布局状态
    struct Layout {
        int16_t topH = 60;
        int16_t headerH = 20;
        int16_t dayW = 56;
        int16_t dayH = 44;
        // ... 其他坐标变量，将原本的 calLayout 移到这里
    } _layout;

    // 内部农历/节气缓存
    int _lunarDates[31];
    int _jqAccDate[24];

    // 内部绘制辅助函数
    void drawHeader();
    void drawYearAndMonth();
    void drawDays(); // 核心日历网格
    void drawStatus(); // 电池等
    void drawSensorInfo(); // 替代原来的 Weather
    void drawCountdown();
};

#endif