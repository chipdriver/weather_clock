#ifndef __ESP32_WEATHER_H__
#define __ESP32_WEATHER_H__

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "usart.h"

// WiFi配置宏定义
#define WIFI_SSID     "Niceday"     //wifi名称
#define WIFI_PASSWORD "17853658647" //WiFi密码

#define RXBUFFER 512  // 接收缓冲区大小

uint8_t AT_SendAndWait(const char *cmd, const char *expect, uint32_t timeout_ms);
void wifi_connect(void);
uint8_t AT_SendFormatAndWait(const char *expect, uint32_t timeout_ms, const char *format, ...);
void get_weather(void);
void parse_weather_json(void);
void receive_weather_data_immediate(void);
void get_time(void);
void parse_time_data(void);
#endif /* __ESP32_WEATHER_H__ */