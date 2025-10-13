#ifndef __ESP32_WEATHER_H__
#define __ESP32_WEATHER_H__

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// WiFi配置宏定义 - 用户可修改
#define WIFI_SSID     "Niceday"     // 修改为你的WiFi名称
#define WIFI_PASSWORD "178536586471" // 修改为你的WiFi密码

// 缓冲区大小定义
#define WIFI_RX_BUFFER_SIZE 1024
#define WEATHER_DATA_SIZE   512

// 全局天气数据数组 - 方便外部调用
extern char WeatherInfo[10][64];  // 天气信息数组

// 天气数据索引定义
#define WEATHER_CITY        0  // 城市
#define WEATHER_CONDITION   1  // 天气状况
#define WEATHER_TEMPERATURE 2  // 温度
#define WEATHER_HUMIDITY    3  // 湿度
#define WEATHER_WIND        4  // 风力
#define WEATHER_PRESSURE    5  // 气压
#define WEATHER_VISIBILITY  6  // 能见度
#define WEATHER_UPDATE_TIME 7  // 更新时间
#define WEATHER_DESCRIPTION 8  // 详细描述
#define WEATHER_STATUS      9  // 连接状态

// 函数声明
uint8_t ESP32_Init(void);
uint8_t ESP32_ConnectWiFi(void);
uint8_t ESP32_GetWeather(void);
uint8_t ESP32_SendCmd_WithResponse(char *cmd, char *expected, uint32_t timeout);
void ESP32_ParseWeatherData(char *raw_data);
char WIFI_Config(int time, char *cmd, char *response);
void u1_printf(char* fmt, ...);

#endif /* __ESP32_WEATHER_H__ */