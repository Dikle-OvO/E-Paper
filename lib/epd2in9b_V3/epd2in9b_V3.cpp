/**
 *  @filename   :   epd2in9b_V3.cpp
 *  @brief      :   Implements for e-paper library
 *  @author     :   Waveshare
 *
 *  Copyright (C) Waveshare     Dec 3 2020
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
#include "epd2in9b_V3.h"
#include "imagedata.h"

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH / 8;
    height = EPD_HEIGHT;
};

int Epd::Init(void) {
    /* 1. 硬件接口初始化 */
    if (IfInit() != 0) {
        return -1;
    }

    /* 2. 硬件复位 */
    Reset();
    
    /* 3. 软件复位 (SWRESET) */
    WaitUntilIdle();
    SendCommand(0x12); 
    WaitUntilIdle();

    /* 4. 驱动输出控制 (Driver Output Control) */
    SendCommand(0x01);
    SendData((EPD_HEIGHT - 1) & 0xFF);
    SendData((EPD_HEIGHT - 1) >> 8);
    SendData(0x00);

    /* 5. 数据进入模式 (Data Entry Mode) */
    // 0x01: X轴递增, Y轴递减 (佳显驱动默认设置)
    SendCommand(0x11);
    SendData(0x01);

    /* 6. 设置 RAM X 地址窗口 (Set Ram-X Address Start/End) */
    // X轴从 0 到 15 (128像素/8 = 16字节)
    SendCommand(0x44);
    SendData(0x00);
    SendData((EPD_WIDTH / 8) - 1);

    /* 7. 设置 RAM Y 地址窗口 (Set Ram-Y Address Start/End) */
    // Y轴从 295 到 0 (因为是Y递减模式)
    SendCommand(0x45);
    SendData((EPD_HEIGHT - 1) & 0xFF);
    SendData((EPD_HEIGHT - 1) >> 8);
    SendData(0x00);
    SendData(0x00);

    /* 8. 设置波形边框 (Border Waveform) */
    SendCommand(0x3C);
    SendData(0x05);

    /* 9. 显示更新控制 (Display Update Control) */
    SendCommand(0x21);
    SendData(0x00);
    SendData(0x80);

    /* 10. 读取内置温度传感器 */
    SendCommand(0x18);
    SendData(0x80);

    /* 11. 设置 RAM X 地址计数器初始值 */
    SendCommand(0x4E);
    SendData(0x00);

    /* 12. 设置 RAM Y 地址计数器初始值 */
    SendCommand(0x4F);
    SendData((EPD_HEIGHT - 1) & 0xFF);
    SendData((EPD_HEIGHT - 1) >> 8);

    WaitUntilIdle();
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
 * @brief: Wait until the busy_pin goes LOW
 * Good Display / SSD1680 Logic: HIGH = Busy, LOW = Idle
 */
void Epd::WaitUntilIdle(void) {
    Serial.println("e-Paper waiting for busy release...");
    // 佳显逻辑：只要是高电平，就是忙，死循环等待
    while(DigitalRead(busy_pin) == HIGH) {      
        DelayMs(10); // 稍作延时，避免死锁 CPU
    }      
    Serial.println("e-Paper busy released!");
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);   
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
}

void Epd::DisplayFrame(const UBYTE *blackimage, const UBYTE *ryimage) {
    // 1. 发送黑白数据 (对应佳显驱动的 Write RAM 0x24)
    SendCommand(0x24);
    for (UWORD j = 0; j < height; j++) {
        for (UWORD i = 0; i < width; i++) {
            SendData(pgm_read_byte(&blackimage[i + (j * width)]));
        }
    }
    
    // 2. 发送红色数据 (对应佳显驱动的 Write RAM 0x26)
    SendCommand(0x26);
    for (UWORD j = 0; j < height; j++) {
        for (UWORD i = 0; i < width; i++) {
            // 注意：佳显官方 Demo 在这里通常会取反(~)，那是针对特定图片格式的。
            // 但因为你使用的是微雪 Paint 库 (0=有色/红色)，
            // 而 SSD1680 芯片也是 (0=红色)，所以这里直接发送即可，不要取反。
            SendData(pgm_read_byte(&ryimage[i + (j * width)]));
        }
    }

    // 3. 执行刷新 (对应佳显驱动的 Update)
    // 必须先配置 Display Update Control 2 (0x22)
    SendCommand(0x22);
    SendData(0xF7);     // 0xF7: 标准全屏刷新 (0xC7 为快刷，但三色屏通常只能全刷)
    
    // 激活刷新 (Master Activation)
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd::Clear(void) {
    // 1. 发送黑白数据 (Write RAM BW)
    // 填充 0xFF 代表白色 (White)
    SendCommand(0x24);
    for (UWORD j = 0; j < height; j++) {
        for (UWORD i = 0; i < width; i++) {
            SendData(0xff);
        }
    }

    // 2. 发送红色数据 (Write RAM Red)
    // 【关键修改】这里必须填 0x00 代表无色/透明。
    // 佳显驱动逻辑: 0x00=无色, 0xFF=红色 (与微雪旧版相反)
    SendCommand(0x26);
    for (UWORD j = 0; j < height; j++) {
        for (UWORD i = 0; i < width; i++) {
            SendData(0x00); // 填 0x00，千万别填 0xff
        }
    }
    
    // 3. 执行刷新 (Update)
    SendCommand(0x22);
    SendData(0xF7);     // 使用全屏刷新模式
    SendCommand(0x20);  // 激活刷新
    WaitUntilIdle();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          The only one parameter is a check code, the command would be
 *          You can use EPD_Reset() to awaken
 */
void Epd::Sleep(void) {
    SendCommand(0x02); // POWER_OFF
    WaitUntilIdle();
    SendCommand(0x07); // DEEP_SLEEP
    SendData(0xA5); // check code
}



/* END OF FILE */
