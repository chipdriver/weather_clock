/**
 * @file    font.h
 * @brief   字体数据定义文件 - 包含ASCII字符和中文字符的字模数据
 * @details 本文件定义了LCD显示用的字体数据，包括：
 *          - 8x16像素的ASCII字符字模数据
 *          - 16x16像素的中文字符字模数据  
 *          - 24x24像素的中文字符字模数据
 * @author  STM32项目开发团队
 * @date    2024年
 * @version V1.0
 */

#ifndef __FONT_H__
#define __FONT_H__

/**
 * @brief 字体存储位置控制宏
 * @note  1=使用片内Flash存储字体数据，0=使用外部存储器
 */
#define USE_ONCHIP_FLASH_FONT 1

/**
 * @brief 16x16像素中文字符结构体定义
 * @details 用于存储单个中文字符的字模数据
 */
struct typFNT_GB162
{
       const char *Index;  /**< 字符索引，指向中文字符字符串 */
       char Msk[32];       /**< 字模数据，16x16像素=32字节 */
};

/**
 * @brief 24x24像素中文字符结构体定义
 * @details 用于存储单个大号中文字符的字模数据
 */
struct typFNT_GB242
{
       const char *Index;  /**< 字符索引，指向中文字符字符串 */
       char Msk[72];       /**< 字模数据，24x24像素=72字节 */
};

/**
 * @brief 字体数据数组外部声明
 */
extern const unsigned char asc16[];                    /**< ASCII字符字模数据数组 (8x16像素) */
extern const unsigned char sz32[];                     /**< 32x32像素数字字体数据数组 */
extern const struct typFNT_GB162 hz16[];              /**< 16x16中文字符字模数据数组 */
extern const struct typFNT_GB242 hz24[];              /**< 24x24中文字符字模数据数组 */
extern const unsigned char gImage_weather[3208];       /**< 天气图片数据 (40x40像素) */
extern const unsigned char gImage_humi[908] ;          /**< 湿度图片数据 (40x40像素) */
extern const unsigned char gImage_temp[558];          /**< 温度图片数据 (40x40像素) */
extern const unsigned char gImage_clock[2458];        /**< 时钟图片数据 (40x40像素) */
/**
 * @brief 字体数量定义
 */
#define hz16_num   2    /* 当前已定义的16x16中文字符数量 */
#define hz24_num   1    /* 当前已定义的24x24中文字符数量 */

/* 
 * 使用说明：
 * 1. ASCII字符使用asc16[]数组，支持8x16像素显示
 * 2. 中文字符使用hz16[]或hz24[]数组，分别支持16x16和24x24像素显示
 * 3. 添加新字符时，需要同时更新对应的_num宏定义
 * 4. 字模数据通过字模软件生成，按行扫描方式存储
 */

#endif /* __FONT_H__ */
