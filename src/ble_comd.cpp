#include <bluefruit.h>
#include "epd2in13_V3.h" 

BLEUart bleuart;
extern int refresh_count;
extern Epd epd;
extern unsigned char image[4000]; 

// --- 数据接收缓冲 ---
// 2.13寸屏 V3 分辨率 122x250
// 字节数 = 122 * 250 / 8 = 3812.5 -> 约 3813 字节
// 我们定义一个接收缓冲区和索引
uint8_t rxBuffer[4000];
int rxIndex = 0;
bool isReceivingImage = false;

// --- 模式定义 ---
enum AppMode {
  MODE_CLOCK,  // 时钟模式 (本地刷新)
  MODE_IMAGE   // 图片模式 (显示上位机发的图)
};

AppMode currentMode = MODE_CLOCK;

// --- 【核心】处理上位机发来的数据 ---
void handleBLEUart(void) {
  if (bleuart.available()) {
    
    // 如果还没开始接收图片，先判断是不是指令
    if (!isReceivingImage) {
      // 偷懒做法：读取第一个字符判断
      // 如果收到 'C' (CMD:CLOCK)，切换模式
      int c = bleuart.peek(); 
      
      if (c == 'C') { 
        // char buf[20];
        // bleuart.readBytes(buf, 9); // 读取 "CMD:CLOCK"
        // if (strncmp(buf, "CMD:CLOCK", 9) == 0) {
           Serial.println("Switch to CLOCK Mode");
           currentMode = MODE_CLOCK;
        //    refresh_count = 600; // 强制全刷一次
           return;
        // }
      }
      
      // 如果收到 'I' (IMG:START)，开始接收图片
      if (c == 'I') {
         char buf[10];
         bleuart.readBytes(buf, 9); // 读取 "IMG:START"
         if (strncmp(buf, "IMG:START", 9) == 0) {
            Serial.println("Start Receiving Image...");
            isReceivingImage = true;
            rxIndex = 0;
            currentMode = MODE_IMAGE; // 切换模式
            return;
         }
      }
    }

    // --- 正在接收图片数据 ---
    if (isReceivingImage) {
      while (bleuart.available()) {
        uint8_t b = bleuart.read();
        if (rxIndex < 4000) {
          rxBuffer[rxIndex++] = b;
        }
        
        // 2.13寸屏的数据量约为 4000 字节 (根据 epd.width/height 计算)
        // 实际上 122*250/8 = 3813 字节
        // 我们设定一个阈值，比如收够了 3813 个字节就刷屏
        if (rxIndex >= 3813) {
           Serial.println("Image Received! Displaying...");
           
           // 将接收到的数据复制到显存并显示
           memcpy(image, rxBuffer, 3813);
           
           epd.Init(FULL); // 图片建议全刷，清晰
           epd.Display(image);
           epd.Init(PART); // 刷完休眠或准备局刷
           
           isReceivingImage = false;
           rxIndex = 0;
        }
      }
    }
  }
}