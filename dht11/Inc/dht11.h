/**
 * @file dht11.h
 * @brief DHT11温湿度传感器驱动头文件
 * @author Weather Station Project
 * @date 2024
 * @version 1.0
 * 
 * DHT11是一款数字温湿度传感器，通过单总线协议与MCU通信
 * 测量范围：温度0-50°C，湿度20-90%RH
 * 精度：温度±2°C，湿度±5%RH
 */

#ifndef DHT11_H
#define DHT11_H

// 包含STM32主头文件，提供GPIO和HAL库支持
#include "main.h"
// 包含标准布尔类型定义
#include <stdbool.h>
// 包含标准整型定义
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置宏定义 ==================== */

/**
 * @brief DHT11数据引脚GPIO端口定义
 * @note 可根据实际硬件连接修改，默认使用GPIOA
 */
#define DHT_GPIO_Port   GPIOA

/**
 * @brief DHT11数据引脚定义
 * @note 可根据实际硬件连接修改，默认使用PA6
 *       换引脚时只需修改这里即可
 */
#define DHT_Pin         GPIO_PIN_6

/* ==================== 函数声明 ==================== */

/**
 * @brief 初始化DHT11传感器
 * @param 无
 * @return bool 初始化结果
 * @retval true  初始化成功
 * @retval false 初始化失败
 * @note 该函数会初始化DWT延时功能，配置GPIO引脚，并进行传感器稳定等待
 */
bool DHT11_Init(void);

/**
 * @brief 读取DHT11温湿度数据
 * @param humidity    指向湿度变量的指针，用于返回湿度值(%)
 * @param temperature 指向温度变量的指针，用于返回温度值(°C)
 * @return bool 读取结果
 * @retval true  读取成功，数据有效
 * @retval false 读取失败，数据无效
 * @note 读取过程约18ms，函数内部会进行数据校验
 */
bool DHT11_Read(int *humidity, int *temperature);

/**
 * @brief 初始化DWT(Data Watchpoint and Trace)单元用于微秒级延时
 * @param 无
 * @return 无
 * @note DWT是ARM Cortex-M内核的调试单元，可用于精确计时
 *       该函数只需调用一次，通常在系统初始化时调用
 */
void DWT_Delay_Init(void);

/**
 * @brief 微秒级精确延时函数
 * @param us 延时时间，单位为微秒(μs)
 * @return 无
 * @note 基于DWT计数器实现，比HAL_Delay更精确
 *       适用于DHT11通信时序要求
 */
void DWT_Delay_us(uint32_t us);
void DHT11_SendStartSignal(void);//DHT11发送开始信号
bool DHT11_WaitForResponse(void);//等待DHT11响应信号
void DHT11_ReadData(uint8_t *data);//读取DHT11数据

#ifdef __cplusplus
}
#endif

#endif // DHT11_H