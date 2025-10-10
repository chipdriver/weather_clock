#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

/*头文件*/
#include "stm32f4xx_hal.h"

#define RED  	0xf800
#define GREEN	0x07e0
#define BLUE 	0x001f
#define WHITE	0xffff
#define BLACK	0x0000
#define YELLOW  0xFFE0
#define CYAN    0x07FF      // 青色
#define MAGENTA 0xF81F      // 洋红色
#define GRAY0   0xEF7D   	
#define GRAY1   0x8410      	
#define GRAY2   0x4208      	




#define LCD_CTRL   	  	    GPIOA		// 所有控制引脚都在GPIOA上

// 根据引脚配置表定义
#define LCD_SCL        	    GPIO_PIN_5	//PA5 SPI1_SCK--->>ST7735 --SCL
#define LCD_SDA        	    GPIO_PIN_7	//PA7 SPI1_MOSI--->>ST7735 --SDA
#define LCD_CS        	    GPIO_PIN_4  //PA4 GPIO_Out--->>ST7735 --CS

#define LCD_BLK        	    GPIO_PIN_1  //PA1 GPIO_Out--->>ST7735 --BLK (背光控制)
#define LCD_DC         	    GPIO_PIN_3	//PA3 GPIO_Out--->>ST7735 --DC (数据/命令选择)
#define LCD_RES     	    GPIO_PIN_2	//PA2 GPIO_Out--->>ST7735 --RES (硬件复位)

//#define LCD_CS_SET(x) LCD_CTRL->ODR=(LCD_CTRL->ODR&~LCD_CS)|(x ? LCD_CS:0)

//LCD控制口置1操作语句宏定义 (使用HAL库标准方式)
#define	LCD_SCL_SET     HAL_GPIO_WritePin(LCD_CTRL, LCD_SCL, GPIO_PIN_SET)    
#define	LCD_SDA_SET     HAL_GPIO_WritePin(LCD_CTRL, LCD_SDA, GPIO_PIN_SET)   
#define	LCD_CS_SET      HAL_GPIO_WritePin(LCD_CTRL, LCD_CS, GPIO_PIN_SET)  
#define	LCD_BLK_SET     HAL_GPIO_WritePin(LCD_CTRL, LCD_BLK, GPIO_PIN_SET)   
#define	LCD_DC_SET      HAL_GPIO_WritePin(LCD_CTRL, LCD_DC, GPIO_PIN_SET) 
#define	LCD_RES_SET     HAL_GPIO_WritePin(LCD_CTRL, LCD_RES, GPIO_PIN_SET) 

//LCD控制口置0操作语句宏定义
#define	LCD_SCL_CLR     HAL_GPIO_WritePin(LCD_CTRL, LCD_SCL, GPIO_PIN_RESET)  
#define	LCD_SDA_CLR     HAL_GPIO_WritePin(LCD_CTRL, LCD_SDA, GPIO_PIN_RESET) 
#define	LCD_CS_CLR      HAL_GPIO_WritePin(LCD_CTRL, LCD_CS, GPIO_PIN_RESET) 
#define	LCD_BLK_CLR     HAL_GPIO_WritePin(LCD_CTRL, LCD_BLK, GPIO_PIN_RESET) 
#define	LCD_RES_CLR     HAL_GPIO_WritePin(LCD_CTRL, LCD_RES, GPIO_PIN_RESET)
#define	LCD_DC_CLR      HAL_GPIO_WritePin(LCD_CTRL, LCD_DC, GPIO_PIN_RESET)

// 为了兼容旧代码，保留旧的宏定义
#define LCD_LED_SET     LCD_BLK_SET
#define LCD_LED_CLR     LCD_BLK_CLR
#define LCD_RS_SET      LCD_DC_SET
#define LCD_RS_CLR      LCD_DC_CLR
#define LCD_RST_SET     LCD_RES_SET
#define LCD_RST_CLR     LCD_RES_CLR 

#define LCD_DATAOUT(x) LCD_DATA->ODR=x; //Êý¾ÝÊä³ö
#define LCD_DATAIN     LCD_DATA->IDR;   //Êý¾ÝÊäÈë

#define LCD_WR_DATA(data){\
LCD_RS_SET;\
LCD_CS_CLR;\
LCD_DATAOUT(data);\
LCD_WR_CLR;\
LCD_WR_SET;\
LCD_CS_SET;\
} 



// ST7735 LCD驱动函数声明
void LCD_GPIO_Init(void);
void LCD_SPI_Init(void);
void Lcd_WriteCommand(uint8_t cmd);
void Lcd_WriteData(uint8_t data);
void Lcd_WriteData_16Bit(uint16_t data);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_Clear(uint16_t Color);
void Lcd_SetXY(uint16_t x, uint16_t y);
void Lcd_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void Gui_DrawPoint(uint16_t x, uint16_t y, uint16_t Color);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_BacklightOn(void);
void LCD_BacklightOff(void);

// 兼容旧接口
void Lcd_WriteIndex(uint8_t Index);
void Lcd_WriteReg(uint8_t Index, uint8_t Data);
uint16_t Lcd_ReadReg(uint8_t LCD_Reg);
unsigned int Lcd_ReadPoint(uint16_t x, uint16_t y);

#endif /* __LCD_DRIVER_H */
