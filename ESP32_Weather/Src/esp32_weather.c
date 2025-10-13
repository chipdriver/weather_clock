#include "esp32_weather.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"

extern UART_HandleTypeDef huart1;  // ESP32通信
extern UART_HandleTypeDef huart6;  // 调试输出

// 全局天气信息数组 - 外部可以访问
char WeatherInfo[10][64] = {0};

// WiFi接收缓冲区
char WiFi_RX_BUF[WIFI_RX_BUFFER_SIZE] = {0};
int WiFi_RxCounter = 0;

/**
 * @brief 发送AT命令并等待响应
 * @param cmd: AT命令
 * @param expected: 期望的响应内容
 * @param timeout: 超时时间(ms)
 * @return 1-成功, 0-失败
 */
uint8_t ESP32_SendCmd_WithResponse(char *cmd, char *expected, uint32_t timeout)
{
    char buf[256];
    char rx_buf[512] = {0};
    
    // 构造命令
    sprintf(buf, "%s\r\n", cmd);
    
    // 调试输出
    HAL_UART_Transmit(&huart6, (uint8_t *)"[TX] ", 5, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)buf, strlen(buf), 1000);
    
    // 发送命令到ESP32
    HAL_StatusTypeDef result = HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 1000);
    if (result != HAL_OK) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"[ERROR] Send failed\r\n", 21, 1000);
        return 0;
    }
    
    // 等待响应
    HAL_Delay(100);
    
    // 接收响应
    uint32_t start_time = HAL_GetTick();
    uint16_t rx_index = 0;
    
    while ((HAL_GetTick() - start_time) < timeout && rx_index < sizeof(rx_buf)-1) {
        uint8_t received_byte;
        if (HAL_UART_Receive(&huart1, &received_byte, 1, 10) == HAL_OK) {
            rx_buf[rx_index++] = received_byte;
            rx_buf[rx_index] = '\0';
            
            // 检查是否收到期望的响应
            if (expected && strstr(rx_buf, expected)) {
                HAL_UART_Transmit(&huart6, (uint8_t *)"[RX] ", 5, 1000);
                HAL_UART_Transmit(&huart6, (uint8_t *)rx_buf, strlen(rx_buf), 1000);
                HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, 1000);
                return 1;
            }
        }
    }
    
    // 输出收到的内容（即使没有匹配到期望值）
    if (rx_index > 0) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"[RX] ", 5, 1000);
        HAL_UART_Transmit(&huart6, (uint8_t *)rx_buf, strlen(rx_buf), 1000);
        HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, 1000);
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t *)"[TIMEOUT] No response\r\n", 23, 1000);
    }
    
    return 0;
}

/**
 * @brief 连接WiFi
 * @return 1-成功, 0-失败
 */
uint8_t ESP32_ConnectWiFi(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== ESP32 WiFi Connection ===\r\n", 32, 1000);
    
    // 1. 测试ESP32连接
    if (!ESP32_SendCmd_WithResponse("AT", "OK", 2000)) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"ESP32 not responding!\r\n", 23, 1000);
        strcpy(WeatherInfo[WEATHER_STATUS], "ESP32 FAILED");
        return 0;
    }
    
    // 2. 设置WiFi模式为Station模式
    ESP32_SendCmd_WithResponse("AT+CWMODE=1", "OK", 2000);
    
    // 3. 连接WiFi - 使用宏定义的WiFi名称和密码
    char wifi_cmd[128];
    sprintf(wifi_cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    
    if (ESP32_SendCmd_WithResponse(wifi_cmd, "WIFI GOT IP", 15000)) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"WiFi connected successfully!\r\n", 30, 1000);
        strcpy(WeatherInfo[WEATHER_STATUS], "WiFi Connected");
        
        // 4. 查询IP地址
        ESP32_SendCmd_WithResponse("AT+CIFSR", "OK", 3000);
        
        return 1;
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t *)"WiFi connection failed!\r\n", 25, 1000);
        strcpy(WeatherInfo[WEATHER_STATUS], "WiFi Failed");
        return 0;
    }
}

/**
 * @brief 获取天气数据
 * @return 1-成功, 0-失败
 */
uint8_t ESP32_GetWeather(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== Getting Weather Data ===\r\n", 31, 1000);
    
    // 1. 建立TCP连接到天气API服务器
    if (!ESP32_SendCmd_WithResponse("AT+CIPSTART=\"TCP\",\"wttr.in\",80", "OK", 10000)) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"Failed to connect to weather server\r\n", 37, 1000);
        strcpy(WeatherInfo[WEATHER_STATUS], "Server Failed");
        return 0;
    }
    
    // 2. 准备HTTP请求 - 获取北京天气，格式化为简单文本
    char http_request[] = "GET /Beijing?format=%l:+%c+%t+%h+%w HTTP/1.1\r\n"
                         "Host: wttr.in\r\n"
                         "User-Agent: STM32WeatherClock\r\n"
                         "Connection: close\r\n\r\n";
    
    char send_cmd[256];
    sprintf(send_cmd, "AT+CIPSEND=%d", (int)strlen(http_request));
    
    if (ESP32_SendCmd_WithResponse(send_cmd, ">", 3000)) {
        // 发送HTTP请求数据
        HAL_UART_Transmit(&huart1, (uint8_t *)http_request, strlen(http_request), 5000);
        HAL_UART_Transmit(&huart6, (uint8_t *)"HTTP request sent!\r\n", 20, 1000);
        
        // 3. 接收天气数据
        HAL_Delay(3000);  // 等待服务器响应
        
        char weather_data[1024] = {0};
        uint16_t data_index = 0;
        uint32_t start_time = HAL_GetTick();
        
        while ((HAL_GetTick() - start_time) < 10000 && data_index < sizeof(weather_data)-1) {
            uint8_t received_byte;
            if (HAL_UART_Receive(&huart1, &received_byte, 1, 100) == HAL_OK) {
                weather_data[data_index++] = received_byte;
            }
        }
        
        if (data_index > 0) {
            weather_data[data_index] = '\0';
            HAL_UART_Transmit(&huart6, (uint8_t *)"Weather data received!\r\n", 24, 1000);
            
            // 解析天气数据
            ESP32_ParseWeatherData(weather_data);
            
            strcpy(WeatherInfo[WEATHER_STATUS], "Data Updated");
        } else {
            HAL_UART_Transmit(&huart6, (uint8_t *)"No weather data received\r\n", 26, 1000);
            strcpy(WeatherInfo[WEATHER_STATUS], "No Data");
        }
    }
    
    // 4. 关闭连接
    ESP32_SendCmd_WithResponse("AT+CIPCLOSE", "OK", 3000);
    
    return 1;
}

/**
 * @brief 解析天气数据到WeatherInfo数组
 * @param raw_data: 原始天气数据
 */
void ESP32_ParseWeatherData(char *raw_data)
{
    // 查找HTTP响应体（在"\r\n\r\n"之后）
    char *weather_start = strstr(raw_data, "\r\n\r\n");
    if (weather_start) {
        weather_start += 4; // 跳过HTTP头
        
        // wttr.in返回的格式类似：Beijing: ⛅ +22°C 65% 10km/h
        // 解析这个格式
        
        // 提取城市名称
        if (strstr(weather_start, "Beijing:")) {
            strcpy(WeatherInfo[WEATHER_CITY], "Beijing");
        }
        
        // 提取温度 (查找 +XX°C 或 -XX°C 格式)
        char *temp_pos = strstr(weather_start, "°C");
        if (temp_pos) {
            // 向前查找数字开始位置
            char *temp_start = temp_pos - 1;
            while (temp_start > weather_start && (*temp_start >= '0' && *temp_start <= '9')) {
                temp_start--;
            }
            if (*temp_start == '+' || *temp_start == '-') {
                // 找到温度，复制到数组
                int temp_len = temp_pos - temp_start + 2; // 包含°C
                if (temp_len < 64) {
                    strncpy(WeatherInfo[WEATHER_TEMPERATURE], temp_start, temp_len);
                    WeatherInfo[WEATHER_TEMPERATURE][temp_len] = '\0';
                }
            }
        }
        
        // 提取湿度 (查找 XX% 格式)
        char *humidity_pos = strstr(weather_start, "%");
        if (humidity_pos) {
            char *humidity_start = humidity_pos - 1;
            while (humidity_start > weather_start && (*humidity_start >= '0' && *humidity_start <= '9')) {
                humidity_start--;
            }
            humidity_start++; // 指向数字开始
            int humidity_len = humidity_pos - humidity_start + 1; // 包含%
            if (humidity_len < 64) {
                strncpy(WeatherInfo[WEATHER_HUMIDITY], humidity_start, humidity_len);
                WeatherInfo[WEATHER_HUMIDITY][humidity_len] = '\0';
            }
        }
        
        // 提取天气状况 (emoji或文字描述)
        // 简单处理：查找常见天气词汇
        if (strstr(weather_start, "☀") || strstr(weather_start, "sunny")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Sunny");
        } else if (strstr(weather_start, "☁") || strstr(weather_start, "cloudy")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Cloudy");
        } else if (strstr(weather_start, "⛅") || strstr(weather_start, "partly")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Partly Cloudy");
        } else if (strstr(weather_start, "🌧") || strstr(weather_start, "rain")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Rainy");
        } else if (strstr(weather_start, "⛈") || strstr(weather_start, "storm")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Stormy");
        } else if (strstr(weather_start, "🌨") || strstr(weather_start, "snow")) {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Snowy");
        } else {
            strcpy(WeatherInfo[WEATHER_CONDITION], "Unknown");
        }
        
        // 存储更新时间
        strcpy(WeatherInfo[WEATHER_UPDATE_TIME], "Just Now");
        
        // 输出解析结果到调试串口
        HAL_UART_Transmit(&huart6, (uint8_t *)"=== Parsed Weather ===\r\n", 25, 1000);
        
        char debug_msg[100];
        sprintf(debug_msg, "City: %s\r\n", WeatherInfo[WEATHER_CITY]);
        HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        
        sprintf(debug_msg, "Condition: %s\r\n", WeatherInfo[WEATHER_CONDITION]);
        HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        
        sprintf(debug_msg, "Temperature: %s\r\n", WeatherInfo[WEATHER_TEMPERATURE]);
        HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        
        sprintf(debug_msg, "Humidity: %s\r\n", WeatherInfo[WEATHER_HUMIDITY]);
        HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    }
}

/**
 * @brief ESP32初始化
 * @return 1-成功, 0-失败
 */
uint8_t ESP32_Init(void)
{
    // 清空天气信息数组
    for (int i = 0; i < 10; i++) {
        memset(WeatherInfo[i], 0, 64);
    }
    
    // 设置初始状态
    strcpy(WeatherInfo[WEATHER_STATUS], "Initializing");
    strcpy(WeatherInfo[WEATHER_CITY], "Unknown");
    strcpy(WeatherInfo[WEATHER_CONDITION], "Unknown");
    strcpy(WeatherInfo[WEATHER_TEMPERATURE], "--°C");
    strcpy(WeatherInfo[WEATHER_HUMIDITY], "--%");
    
    return 1;
}

/**
 * @brief WiFi配置函数 - 兼容旧版本
 * @param time: 等待时间（单位：100ms）
 * @param cmd: AT命令
 * @param response: 期望响应
 * @return 0-成功, 1-失败
 */
char WIFI_Config(int time, char *cmd, char *response) 
{
    memset(WiFi_RX_BUF, 0, WIFI_RX_BUFFER_SIZE);
    WiFi_RxCounter = 0;
    
    // 发送命令
    u1_printf("%s\r\n", cmd);
    
    while (time--) {
        HAL_Delay(100);
        
        // 尝试接收数据
        uint8_t temp_buf[64] = {0};
        if(HAL_UART_Receive(&huart1, temp_buf, 64, 50) == HAL_OK) {
            strncat(WiFi_RX_BUF, (char*)temp_buf, strlen((char*)temp_buf));
            WiFi_RxCounter += strlen((char*)temp_buf);
        }
        
        // 检查是否收到期望的响应
        if (strstr((char *)WiFi_RX_BUF, response)) {
            break;
        }
    }
    
    if (time > 0) {
        return 0;  // 成功
    } else {
        return 1;  // 失败
    }
}

