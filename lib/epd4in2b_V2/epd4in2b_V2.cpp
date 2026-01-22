/**
 *  @filename   :   epd4in2b_V2.cpp
 *  @brief      :   Implements for Dual-color e-paper library
 *  @author     :	Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     Nov 25 2020
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include "epd4in2b_V2.h"

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
};

int Epd::Init(void) {
    // 1. 硬件接口初始化
    if (IfInit() != 0) {
        return -1;
    }
    
    // 2. 硬件复位
    Reset();
    WaitUntilIdle();

    // 3. 软件复位
    SendCommand(0x12); 
    WaitUntilIdle();


    // 4. 驱动输出控制 (Driver Output Control)
    SendCommand(0x00);
    SendData(0x13);

    SendCommand(0x01);
    SendData(0x2B); // (300-1)%256
    SendData(0x01); // (300-1)/256
    SendData(0x01);

    // 5. 数据进入模式 (Data Entry Mode)
    // 0x01: X递增, Y递减 (符合一般绘图习惯)
    SendCommand(0x11);
    SendData(0x01);

    // 6. 设置 RAM X 地址窗口
    SendCommand(0x44); 
    SendData(0x00);
    SendData(0x31); // 400/8 - 1 = 49 (0x31)
    
    // 7. 设置 RAM Y 地址窗口
    SendCommand(0x45);
    SendData(0x2B); // 299 (0x12B)
    SendData(0x01);
    SendData(0x00);
    SendData(0x00); 

    // 8. 边框波形 (Border Waveform)
    SendCommand(0x3C);
    SendData(0x05); // 0x05通常为白色边框

    // 9. 显示更新控制 (Display Update Control)
    SendCommand(0x21);
    SendData(0x00);
    SendData(0x80);

    // 10. 内置温度传感器
    SendCommand(0x18);
    SendData(0x80);

    // 11. 设置初始坐标
    SendCommand(0x4E); 
    SendData(0x00);
    SendCommand(0x4F); 
    SendData(0x2B);
    SendData(0x01);
    
    WaitUntilIdle();
    
    // 【重要】删除了 Lut() 调用，使用屏幕内置 OTP 波形
    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the busy_pin goes HIGH
 */
void Epd::WaitUntilIdle(void) {
    while(DigitalRead(busy_pin) == 1) {      //0: busy, 1: idle
        DelayMs(100);
    }      
}

/**
 *  @brief: module reset. 
 *          often used to awaken the module in deep sleep, 
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);   
    DigitalWrite(reset_pin, LOW);
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);   
}

/**
 *  @brief: transmit partial data to the SRAM
 */
void Epd::SetPartialWindow(const unsigned char* buffer_black, const unsigned char* buffer_red, int x, int y, int w, int l) {
    SendCommand(PARTIAL_IN);
    SendCommand(PARTIAL_WINDOW);
    SendData(x >> 8);
    SendData(x & 0xf8);     // x should be the multiple of 8, the last 3 bit will always be ignored
    SendData(((x & 0xf8) + w  - 1) >> 8);
    SendData(((x & 0xf8) + w  - 1) | 0x07);
    SendData(y >> 8);        
    SendData(y & 0xff);
    SendData((y + l - 1) >> 8);        
    SendData((y + l - 1) & 0xff);
    SendData(0x01);         // Gates scan both inside and outside of the partial window. (default) 
    DelayMs(2);
    SendCommand(DATA_START_TRANSMISSION_1);
    if (buffer_black != NULL) {
        for(int i = 0; i < w  / 8 * l; i++) {
            SendData(buffer_black[i]);  
        }  
    }
    DelayMs(2);
    SendCommand(DATA_START_TRANSMISSION_2);
    if (buffer_red != NULL) {
        for(int i = 0; i < w  / 8 * l; i++) {
            SendData(buffer_red[i]);  
        }  
    }
    DelayMs(2);
    SendCommand(PARTIAL_OUT);  
}

/**
 *  @brief: transmit partial data to the black part of SRAM
 */
void Epd::SetPartialWindowBlack(const unsigned char* buffer_black, int x, int y, int w, int l) {
    SendCommand(PARTIAL_IN);
    SendCommand(PARTIAL_WINDOW);
    SendData(x >> 8);
    SendData(x & 0xf8);     // x should be the multiple of 8, the last 3 bit will always be ignored
    SendData(((x & 0xf8) + w  - 1) >> 8);
    SendData(((x & 0xf8) + w  - 1) | 0x07);
    SendData(y >> 8);        
    SendData(y & 0xff);
    SendData((y + l - 1) >> 8);        
    SendData((y + l - 1) & 0xff);
    SendData(0x01);         // Gates scan both inside and outside of the partial window. (default) 
    DelayMs(2);
    SendCommand(DATA_START_TRANSMISSION_1);
    if (buffer_black != NULL) {
        for(int i = 0; i < w  / 8 * l; i++) {
            SendData(buffer_black[i]);  
        }  
    }
    DelayMs(2);
    SendCommand(PARTIAL_OUT);  
}

/**
 *  @brief: transmit partial data to the red part of SRAM
 */
void Epd::SetPartialWindowRed(const unsigned char* buffer_red, int x, int y, int w, int l) {
    SendCommand(PARTIAL_IN);
    SendCommand(PARTIAL_WINDOW);
    SendData(x >> 8);
    SendData(x & 0xf8);     // x should be the multiple of 8, the last 3 bit will always be ignored
    SendData(((x & 0xf8) + w  - 1) >> 8);
    SendData(((x & 0xf8) + w  - 1) | 0x07);
    SendData(y >> 8);        
    SendData(y & 0xff);
    SendData((y + l - 1) >> 8);        
    SendData((y + l - 1) & 0xff);
    SendData(0x01);         // Gates scan both inside and outside of the partial window. (default) 
    DelayMs(2);
    SendCommand(DATA_START_TRANSMISSION_2);
    if (buffer_red != NULL) {
        for(int i = 0; i < w  / 8 * l; i++) {
            SendData(buffer_red[i]);  
        }  
    }
    DelayMs(2);
    SendCommand(PARTIAL_OUT);  
}

/**
 * @brief: refresh and displays the frame
 * Adapted for SSD1683 (HINK-E42A48-A1)
 */
void Epd::DisplayFrame(const unsigned char* frame_black, const unsigned char* frame_red) {
    // --- 关键修正：写黑白数据前，先重置光标到起点 ---
    SendCommand(0x4E); // Set RAM X Address Counter
    SendData(0x00);
    SendCommand(0x4F); // Set RAM Y Address Counter
    SendData(0x2B);    // Y = 299 (0x12B) 低位
    SendData(0x01);    // Y = 299 (0x12B) 高位
    
    // 1. 写黑白数据 (Write RAM BW)
    if (frame_black != NULL) {
        SendCommand(0x24);
        for (int i = 0; i < 15000; i++) {
            SendData(pgm_read_byte(&frame_black[i]));
        }
    } else {
        // 如果传入NULL，也建议填白，防止花屏
        SendCommand(0x24);
        for (int i = 0; i < 15000; i++) SendData(0xFF);
    }

    // --- 写红色数据前，再次重置光标到起点 ---
    SendCommand(0x4E); 
    SendData(0x00);
    SendCommand(0x4F); 
    SendData(0x2B); 
    SendData(0x01);

    // 2. 写红色数据 (Write RAM Red)
    if (frame_red != NULL) {
        SendCommand(0x26);
        for (int i = 0; i < 15000; i++) {
            SendData(pgm_read_byte(&frame_red[i]));
        }
    } else {
        // 如果传入NULL，填无色(0x00)
        SendCommand(0x26);
        for (int i = 0; i < 15000; i++) SendData(0x00);
    }

    // 3. 刷新
    SendCommand(0x22); 
    SendData(0xF7);    
    SendCommand(0x20); 
    WaitUntilIdle();
}

/**
 * @brief: clear the frame data from the SRAM, this won't refresh the display
 */
void Epd::ClearFrame(void) {
    // --- 重置光标 ---
    SendCommand(0x4E); SendData(0x00);
    SendCommand(0x4F); SendData(0x2B); SendData(0x01);

    // 1. 填黑白显存（全白）
    SendCommand(0x24);           
    for(int i = 0; i < 15000; i++) {
        SendData(0xFF);  
    }  

    // --- 重置光标 ---
    SendCommand(0x4E); SendData(0x00);
    SendCommand(0x4F); SendData(0x2B); SendData(0x01);

    // 2. 填红色显存（全透明）
    // 注意：SSD1683 红色通道 0x00 是不显示，如果变红请改回 0xFF
    SendCommand(0x26);           
    for(int i = 0; i < 15000; i++) {
        SendData(0x00);  
    }  

    // 3. 刷新
    SendCommand(0x22); 
    SendData(0xF7);
    SendCommand(0x20); 
    WaitUntilIdle();
}

/**
 * @brief: This displays the frame data from SRAM
 */
void Epd::DisplayFrame(void) {
    SendCommand(DISPLAY_REFRESH); 
    DelayMs(100);
    WaitUntilIdle();
}

/**
 * @brief: After this command is transmitted, the chip would enter the deep-sleep mode to save power. 
 *         The deep sleep mode would return to standby by hardware reset. The only one parameter is a 
 *         check code, the command would be executed if check code = 0xA5. 
 *         You can use Epd::Reset() to awaken and use Epd::Init() to initialize.
 */
void Epd::Sleep() {
    SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
    SendData(0xF7);     // border floating
    SendCommand(POWER_OFF);
    WaitUntilIdle();
    SendCommand(DEEP_SLEEP);
    SendData(0xA5);     // check code
}


/* END OF FILE */
