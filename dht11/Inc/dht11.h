#ifndef DHT11_H
#define DHT11_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 选择数据脚
#define DHT_GPIO_Port   GPIOA
#define DHT_Pin         GPIO_PIN_6   // 换脚只改这里

bool DHT11_Init(void);
bool DHT11_Read(int *humidity, int *temperature);

// 初始化一次 DWT，用于微秒延时
void DWT_Delay_Init(void);
void DWT_Delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif // DHT11_H