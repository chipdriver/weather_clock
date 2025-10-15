/**
 ******************************************************************************
 * @file           : lcd_Driver.c
 * @brief          : ST7735 LCD底层驱动程序
 *                   提供与ST7735芯片直接通信的底层功能
 * @author         : Chipdriver
 * @version        : V1.0
 * @date           : 2025-01-10
 ******************************************************************************
 * 主要功能：
 * - GPIO硬件初始化
 * - 软件SPI通信实现
 * - ST7735芯片初始化和配置
 * - 基础显示操作（清屏、画点、区域设置）
 * - 背光和显示控制
 * - 为上层GUI提供底层硬件接口
 ******************************************************************************
 */

#include "lcd_Driver.h"  
#include "stm32f4xx_hal.h"
#include "LCD_Config.h"

/*========================底层通信必须函数（硬件直接操作），最基础的函数，所有其他功能都依赖它们=========================*/

/*---------------------------------------------------GPIO硬件层----------------------------------------------------*/
/**
 * @brief  LCD GPIO引脚初始化
 * @param  无
 * @return 无
 * @note   初始化PA1(BLK), PA2(RES), PA3(DC), PA4(CS), PA5(SCL), PA7(SDA)
 */
void LCD_GPIO_Init(void)
{
    // 定义GPIO初始化结构体
    GPIO_InitTypeDef  GPIO_InitStructure = { 0 };
    
    // 使能GPIOA时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置引脚 - 所有LCD控制引脚 (PA1, PA2, PA3, PA4, PA5, PA7)
    GPIO_InitStructure.Pin = GPIO_PIN_1| GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;              // 推挽输出模式
    GPIO_InitStructure.Pull = GPIO_NOPULL;                      // 无上下拉
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;       // 高速模式
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);                  // 初始化GPIOA
    
    // 初始化引脚状态为默认值
    LCD_CS_SET;      // CS默认高电平（未选中）
    LCD_SCL_SET;     // SCL默认高电平（时钟空闲）
    LCD_SDA_SET;     // SDA默认高电平（数据线空闲）
    LCD_RES_SET;     // RES默认高电平（不复位）
    LCD_DC_SET;      // DC默认高电平（数据模式）
    LCD_BLK_SET;     // 背光开启
}

/*---------------------------------------------------SPI通信核心----------------------------------------------------*/
/**
 * @brief  软件SPI发送8位数据
 * @param  Data: 要发送的8位数据
 * @return 无
 * @note   使用软件模拟SPI协议，MSB先发送
 */
void SPI_WriteData(uint8_t Data)
{
    unsigned char i=0;
    
    // 循环8次，每次发送1位数据
    for(i=8; i>0; i--)
    {
        // 判断最高位是1还是0
        if(Data & 0x80)    
            LCD_SDA_SET;  // 如果最高位是1，输出高电平
        else 
            LCD_SDA_CLR;  // 如果最高位是0，输出低电平
           
        LCD_SCL_CLR;      // 时钟下降沿，准备数据
        // 可以添加短延时保证时序稳定
        // for(volatile int j=0; j<10; j++); 
        LCD_SCL_SET;      // 时钟上升沿，从机采样数据
        Data <<= 1;       // 数据左移1位，准备发送下一位
    }
}

/*---------------------------------------------------命令/数据发送----------------------------------------------------*/

/**
 * @brief  向ST7735发送命令（寄存器地址）
 * @param  Index: 要发送的8位命令
 * @return 无
 * @note   DC=0表示发送的是命令 
 */
void Lcd_WriteIndex(uint8_t Index)
{
    // SPI 写命令时序开始
    LCD_CS_CLR;        // 片选信号拉低，选中LCD设备
    LCD_RS_CLR;        // DC=0 表示发送命令
    SPI_WriteData(Index);  // 发送8位命令
    LCD_CS_SET;        // 片选信号拉高，取消选中
}

/**
 * @brief  向ST7735发送数据（寄存器数据）
 * @param  Data: 要发送的8位数据
 * @return 无
 * @note   DC=1表示发送的是数据
 */
void Lcd_WriteData(uint8_t Data)
{
    LCD_CS_CLR;        // 片选信号拉低，选中LCD设备
    LCD_RS_SET;        // DC=1 表示发送数据
    SPI_WriteData(Data);  // 发送8位数据
    LCD_CS_SET;        // 片选信号拉高，取消选中
}

/*---------------------------------------------------硬件控制----------------------------------------------------*/

/**
 * @brief  LCD硬件复位
 * @param  无
 * @return 无
 * @note   通过拉低复位引脚100ms实现硬件复位
 */
void Lcd_Reset(void)
{
    LCD_RST_CLR;       // 复位引脚拉低
    HAL_Delay(100);    // 保持低电平100ms
    LCD_RST_SET;       // 复位引脚拉高
    HAL_Delay(50);     // 等待50ms让LCD稳定
}

/**
 * @brief  打开LCD背光
 * @param  无
 * @return 无
 */
void LCD_BacklightOn(void) 
{
    LCD_BLK_SET;  // 背光引脚设置为高电平
}

/**
 * @brief  关闭LCD背光
 * @param  无
 * @return 无
 */
void LCD_BacklightOff(void) 
{
    LCD_BLK_CLR;  // 背光引脚设置为低电平
}

/*========================组合功能函数（基于底层通信组合而成）,基于上面的底层函数组合实现的=========================*/

/*---------------------------------------------------第1层组合：数据格式转换----------------------------------------------------*/

/**
 * @brief  向LCD写入一个16位数据（RGB565格式）
 * @param  Data: 要发送的16位数据
 * @return 无
 * @note   先发送高8位，再发送低8位
 */
void LCD_WriteData_16Bit(uint16_t Data)
{
    LCD_CS_CLR;                 // 片选信号拉低，选中LCD设备
    LCD_RS_SET;                 // DC=1 表示发送数据
    SPI_WriteData(Data >> 8);   // 发送高8位数据
    SPI_WriteData(Data & 0xFF); // 发送低8位数据
    LCD_CS_SET;                 // 片选信号拉高，取消选中
}

/**
 * @brief  向LCD寄存器写入命令和数据
 * @param  Index: 寄存器地址（命令）
 * @param  Data: 要写入的数据
 * @return 无
 */
void Lcd_WriteReg(uint8_t Index, uint8_t Data)
{
    Lcd_WriteIndex(Index);  // 先发送命令
    Lcd_WriteData(Data);    // 再发送数据
}

/*---------------------------------------------------第2层组合：显示区域控制----------------------------------------------------*/

/**
 * @brief  设置LCD显示区域
 * @param  x_start: 起始X坐标 (0-127)
 * @param  y_start: 起始Y坐标 (0-159)
 * @param  x_end: 结束X坐标 (0-127)
 * @param  y_end: 结束Y坐标 (0-159)
 * @return 无
 * @note   设置后，在此区域写点数据会自动换行
 */
void Lcd_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{		
    // 设置列地址范围 (Column Address Set)
    Lcd_WriteIndex(0x2A);           // CASET命令
    Lcd_WriteData(0x00);            // XS高字节
    Lcd_WriteData(x_start + 2);     // XS低字节 (+2是屏幕偏移)
    Lcd_WriteData(0x00);            // XE高字节
    Lcd_WriteData(x_end + 2);       // XE低字节

    // 设置行地址范围 (Row Address Set)
    Lcd_WriteIndex(0x2B);           // RASET命令
    Lcd_WriteData(0x00);            // YS高字节
    Lcd_WriteData(y_start + 3);     // YS低字节 (+3是屏幕偏移)
    Lcd_WriteData(0x00);            // YE高字节
    Lcd_WriteData(y_end + 3);       // YE低字节
    
    // 准备写入显存 (Memory Write)
    Lcd_WriteIndex(0x2C);           // RAMWR命令
}

/**
 * @brief  设置LCD光标位置
 * @param  x: X坐标 (0-127)
 * @param  y: Y坐标 (0-159)
 * @return 无
 * @note   实际上是设置一个1x1像素的显示区域
 */
void Lcd_SetXY(uint16_t x, uint16_t y)
{
    Lcd_SetRegion(x, y, x, y);  // 设置单个像素区域
}

/*---------------------------------------------------第3层组合：像素操作----------------------------------------------------*/

/**
 * @brief  在指定位置画一个点
 * @param  x: X坐标 (0-127)
 * @param  y: Y坐标 (0-159)
 * @param  Data: 点的颜色 (RGB565格式)
 * @return 无
 */
void Gui_DrawPoint(uint16_t x, uint16_t y, uint16_t Data)
{
    Lcd_SetRegion(x, y, x+1, y+1);  // 设置一个像素的区域
    LCD_WriteData_16Bit(Data);       // 写入颜色数据
}    

/**
 * @brief  读取LCD某一点的颜色值
 * @param  x: X坐标 (0-127)
 * @param  y: Y坐标 (0-159)
 * @return 该点的颜色值 (RGB565格式)
 * @note   此函数未完全实现，需要配置SPI读取模式
 */
unsigned int Lcd_ReadPoint(uint16_t x, uint16_t y)
{
    unsigned int Data;
    Lcd_SetXY(x, y);  // 设置读取位置

    // TODO: 需要实现SPI读取功能
    // Lcd_ReadData(); // 丢掉第一个无用字节
    // Data = Lcd_ReadData(); // 读取实际数据
    
    Lcd_WriteData(Data);  // 临时代码
    return Data;
}

/*---------------------------------------------------第4层组合：整屏操作----------------------------------------------------*/

/**
 * @brief  全屏清屏函数
 * @param  Color: 填充颜色 (RGB565格式)
 * @return 无
 * @note   将整个屏幕填充为指定颜色
 */
void Lcd_Clear(uint16_t Color)               
{	
    unsigned int i, m;
    
    // 设置全屏显示区域 (0,0) 到 (127,159)
    Lcd_SetRegion(0, 0, X_MAX_PIXEL-1, Y_MAX_PIXEL-1);
    
    // 准备写入显存
    Lcd_WriteIndex(0x2C);
    
    // 遍历所有像素点
    for(i=0; i<X_MAX_PIXEL; i++)      // 列循环 (128次)
    {
        for(m=0; m<Y_MAX_PIXEL; m++)  // 行循环 (160次)
        {	
            LCD_WriteData_16Bit(Color);  // 写入颜色数据
        }
    }   
}

/*---------------------------------------------------最高层：初始化----------------------------------------------------*/

/**
 * @brief  ST7735R LCD初始化（适用于1.44寸128x160屏幕）
 * @param  无
 * @return 无
 * @note   包含完整的ST7735R初始化序列
 */
void Lcd_Init(void)
{    
    LCD_GPIO_Init();   // 初始化GPIO引脚
    Lcd_Reset();       // 硬件复位LCD

    // ============ ST7735R初始化序列 ============
    
    // 退出睡眠模式
    Lcd_WriteIndex(0x11);  // SLPOUT - Sleep Out
    HAL_Delay(120);        // 等待120ms让LCD稳定
        
    // -------- 帧率控制 --------
    // Frame Rate Control (In normal mode/ Full colors)
    Lcd_WriteIndex(0xB1);  
    Lcd_WriteData(0x01);   // RTNA设置
    Lcd_WriteData(0x2C);   // FPA (前廊参数)
    Lcd_WriteData(0x2D);   // BPA (后廊参数)

    // Frame Rate Control (In Idle mode/ 8-colors)
    Lcd_WriteIndex(0xB2); 
    Lcd_WriteData(0x01); 
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 

    // Frame Rate Control (In Partial mode/ full colors)
    Lcd_WriteIndex(0xB3); 
    Lcd_WriteData(0x01);   // Dot inversion模式
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 
    Lcd_WriteData(0x01);   // Line inversion模式
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 
    
    // Display Inversion Control
    Lcd_WriteIndex(0xB4);  
    Lcd_WriteData(0x07);   // Column inversion (列反转)
    
    // -------- 电源控制序列 --------
    // Power Control 1
    Lcd_WriteIndex(0xC0); 
    Lcd_WriteData(0xA2);   // AVDD=5.0V
    Lcd_WriteData(0x02);   // GVDD=4.6V
    Lcd_WriteData(0x84);   // GVCL=-4.6V
    
    // Power Control 2
    Lcd_WriteIndex(0xC1); 
    Lcd_WriteData(0xC5);   // VGH=14.7V, VGL=-7.35V

    // Power Control 3 (in Normal mode/ Full colors)
    Lcd_WriteIndex(0xC2); 
    Lcd_WriteData(0x0A);   // OP放大器电流设置
    Lcd_WriteData(0x00);   // 增压频率设置

    // Power Control 4 (in Idle mode/ 8-colors)
    Lcd_WriteIndex(0xC3); 
    Lcd_WriteData(0x8A); 
    Lcd_WriteData(0x2A); 
    
    // Power Control 5 (in Partial mode/ full-colors)
    Lcd_WriteIndex(0xC4); 
    Lcd_WriteData(0x8A); 
    Lcd_WriteData(0xEE); 
    
    // VCOM Control 1
    Lcd_WriteIndex(0xC5);  
    Lcd_WriteData(0x0E);   // VCOM电压设置
    
    // Memory Data Access Control
    Lcd_WriteIndex(0x36);  
    Lcd_WriteData(0xC8);   // MX=1, MY=1, RGB模式
                           // 屏幕方向：旋转180度
    
    // -------- Gamma校正序列 --------
    // Positive Gamma Correction
    Lcd_WriteIndex(0xE0); 
    Lcd_WriteData(0x0f);   // VP63
    Lcd_WriteData(0x1a);   // VP62
    Lcd_WriteData(0x0f);   // VP61
    Lcd_WriteData(0x18);   // VP59
    Lcd_WriteData(0x2f);   // VP57
    Lcd_WriteData(0x28);   // VP50
    Lcd_WriteData(0x20);   // VP43
    Lcd_WriteData(0x22);   // VP36
    Lcd_WriteData(0x1f);   // VP27
    Lcd_WriteData(0x1b);   // VP20
    Lcd_WriteData(0x23);   // VP13
    Lcd_WriteData(0x37);   // VP6
    Lcd_WriteData(0x00);   // VP4
    Lcd_WriteData(0x07);   // VP2
    Lcd_WriteData(0x02);   // VP1
    Lcd_WriteData(0x10);   // VP0

    // Negative Gamma Correction
    Lcd_WriteIndex(0xE1); 
    Lcd_WriteData(0x0f);   // VN63
    Lcd_WriteData(0x1b);   // VN62
    Lcd_WriteData(0x0f);   // VN61
    Lcd_WriteData(0x17);   // VN59
    Lcd_WriteData(0x33);   // VN57
    Lcd_WriteData(0x2c);   // VN50
    Lcd_WriteData(0x29);   // VN43
    Lcd_WriteData(0x2e);   // VN36
    Lcd_WriteData(0x30);   // VN27
    Lcd_WriteData(0x30);   // VN20
    Lcd_WriteData(0x39);   // VN13
    Lcd_WriteData(0x3f);   // VN6
    Lcd_WriteData(0x00);   // VN4
    Lcd_WriteData(0x07);   // VN2
    Lcd_WriteData(0x03);   // VN1
    Lcd_WriteData(0x10);   // VN0
    
    // -------- 显示区域设置 --------
    // Column Address Set (X坐标范围)
    Lcd_WriteIndex(0x2A);
    Lcd_WriteData(0x00);   // XS高字节
    Lcd_WriteData(0x00);   // XS低字节 (起始列=0)
    Lcd_WriteData(0x00);   // XE高字节
    Lcd_WriteData(0x7f);   // XE低字节 (结束列=127)

    // Row Address Set (Y坐标范围)
    Lcd_WriteIndex(0x2B);
    Lcd_WriteData(0x00);   // YS高字节
    Lcd_WriteData(0x00);   // YS低字节 (起始行=0)
    Lcd_WriteData(0x00);   // YE高字节
    Lcd_WriteData(0x9f);   // YE低字节 (结束行=159)
    
    // Enable test command (制造商特定命令)
    Lcd_WriteIndex(0xF0);  
    Lcd_WriteData(0x01);   
    
    // Disable ram power save mode
    Lcd_WriteIndex(0xF6);  
    Lcd_WriteData(0x00);   
    
    // Interface Pixel Format (像素格式设置)
    Lcd_WriteIndex(0x3A);  
    Lcd_WriteData(0x05);   // 16-bit/pixel (RGB565格式)
    
    // Display On (开启显示)
    Lcd_WriteIndex(0x29);  
    
    // 开启背光
    LCD_BacklightOn();
}









