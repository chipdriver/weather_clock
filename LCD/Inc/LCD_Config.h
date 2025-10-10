/**
 ******************************************************************************
 * @file           : LCD_Config.h
 * @brief          : LCD配置参数头文件
 *                   定义LCD屏幕的基本参数和硬件配置常量
 * @author         : STM32 Weather Clock Project
 * @version        : V1.0
 * @date           : 2025-01-10
 ******************************************************************************
 */

#ifndef __LCD_CONFIG_H
#define __LCD_CONFIG_H

// ST7735R 1.44寸屏幕分辨率参数
#define X_MAX_PIXEL	        128     // 屏幕宽度（像素）
#define Y_MAX_PIXEL	        160     // 屏幕高度（像素）

// 颜色深度配置
#define LCD_COLOR_DEPTH     16      // 16位颜色深度

#endif /* __LCD_CONFIG_H */
