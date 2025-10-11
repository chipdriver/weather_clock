#ifndef __ESP32_WEATHER_H__
#define __ESP32_WEATHER_H__


/*头文件*/
#include "main.h"
#include <string.h>
#include <stdio.h>

// 天气数据结构体
typedef struct {
    char city[32];          /* 城市名称 */
    char weather[32];       /* 天气状况 */
    float temperature;      /* 温度 */
    int humidity;           /* 湿度 */
    char description[64];   /* 天气描述 */
} WeatherData_t;

// 函数声明
void ESP32_SendCmd(char *cmd);
uint8_t ESP32_SendCmd_WithResponse(char *cmd, char *expected, uint32_t timeout);
uint8_t ESP32_ConnectToWiFi(void);
void ESP32_GetWeather(void);
void ESP32_GetTime(void);
void ESP32_ConnectWiFi(void);

#endif /* __ESP32_WEATHER_H__ */