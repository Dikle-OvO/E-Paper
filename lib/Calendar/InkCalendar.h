#ifndef InkCalendar_H
#define InkCalendar_H

// ================== 屏幕参数定义 ==================
#define EPD_WIDTH   400
#define EPD_HEIGHT  300

// ================== 风格配置 ==================
// 时间设置 (在此处修改测试时间)
#define CURRENT_YEAR      2026
#define CURRENT_MONTH     1
#define TODAY_DAY         30

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
// 中文
#define Chinese_FONT      u8g2_font_wqy15_t_gb2312a 

void calendarInit(void); 

#endif
