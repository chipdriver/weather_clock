#ifndef __GUI_H
#define __GUI_H

#include "stm32f4xx_hal.h"

// 函数声明
uint16_t LCD_BGR2RGB(uint16_t c);
void Gui_Circle(uint16_t X, uint16_t Y, uint16_t R, uint16_t fc);
void Gui_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Color);
void Gui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bc);
void Gui_box2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t mode);
void DisplayButtonDown(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void DisplayButtonUp(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// 新增的文字显示函数
void Gui_DrawNumber(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint32_t num);
void Gui_Draw8x16Char(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, char c);
void Gui_DrawString(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, const char* str);

// 汉字显示函数
void Gui_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s);
void Gui_DrawFont_GBK24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s);

// 图片显示函数
void Gui_DrawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap);
void Gui_DrawIcon(uint16_t x, uint16_t y, uint16_t size, const uint16_t *icon_data);
void Gui_DrawImage(uint16_t x, uint16_t y, const uint8_t *image_data);

#endif /* __GUI_H */