/**
 * @file dht11.h
 * @brief DHT11温湿度传感器驱动头文件
 * @author Chipdriver
 * @date 2025
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

/*宏定义*/
#define DHT_GPIO_Port   GPIOA
#define DHT_Pin         GPIO_PIN_6

/*函数声明*/
void DWT_Delay_Init(void);//初始化DWT(Data Watchpoint and Trace)单元用于微秒级延时
void DWT_Delay_us(uint32_t us);//微秒级精确延时函数

bool DHT11_Init(void);  //DHT11初始化函数
bool DHT11_Read(int *humidity, int *temperature);   //读取DHT11温湿度数据函数

void DHT11_SendStartSignal(void);//DHT11发送开始信号
bool DHT11_WaitForResponse(void);//等待DHT11响应信号
void DHT11_ReadData(uint8_t *data);//读取DHT11数据

#ifdef __cplusplus
}
#endif

#endif // DHT11_H