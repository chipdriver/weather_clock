/**
 ******************************************************************************
 * @file           : lcd_Driver.c
 * @brief          : ST7735 LCD底层驱动程序
 *                   提供与ST7735芯片直接通信的底层功能
 * @author         : STM32 Weather Clock Project
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

#include "lcd_Driver.h"  // 注意大小写要匹配
#include "stm32f4xx_hal.h"
#include "LCD_Config.h"
/**
 * @brief  LCD GPIO引脚初始化
 * @param  无
 * @return 无
 */
void LCD_GPIO_Init(void)
{
    //结构体
    GPIO_InitTypeDef  GPIO_InitStructure = { 0 };
    //使能时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    //配置引脚 - 所有LCD控制引脚
    GPIO_InitStructure.Pin = GPIO_PIN_1| GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化引脚状态
    LCD_CS_SET;      // CS默认高电平
    LCD_SCL_SET;     // SCL默认高电平
    LCD_SDA_SET;     // SDA默认高电平
    LCD_RES_SET;     // RES默认高电平
    LCD_DC_SET;      // DC默认高电平
    LCD_BLK_SET;     // 背光开启
}

/**
 * @brief  软件SPI发送8位数据
 * @param  Data: 要发送的8位数据
 * @return 无
 */
void SPI_WriteData(uint8_t Data)
{
    unsigned char i=0;
    for(i=8;i>0;i--)
    {
        if(Data&0x80)    
            LCD_SDA_SET; //输出数据1
        else 
            LCD_SDA_CLR; //输出数据0
           
        LCD_SCL_CLR;     // 时钟下降沿    
        // 可以添加短延时保证时序稳定
        // for(volatile int j=0; j<10; j++); 
        LCD_SCL_SET;     // 时钟上升沿，数据被采样
        Data<<=1;        // 数据左移，准备下一位
    }
}

/**
 * @brief  向ST7735发送命令
 * @param  Index: 要发送的8位命令
 * @return 无
 */
void Lcd_WriteIndex(uint8_t Index)
{
    //SPI 写命令时序开始
    LCD_CS_CLR;        // 选中设备
    LCD_RS_CLR;        // DC=0 表示命令
    SPI_WriteData(Index);
    LCD_CS_SET;        // 取消选中
}

/**
 * @brief  向ST7735发送数据
 * @param  Data: 要发送的8位数据
 * @return 无
 */
void Lcd_WriteData(uint8_t Data)
{
    LCD_CS_CLR;        // 选中设备
    LCD_RS_SET;        // DC=1 表示数据
    SPI_WriteData(Data);
    LCD_CS_SET;        // 取消选中
}

//向液晶屏写一个16位数据
void LCD_WriteData_16Bit(uint16_t Data)
{
    LCD_CS_CLR;        // 选中设备
    LCD_RS_SET;        // DC=1 表示数据
    SPI_WriteData(Data>>8);     //写入高8位数据
    SPI_WriteData(Data & 0xFF); //写入低8位数据
    LCD_CS_SET;        // 取消选中
}

void Lcd_WriteReg(uint8_t Index,uint8_t Data)
{
    Lcd_WriteIndex(Index);
    Lcd_WriteData(Data);
}

void Lcd_Reset(void)
{
    LCD_RST_CLR;       // 复位低电平
    HAL_Delay(100);
    LCD_RST_SET;       // 复位高电平
    HAL_Delay(50);
}

// 背光控制函数
void LCD_BacklightOn(void) {
    LCD_BLK_SET;
}

void LCD_BacklightOff(void) {
    LCD_BLK_CLR;
}

/**
 * @brief  ST7735 LCD初始化（适用于1.44寸屏幕）
 * @param  无
 * @return 无
 */
void Lcd_Init(void)
{    
    LCD_GPIO_Init();   // 初始化GPIO
    Lcd_Reset();       // 复位LCD

    //LCD Init For 1.44Inch LCD Panel with ST7735R.
    Lcd_WriteIndex(0x11);//Sleep exit 
    HAL_Delay(120);
        
    //ST7735R Frame Rate
    Lcd_WriteIndex(0xB1); 
    Lcd_WriteData(0x01); 
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 

    Lcd_WriteIndex(0xB2); 
    Lcd_WriteData(0x01); 
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 

    Lcd_WriteIndex(0xB3); 
    Lcd_WriteData(0x01); 
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 
    Lcd_WriteData(0x01); 
    Lcd_WriteData(0x2C); 
    Lcd_WriteData(0x2D); 
    
    Lcd_WriteIndex(0xB4); //Column inversion 
    Lcd_WriteData(0x07); 
    
    //ST7735R Power Sequence
    Lcd_WriteIndex(0xC0); 
    Lcd_WriteData(0xA2); 
    Lcd_WriteData(0x02); 
    Lcd_WriteData(0x84); 
    Lcd_WriteIndex(0xC1); 
    Lcd_WriteData(0xC5); 

    Lcd_WriteIndex(0xC2); 
    Lcd_WriteData(0x0A); 
    Lcd_WriteData(0x00); 

    Lcd_WriteIndex(0xC3); 
    Lcd_WriteData(0x8A); 
    Lcd_WriteData(0x2A); 
    Lcd_WriteIndex(0xC4); 
    Lcd_WriteData(0x8A); 
    Lcd_WriteData(0xEE); 
    
    Lcd_WriteIndex(0xC5); //VCOM 
    Lcd_WriteData(0x0E); 
    
    Lcd_WriteIndex(0x36); //MX, MY, RGB mode 
    Lcd_WriteData(0xC8); 
    
    //ST7735R Gamma Sequence
    Lcd_WriteIndex(0xe0); 
    Lcd_WriteData(0x0f); 
    Lcd_WriteData(0x1a); 
    Lcd_WriteData(0x0f); 
    Lcd_WriteData(0x18); 
    Lcd_WriteData(0x2f); 
    Lcd_WriteData(0x28); 
    Lcd_WriteData(0x20); 
    Lcd_WriteData(0x22); 
    Lcd_WriteData(0x1f); 
    Lcd_WriteData(0x1b); 
    Lcd_WriteData(0x23); 
    Lcd_WriteData(0x37); 
    Lcd_WriteData(0x00);     
    Lcd_WriteData(0x07); 
    Lcd_WriteData(0x02); 
    Lcd_WriteData(0x10); 

    Lcd_WriteIndex(0xe1); 
    Lcd_WriteData(0x0f); 
    Lcd_WriteData(0x1b); 
    Lcd_WriteData(0x0f); 
    Lcd_WriteData(0x17); 
    Lcd_WriteData(0x33); 
    Lcd_WriteData(0x2c); 
    Lcd_WriteData(0x29); 
    Lcd_WriteData(0x2e); 
    Lcd_WriteData(0x30); 
    Lcd_WriteData(0x30); 
    Lcd_WriteData(0x39); 
    Lcd_WriteData(0x3f); 
    Lcd_WriteData(0x00); 
    Lcd_WriteData(0x07); 
    Lcd_WriteData(0x03); 
    Lcd_WriteData(0x10);  
    
    Lcd_WriteIndex(0x2a);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x7f);

    Lcd_WriteIndex(0x2b);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x00);
    Lcd_WriteData(0x9f);
    
    Lcd_WriteIndex(0xF0); //Enable test command  
    Lcd_WriteData(0x01); 
    Lcd_WriteIndex(0xF6); //Disable ram power save mode 
    Lcd_WriteData(0x00); 
    
    Lcd_WriteIndex(0x3A); //65k mode 
    Lcd_WriteData(0x05); 
    
    Lcd_WriteIndex(0x29);//Display on     
    
    // 开启背光
    LCD_BacklightOn();
}

/*************************************************
º¯ÊýÃû£ºLCD_Set_Region
¹¦ÄÜ£ºÉèÖÃlcdÏÔÊ¾ÇøÓò£¬ÔÚ´ËÇøÓòÐ´µãÊý¾Ý×Ô¶¯»»ÐÐ
Èë¿Ú²ÎÊý£ºxyÆðµãºÍÖÕµã
·µ»ØÖµ£ºÎÞ
*************************************************/
void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
{		
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData(0x00);
	Lcd_WriteData(x_start+2);
	Lcd_WriteData(0x00);
	Lcd_WriteData(x_end+2);

	Lcd_WriteIndex(0x2b);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_start+3);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_end+3);
	
	Lcd_WriteIndex(0x2c);

}

/*************************************************
º¯ÊýÃû£ºLCD_Set_XY
¹¦ÄÜ£ºÉèÖÃlcdÏÔÊ¾ÆðÊ¼µã
Èë¿Ú²ÎÊý£ºxy×ø±ê
·µ»ØÖµ£ºÎÞ
*************************************************/
void Lcd_SetXY(uint16_t x,uint16_t y)
{
  	Lcd_SetRegion(x,y,x,y);
}

	
/*************************************************
º¯ÊýÃû£ºLCD_DrawPoint
¹¦ÄÜ£º»­Ò»¸öµã
Èë¿Ú²ÎÊý£ºÎÞ
·µ»ØÖµ£ºÎÞ
*************************************************/
void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data)
{
	Lcd_SetRegion(x,y,x+1,y+1);
	LCD_WriteData_16Bit(Data);

}    

/*****************************************
 º¯Êý¹¦ÄÜ£º¶ÁTFTÄ³Ò»µãµÄÑÕÉ«                          
 ³ö¿Ú²ÎÊý£ºcolor  µãÑÕÉ«Öµ                                 
******************************************/
unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y)
{
  unsigned int Data;
  Lcd_SetXY(x,y);

  //Lcd_ReadData();//¶ªµôÎÞÓÃ×Ö½Ú
  //Data=Lcd_ReadData();
  Lcd_WriteData(Data);
  return Data;
}
/**
 * @brief  全屏清屏函数
 * @param  Color: 填充颜色
 * @return 无
 */
void Lcd_Clear(uint16_t Color)               
{	
   unsigned int i,m;
   Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
   Lcd_WriteIndex(0x2C);
   for(i=0;i<X_MAX_PIXEL;i++)
    for(m=0;m<Y_MAX_PIXEL;m++)
    {	
	  	LCD_WriteData_16Bit(Color);
    }   
}

