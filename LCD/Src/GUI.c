/**
 ******************************************************************************
 * @file           : GUI.c
 * @brief          : 图形用户界面库 - 提供高级图形绘制功能
 *                   包括基本图形绘制、文字显示、按钮效果等功能
 * @author         : Chipdriver
 * @version        : V1.0
 * @date           : 2025-01-10
 ******************************************************************************
 * 主要功能：
 * - 颜色转换处理
 * - 基本图形绘制（线条、圆形、矩形）
 * - 按钮视觉效果
 * - 数字和字符串显示
 * - 为上层应用提供便捷的绘图接口
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include "lcd_Driver.h"  // 注意大小写
#include "GUI.h"
#include <stdio.h>  // 用于sprintf
#include "font.h"  

/*==================================================================颜色处理类=========================================================================*/
/**
 * @brief  BGR格式颜色转换为RGB格式
 * @param  c: BGR格式的16位颜色值
 * @return RGB格式的16位颜色值
 */
uint16_t LCD_BGR2RGB(uint16_t c)
{
  uint16_t  r,g,b,rgb;   
  b=(c>>0)&0x1f;
  g=(c>>5)&0x3f;
  r=(c>>11)&0x1f;	 
  rgb=(b<<11)+(g<<5)+(r<<0);		 
  return(rgb);
}

/*==================================================================基础几何图形类=========================================================================*/
/**
 * @brief  绘制圆形（空心）
 * @param  X: 圆心X坐标
 * @param  Y: 圆心Y坐标  
 * @param  R: 圆的半径
 * @param  fc: 圆形边框颜色
 * @return 无
 */
void Gui_Circle(uint16_t X,uint16_t Y,uint16_t R,uint16_t fc) 
{
    unsigned short  a,b; 
    int c; 
    a=0; 
    b=R; 
    c=3-2*R; 
    while (a<b) 
    { 
        Gui_DrawPoint(X+a,Y+b,fc);
        Gui_DrawPoint(X-a,Y+b,fc);
        Gui_DrawPoint(X+a,Y-b,fc);
        Gui_DrawPoint(X-a,Y-b,fc);
        Gui_DrawPoint(X+b,Y+a,fc);
        Gui_DrawPoint(X-b,Y+a,fc);
        Gui_DrawPoint(X+b,Y-a,fc);
        Gui_DrawPoint(X-b,Y-a,fc);

        if(c<0) c=c+4*a+6; 
        else 
        { 
            c=c+4*(a-b)+10; 
            b-=1; 
        } 
       a+=1; 
    } 
    if (a==b) 
    { 
        Gui_DrawPoint(X+a,Y+b,fc); 
        Gui_DrawPoint(X+a,Y+b,fc); 
        Gui_DrawPoint(X+a,Y-b,fc); 
        Gui_DrawPoint(X-a,Y-b,fc); 
        Gui_DrawPoint(X+b,Y+a,fc); 
        Gui_DrawPoint(X-b,Y+a,fc); 
        Gui_DrawPoint(X+b,Y-a,fc); 
        Gui_DrawPoint(X-b,Y-a,fc); 
    } 
}

/**
 * @brief  绘制直线（使用Bresenham算法）
 * @param  x0: 起始点X坐标
 * @param  y0: 起始点Y坐标
 * @param  x1: 终点X坐标
 * @param  y1: 终点Y坐标
 * @param  Color: 线条颜色
 * @return 无
 */
void Gui_DrawLine(uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1,uint16_t Color)   
{
    int dx, dy, dx2, dy2, x_inc, y_inc, error, index;

    Lcd_SetXY(x0,y0);
    dx = x1-x0;
    dy = y1-y0;

    if (dx>=0) {
        x_inc = 1;
    } else {
        x_inc = -1;
        dx = -dx;  
    } 
    
    if (dy>=0) {
        y_inc = 1;
    } else {
        y_inc = -1;
        dy = -dy; 
    } 

    dx2 = dx << 1;
    dy2 = dy << 1;

    if (dx > dy) {
        error = dy2 - dx; 
        for (index=0; index <= dx; index++) {
            Gui_DrawPoint(x0,y0,Color);
            if (error >= 0) {
                error-=dx2;
                y0+=y_inc;
            }
            error+=dy2;
            x0+=x_inc;
        }
    } else {
        error = dx2 - dy; 
        for (index=0; index <= dy; index++) {
            Gui_DrawPoint(x0,y0,Color);
            if (error >= 0) {
                error-=dy2;
                x0+=x_inc;
            }
            error+=dx2;
            y0+=y_inc;
        }
    }
}

/*==================================================================矩形/框体类=========================================================================*/

/**
 * @brief  绘制3D效果的矩形框
 * @param  x: 矩形左上角X坐标
 * @param  y: 矩形左上角Y坐标
 * @param  w: 矩形宽度
 * @param  h: 矩形高度
 * @param  bc: 矩形内部填充颜色
 * @return 无
 */
void Gui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h,uint16_t bc)
{
    Gui_DrawLine(x, y, x+w-1, y, 0xEF7D);           // 上边框（亮色-凸起效果）
    Gui_DrawLine(x, y, x, y+h-1, 0xEF7D);           // 左边框（亮色-凸起效果）
    Gui_DrawLine(x+w-1, y, x+w-1, y+h-1, 0x2965);  // 右边框（暗色-阴影效果）
    Gui_DrawLine(x, y+h-1, x+w-1, y+h-1, 0x2965);  // 下边框（暗色-阴影效果）
    
    uint16_t i, j;
    for(i = 1; i < h-1; i++) {
        for(j = 1; j < w-1; j++) {
            Gui_DrawPoint(x+j, y+i, bc);
        }
    }
}

/**
 * @brief  绘制不同模式的矩形边框
 * @param  x: 矩形左上角X坐标
 * @param  y: 矩形左上角Y坐标
 * @param  w: 矩形宽度
 * @param  h: 矩形高度
 * @param  mode: 绘制模式 (0:凸起效果, 1:凹陷效果, 2:普通边框)
 * @return 无
 */
void Gui_box2(uint16_t x,uint16_t y,uint16_t w,uint16_t h, uint8_t mode)
{
    if (mode==0) {
        Gui_DrawLine(x,y,x+w,y,0xEF7D);
        Gui_DrawLine(x+w-1,y+1,x+w-1,y+1+h,0x2965);
        Gui_DrawLine(x,y+h,x+w,y+h,0x2965);
        Gui_DrawLine(x,y,x,y+h,0xEF7D);
    }
    if (mode==1) {
        Gui_DrawLine(x,y,x+w,y,0x2965);
        Gui_DrawLine(x+w-1,y+1,x+w-1,y+1+h,0xEF7D);
        Gui_DrawLine(x,y+h,x+w,y+h,0xEF7D);
        Gui_DrawLine(x,y,x,y+h,0x2965);
    }
    if (mode==2) {
        Gui_DrawLine(x,y,x+w,y,0xffff);
        Gui_DrawLine(x+w-1,y+1,x+w-1,y+1+h,0xffff);
        Gui_DrawLine(x,y+h,x+w,y+h,0xffff);
        Gui_DrawLine(x,y,x,y+h,0xffff);
    }
}

/*==================================================================按钮视觉效果类=========================================================================*/

/**
 * @brief  绘制按下状态的按钮效果
 * @param  x1: 按钮左上角X坐标
 * @param  y1: 按钮左上角Y坐标
 * @param  x2: 按钮右下角X坐标
 * @param  y2: 按钮右下角Y坐标
 * @return 无
 */
void DisplayButtonDown(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
    uint16_t i, j;
    
    for(i = y1+2; i < y2-1; i++) {
        for(j = x1+2; j < x2-1; j++) {
            Gui_DrawPoint(j, i, 0xC618);  // 按下状态的灰色背景
        }
    }
    
    Gui_DrawLine(x1,  y1,  x2,y1, GRAY2);        // 上边框（暗色）
    Gui_DrawLine(x1+1,y1+1,x2-1,y1+1, GRAY1);    // 内上边框（中灰）
    Gui_DrawLine(x1,  y1,  x1,y2, GRAY2);        // 左边框（暗色）
    Gui_DrawLine(x1+1,y1+1,x1+1,y2-1, GRAY1);    // 内左边框（中灰）
    Gui_DrawLine(x1,  y2,  x2,y2, WHITE);        // 下边框（亮色）
    Gui_DrawLine(x2,  y1,  x2,y2, WHITE);        // 右边框（亮色）
}

/**
 * @brief  绘制弹起状态的按钮效果
 * @param  x1: 按钮左上角X坐标
 * @param  y1: 按钮左上角Y坐标
 * @param  x2: 按钮右下角X坐标
 * @param  y2: 按钮右下角Y坐标
 * @return 无
 */
void DisplayButtonUp(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
    uint16_t i, j;
    
    for(i = y1+2; i < y2-1; i++) {
        for(j = x1+2; j < x2-1; j++) {
            Gui_DrawPoint(j, i, 0xE71C);  // 弹起状态的浅灰色背景
        }
    }
    Gui_DrawLine(x1,  y1,  x2,y1, WHITE);        // 上边框（亮色）
    Gui_DrawLine(x1,  y1,  x1,y2, WHITE);        // 左边框（亮色）
    Gui_DrawLine(x1+1,y2-1,x2-1,y2-1, GRAY1);    // 内下边框（中灰）
    Gui_DrawLine(x1,  y2,  x2,y2, GRAY2);        // 下边框（暗色）
    Gui_DrawLine(x2-1,y1+1,x2-1,y2-1, GRAY1);    // 内右边框（中灰）
    Gui_DrawLine(x2,  y1,  x2,y2, GRAY2);        // 右边框（暗色）
}


/*================================================================== ASCII码显示类=========================================================================*/

/**
 * @brief 显示8x16像素英文字符（支持完整ASCII可打印字符）-单字符显示
 * @details 支持显示8x16像素的英文字母、数字、符号等ASCII字符
 *          从ascii_font[]字模数组中获取对应字符的字模数据进行显示
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param fc 前景色（字体颜色），16位RGB565格式
 * @param bc 背景色，16位RGB565格式
 * @param c 要显示的ASCII字符（0x20-0x7E）
 * @note 每个字符占用8x16=128像素，字模数据16字节
 * @note 支持字符范围：空格(' ')到波浪号('~')，共95个字符
 * @note 字模数据存储在ascii_font[]数组中
 * @example Gui_DrawAsciiChar(10, 20, WHITE, BLACK, 'A');
 */
void Gui_DrawAsciiChar(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, char c)
{
    if(c < 0x20 || c > 0x7E) return;  // 只支持可打印ASCII字符
    
    int char_index = c - 0x20;  // 转换为字模数组索引(空格是第0个)
    
    for(int row = 0; row < 16; row++) {
        uint8_t line = ascii_font[char_index][row];  // 获取该行字模数据
        for(int col = 0; col < 8; col++) {
            if(line & (0x80 >> col)) {
                Gui_DrawPoint(x + col, y + row, fc);  // 前景色
            } else {
                if(fc != bc) {
                    Gui_DrawPoint(x + col, y + row, bc);  // 背景色
                }
            }
        }
    }
}

/**
 * @brief 显示英文字符串（支持完整ASCII可打印字符）
 * @details 支持显示包含英文字母、数字、符号的完整字符串
 *          自动处理换行符，支持多行显示
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param fc 前景色（字体颜色），16位RGB565格式
 * @param bc 背景色，16位RGB565格式
 * @param str 要显示的字符串指针
 * @note 每个字符宽度8像素，高度16像素
 * @note 遇到'\n'自动换行，行高16像素
 * @note 支持字符：字母、数字、标点符号、特殊符号等
 * @example Gui_DrawAsciiString(10, 20, WHITE, BLACK, "Hello World!");
 * @example Gui_DrawAsciiString(10, 20, RED, BLACK, "Temp: 25.6C\nHumi: 60%");
 */
void Gui_DrawAsciiString(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, const char* str)
{
    uint16_t pos_x = x;
    uint16_t pos_y = y;
    
    for(int i = 0; str[i] != '\0'; i++) {
        if(str[i] == '\n') {
            // 换行处理
            pos_x = x;
            pos_y += 16;
        } else if(str[i] >= 0x20 && str[i] <= 0x7E) {
            // 显示可打印ASCII字符
            Gui_DrawAsciiChar(pos_x, pos_y, fc, bc, str[i]);
            pos_x += 8;
        } else if(str[i] == '\t') {
            // Tab键处理（4个空格宽度）
            pos_x += 32;
        }
        // 其他不可打印字符忽略
    }
}

/**
 * @brief 显示居中对齐的英文字符串
 * @details 在指定区域内居中显示英文字符串
 * @param x 区域起始X坐标
 * @param y 区域起始Y坐标  
 * @param width 区域宽度
 * @param fc 前景色（字体颜色）
 * @param bc 背景色
 * @param str 要显示的字符串
 * @note 字符串长度超过区域宽度时会被截断
 * @example Gui_DrawAsciiStringCenter(0, 50, 128, WHITE, BLACK, "CENTER");
 */
void Gui_DrawAsciiStringCenter(uint16_t x, uint16_t y, uint16_t width, uint16_t fc, uint16_t bc, const char* str)
{
    int str_len = 0;
    // 计算字符串长度（只计算可显示字符）
    for(int i = 0; str[i] != '\0'; i++) {
        if(str[i] >= 0x20 && str[i] <= 0x7E) {
            str_len++;
        }
    }
    
    int str_width = str_len * 8;  // 字符串总宽度
    
    if(str_width <= width) {
        // 计算居中位置
        uint16_t center_x = x + (width - str_width) / 2;
        Gui_DrawAsciiString(center_x, y, fc, bc, str);
    } else {
        // 字符串太长，左对齐显示
        Gui_DrawAsciiString(x, y, fc, bc, str);
    }
}


/*==================================================================中文字符显示类=========================================================================*/
/**
 * @brief 显示16x16像素中文字符
 * @details 支持显示16x16像素的中文汉字，使用GB2312/GBK编码
 *          从hz16[]字模数组中查找对应字符的字模数据进行显示
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param fc 前景色（字体颜色），16位RGB565格式
 * @param bc 背景色，16位RGB565格式
 * @param s 指向要显示的中文字符串的指针
 * @note 每个中文字符占用16x16=256像素，字模数据32字节
 * @note 只能显示已定义在hz16[]数组中的字符
 * @example Gui_DrawFont_GBK16(10, 20, WHITE, BLACK, (uint8_t*)"孙涵");
 */
void Gui_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s)
{
    unsigned char i,j;
    unsigned short k,x0;
    x0=x;

    while(*s) 
    {	
        if((*s) < 0x80)  // ASCII字符
        {
            if(*s >= 0x20 && *s <= 0x7E) {
                Gui_DrawAsciiChar(x, y, fc, bc, *s);
            }
            x += 8;  // ASCII字符宽度8像素
            s++;
        }
        else  // 中文字符
        {
            // 保持原来的中文显示逻辑不变
            for (k=0;k<hz16_num;k++) 
            {
                if ((hz16[k].Index[0]==*(s))&&(hz16[k].Index[1]==*(s+1)))
                { 
                    for(i=0;i<16;i++)
                    {
                        for(j=0;j<8;j++) 
                        {
                            if(hz16[k].Msk[i*2]&(0x80>>j))
                                Gui_DrawPoint(x+j,y+i,fc);
                            else {
                                if (fc!=bc) Gui_DrawPoint(x+j,y+i,bc);
                            }
                        }
                        for(j=0;j<8;j++) 
                        {
                            if(hz16[k].Msk[i*2+1]&(0x80>>j))
                                Gui_DrawPoint(x+j+8,y+i,fc);
                            else 
                            {
                                if (fc!=bc) Gui_DrawPoint(x+j+8,y+i,bc);
                            }
                        }
                    }
                }
            }
            s+=2;
            x+=16;  // 中文字符宽度16像素
        } 
    }
}

/**
 * @brief 显示24x24像素中文字符
 * @details 支持显示24x24像素的大号中文汉字，使用GB2312/GBK编码
 *          从hz24[]字模数组中查找对应字符的字模数据进行显示
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param fc 前景色（字体颜色），16位RGB565格式
 * @param bc 背景色，16位RGB565格式
 * @param s 指向要显示的中文字符串的指针
 * @note 每个中文字符占用24x24=576像素，字模数据72字节
 * @note 只能显示已定义在hz24[]数组中的字符
 * @note 显示效果比16x16字体更加清晰，适合重要信息显示
 * @example Gui_DrawFont_GBK24(50, 100, RED, BLACK, (uint8_t*)"霖");
 */
void Gui_DrawFont_GBK24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s)
{
    unsigned char i,j;
    unsigned short k;

    while(*s) 
    {
        if(*s < 0x80)  // ASCII字符
        {
            if(*s >= 0x20 && *s <= 0x7E) {
                Gui_DrawAsciiChar(x, y, fc, bc, *s);
            }
            x += 8;  // ASCII字符宽度8像素
            s++;
        }
        else  // 中文字符
        {
            // 保持原来的中文显示逻辑不变
            for (k=0;k<hz24_num;k++) 
            {
                if ((hz24[k].Index[0]==*(s))&&(hz24[k].Index[1]==*(s+1)))
                { 
                    for(i=0;i<24;i++)
                    {
                        for(j=0;j<8;j++) 
                        {
                            if(hz24[k].Msk[i*3]&(0x80>>j))
                                Gui_DrawPoint(x+j,y+i,fc);
                            else 
                            {
                                if (fc!=bc) Gui_DrawPoint(x+j,y+i,bc);
                            }
                        }
                        for(j=0;j<8;j++) 
                        {
                            if(hz24[k].Msk[i*3+1]&(0x80>>j))
                                Gui_DrawPoint(x+j+8,y+i,fc);
                            else {
                                if (fc!=bc) Gui_DrawPoint(x+j+8,y+i,bc);
                            }
                        }
                        for(j=0;j<8;j++) 
                        {
                            if(hz24[k].Msk[i*3+2]&(0x80>>j))	
                                Gui_DrawPoint(x+j+16,y+i,fc);
                            else 
                            {
                                if (fc!=bc) Gui_DrawPoint(x+j+16,y+i,bc);
                            }
                        }
                    }
                }
            }
            s+=2;
            x+=24;  // 中文字符宽度24像素
        }
    }
}


/*==================================================================大号数字类=========================================================================*/

/**
 * @brief 显示32x32像素大号数字字符
 * @details 显示单个32x32像素的数字字符（0-9），字体清晰，适合时钟显示
 *          从sz32[]字模数组中获取对应数字的字模数据进行显示
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param fc 前景色（数字颜色），16位RGB565格式
 * @param bc 背景色，16位RGB565格式
 * @param num 要显示的数字（0-9）
 * @note 每个数字占用32x32=1024像素，字模数据128字节
 * @note 数字字模数据存储在sz32[]数组中
 * @note 适合用于显示时钟时间、日期等重要数字信息
 * @example Gui_DrawFont_Num32(50, 100, WHITE, BLACK, 8); // 显示数字"8"
 */
void Gui_DrawFont_Num32(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint16_t num)
{
	unsigned char i,j,k,c;
	//lcd_text_any(x+94+i*42,y+34,32,32,0x7E8,0x0,sz32,knum[i]);
//	w=w/8;

    for(i=0;i<32;i++)
	{
		for(j=0;j<4;j++) 
		{
			c=*(sz32+num*32*4+i*4+j);
			for (k=0;k<8;k++)	
			{
	
		    	if(c&(0x80>>k))	Gui_DrawPoint(x+j*8+k,y+i,fc);
				else {
					if (fc!=bc) Gui_DrawPoint(x+j*8+k,y+i,bc);
				}
			}
		}
	}
}

/*==================================================================图片/位图显示类=========================================================================*/


/**
 * @brief 显示位图图片----最基础的位图显示函数
 * @details 在指定位置显示一个位图图片，支持任意尺寸
 *          图片数据为16位RGB565格式的像素数组
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param width 图片宽度（像素）
 * @param height 图片高度（像素）
 * @param bitmap 指向图片数据的指针，RGB565格式
 * @note 图片数据按行存储，每个像素2字节
 * @example Gui_DrawBitmap(10, 10, 32, 32, weather_icon);
 */
void Gui_DrawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap)
{
    uint16_t i, j;
    
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width; j++)
        {
            Gui_DrawPoint(x + j, y + i, bitmap[i * width + j]);
        }
    }
}

/**
 * @brief 显示小图标----专门用于正方形图标
 * @details 显示正方形小图标，通常用于天气图标、状态指示等
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param size 图标尺寸（像素，正方形）
 * @param icon_data 指向图标数据的指针，RGB565格式
 * @note 图标数据大小为 size × size × 2 字节
 * @example Gui_DrawIcon(50, 50, 16, sunny_icon_16x16);
 */
void Gui_DrawIcon(uint16_t x, uint16_t y, uint16_t size, const uint16_t *icon_data)
{
    Gui_DrawBitmap(x, y, size, size, icon_data);
}

/**
 * @brief 显示带头信息的图片数据
 * @details 显示包含头信息的图片数据，自动解析图片尺寸和格式
 *          支持常见的图片转换工具生成的数据格式
 * @param x 显示起始X坐标（左上角）
 * @param y 显示起始Y坐标（左上角）
 * @param image_data 指向图片数据的指针，包含头信息和像素数据
 * @note 图片数据格式：前8字节为头信息，后续为RGB565像素数据
 * @note 头信息包含：格式标识、宽度、高度、颜色深度等
 * @example Gui_DrawImage(10, 10, gImage_weather);
 */
void Gui_DrawImage(uint16_t x, uint16_t y, const uint8_t *image_data)
{
    // 解析图片头信息
    uint16_t width = image_data[2] | (image_data[3] << 8);   // 小端序读取宽度
    uint16_t height = image_data[4] | (image_data[5] << 8);  // 小端序读取高度
    
    // 跳过8字节头信息，指向实际像素数据
    const uint8_t *pixel_data = image_data + 8;
    
    uint16_t i, j;
    uint16_t color;
    
    // 逐像素绘制图片
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width; j++)
        {
            // 从字节数据中读取16位颜色值（小端序）
            uint16_t pixel_index = (i * width + j) * 2;
            color = pixel_data[pixel_index] | (pixel_data[pixel_index + 1] << 8);
            
            Gui_DrawPoint(x + j, y + i, color);
        }
    }
}


