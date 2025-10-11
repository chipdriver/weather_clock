#include "esp32_weather.h"
#include "GUI.h"
#include "font.h"
#include "lcd_Driver.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"

extern UART_HandleTypeDef huart1;  // ESP32通信
extern UART_HandleTypeDef huart6;  // 调试输出

// 如果UART1有问题，尝试使用UART6进行ESP32通信
#define USE_UART6_FOR_ESP32 1  // 设为1使用UART6，设为0使用UART1

#if USE_UART6_FOR_ESP32
    #define ESP32_UART huart6
    #define DEBUG_UART huart1
    #warning "Using UART6 for ESP32 communication, UART1 for debug"
#else
    #define ESP32_UART huart1
    #define DEBUG_UART huart6
#endif

// 发送AT命令并等待响应
uint8_t ESP32_SendCmd_WithResponse(char *cmd, char *expected, uint32_t timeout)
{
    char buf[256];
    char rx_buf[512] = {0};
    
    // 构造命令
    sprintf(buf, "%s\r\n", cmd);
    
    // 调试输出
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"[TX] ", 5, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)buf, strlen(buf), 1000);
    
    // 发送命令
    HAL_StatusTypeDef result = HAL_UART_Transmit(&ESP32_UART, (uint8_t *)buf, strlen(buf), 1000);
    if (result != HAL_OK) {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"[ERROR] Send failed\r\n", 21, 1000);
        return 0;
    }
    
    // 等待响应
    HAL_Delay(100);
    
    // 接收响应
    uint32_t start_time = HAL_GetTick();
    uint16_t rx_index = 0;
    
    while ((HAL_GetTick() - start_time) < timeout && rx_index < sizeof(rx_buf)-1) {
        uint8_t received_byte;
        if (HAL_UART_Receive(&ESP32_UART, &received_byte, 1, 10) == HAL_OK) {
            rx_buf[rx_index++] = received_byte;
            rx_buf[rx_index] = '\0';
            
            // 检查是否收到期望的响应
            if (expected && strstr(rx_buf, expected)) {
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"[RX] ", 5, 1000);
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)rx_buf, strlen(rx_buf), 1000);
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\n", 2, 1000);
                return 1;
            }
        }
    }
    
    // 输出收到的内容（即使没有匹配到期望值）
    if (rx_index > 0) {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"[RX] ", 5, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)rx_buf, strlen(rx_buf), 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\n", 2, 1000);
    } else {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"[TIMEOUT] No response\r\n", 23, 1000);
    }
    
    return 0;
}

// 连接WiFi
uint8_t ESP32_ConnectToWiFi(void)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"=== Connecting to WiFi ===\r\n", 29, 1000);
    
    // 1. 测试ESP32连接
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Step 1: Testing ESP32...\r\n", 26, 1000);
    if (!ESP32_SendCmd_WithResponse("AT", "OK", 2000)) {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"ESP32 not responding!\r\n", 23, 1000);
        return 0;
    }
    
    // 2. 设置WiFi模式
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Step 2: Setting WiFi mode...\r\n", 30, 1000);
    ESP32_SendCmd_WithResponse("AT+CWMODE=1", "OK", 2000);
    
    // 3. 连接WiFi
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Step 3: Connecting to Niceday...\r\n", 34, 1000);
    char wifi_cmd[128];
    sprintf(wifi_cmd, "AT+CWJAP=\"Niceday\",\"17853658647\"");
    
    if (ESP32_SendCmd_WithResponse(wifi_cmd, "WIFI GOT IP", 15000)) {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"WiFi connected successfully!\r\n", 30, 1000);
        
        // 4. 查询IP地址
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Step 4: Getting IP address...\r\n", 31, 1000);
        ESP32_SendCmd_WithResponse("AT+CIFSR", "OK", 3000);
        
        return 1;
    } else {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"WiFi connection failed!\r\n", 25, 1000);
        return 0;
    }
}

// 获取天气数据 - 使用免费的天气API
void ESP32_GetWeather(void)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"=== Getting Weather Data ===\r\n", 31, 1000);
    
    // 1. 建立TCP连接到天气API服务器
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Connecting to weather server...\r\n", 33, 1000);
    
    // 使用免费的天气API - wttr.in (无需API密钥)
    if (ESP32_SendCmd_WithResponse("AT+CIPSTART=\"TCP\",\"wttr.in\",80", "OK", 10000)) {
        
        // 2. 发送HTTP GET请求
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Sending HTTP request...\r\n", 25, 1000);
        
        // 准备HTTP请求 - 获取北京天气，格式化为简单文本
        char http_request[] = "GET /Beijing?format=%l:+%c+%t+%h+%w HTTP/1.1\r\n"
                             "Host: wttr.in\r\n"
                             "User-Agent: ESP32WeatherClock\r\n"
                             "Connection: close\r\n\r\n";
        
        char send_cmd[256];
        sprintf(send_cmd, "AT+CIPSEND=%d", (int)strlen(http_request));
        
        if (ESP32_SendCmd_WithResponse(send_cmd, ">", 3000)) {
            // 发送HTTP请求数据
            HAL_UART_Transmit(&ESP32_UART, (uint8_t *)http_request, strlen(http_request), 5000);
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"HTTP request sent!\r\n", 20, 1000);
            
            // 3. 接收天气数据
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Receiving weather data...\r\n", 27, 1000);
            HAL_Delay(3000);  // 等待服务器响应
            
            // 接收数据
            char weather_data[1024] = {0};
            uint16_t data_index = 0;
            uint32_t start_time = HAL_GetTick();
            
            while ((HAL_GetTick() - start_time) < 10000 && data_index < sizeof(weather_data)-1) {
                uint8_t received_byte;
                if (HAL_UART_Receive(&ESP32_UART, &received_byte, 1, 100) == HAL_OK) {
                    weather_data[data_index++] = received_byte;
                }
            }
            
            if (data_index > 0) {
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Weather data received:\r\n", 24, 1000);
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)weather_data, data_index, 5000);
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\n", 2, 1000);
                
                // 简单解析天气数据
                char *weather_start = strstr(weather_data, "\r\n\r\n");
                if (weather_start) {
                    weather_start += 4; // 跳过HTTP头
                    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Parsed weather: ", 16, 1000);
                    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)weather_start, strlen(weather_start), 1000);
                    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\n", 2, 1000);
                }
            } else {
                HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"No weather data received\r\n", 26, 1000);
            }
        }
        
        // 4. 关闭连接
        ESP32_SendCmd_WithResponse("AT+CIPCLOSE", "OK", 3000);
    } else {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Failed to connect to weather server\r\n", 37, 1000);
    }
}

// 获取网络时间
void ESP32_GetTime(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== Getting Network Time ===\r\n", 31, 1000);
    
    // 建立TCP连接到时间服务器
    if (ESP32_SendCmd_WithResponse("AT+CIPSTART=\"TCP\",\"worldtimeapi.org\",80", "OK", 10000)) {
        
        // 发送HTTP请求获取北京时间
        char http_request[] = "GET /api/timezone/Asia/Shanghai HTTP/1.1\r\n"
                             "Host: worldtimeapi.org\r\n"
                             "Connection: close\r\n\r\n";
        
        char send_cmd[256];
        sprintf(send_cmd, "AT+CIPSEND=%d", (int)strlen(http_request));
        
        if (ESP32_SendCmd_WithResponse(send_cmd, ">", 3000)) {
            HAL_UART_Transmit(&huart1, (uint8_t *)http_request, strlen(http_request), 5000);
            HAL_UART_Transmit(&huart6, (uint8_t *)"Time request sent!\r\n", 20, 1000);
            
            // 接收时间数据
            char time_data[1024] = {0};
            uint16_t data_index = 0;
            uint32_t start_time = HAL_GetTick();
            
            while ((HAL_GetTick() - start_time) < 10000 && data_index < sizeof(time_data)-1) {
                uint8_t received_byte;
                if (HAL_UART_Receive(&huart1, &received_byte, 1, 100) == HAL_OK) {
                    time_data[data_index++] = received_byte;
                }
            }
            
            if (data_index > 0) {
                HAL_UART_Transmit(&huart6, (uint8_t *)"Time data received:\r\n", 21, 1000);
                HAL_UART_Transmit(&huart6, (uint8_t *)time_data, data_index, 5000);
                HAL_UART_Transmit(&huart6, (uint8_t *)"\r\n", 2, 1000);
            }
        }
        
        ESP32_SendCmd_WithResponse("AT+CIPCLOSE", "OK", 3000);
    }
}

// 向ESP32发送AT命令（保持兼容性）
void ESP32_SendCmd(char *cmd)
{
    ESP32_SendCmd_WithResponse(cmd, NULL, 2000);
}

// 硬件诊断函数
void ESP32_HardwareDiagnosis(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== ESP32 Hardware Diagnosis ===\r\n", 35, 1000);
    
    // 1. 检查UART配置
    char debug_msg[100];
    sprintf(debug_msg, "UART1 Instance: %p\r\n", (void*)huart1.Instance);
    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    
    sprintf(debug_msg, "UART1 BaudRate: %lu\r\n", huart1.Init.BaudRate);
    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    
    sprintf(debug_msg, "UART1 State: %d\r\n", huart1.gState);
    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    
    // 2. 检查GPIO配置
    uint32_t pa9_mode = (GPIOA->MODER >> (9*2)) & 0x3;
    uint32_t pa10_mode = (GPIOA->MODER >> (10*2)) & 0x3;
    sprintf(debug_msg, "PA9 Mode: %lu (should be 2=AF)\r\n", pa9_mode);
    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    
    sprintf(debug_msg, "PA10 Mode: %lu (should be 2=AF)\r\n", pa10_mode);
    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
    
    // 3. 测试不同波特率
    HAL_UART_Transmit(&huart6, (uint8_t *)"Testing different baudrates...\r\n", 32, 1000);
    
    uint32_t test_rates[] = {9600, 38400, 57600, 115200};
    uint32_t original_baud = huart1.Init.BaudRate;
    
    for (int i = 0; i < 4; i++) {
        sprintf(debug_msg, "Testing %lu baud...\r\n", test_rates[i]);
        HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        
        // 重新配置波特率
        huart1.Init.BaudRate = test_rates[i];
        if (HAL_UART_Init(&huart1) == HAL_OK) {
            
            // 发送AT命令
            HAL_UART_Transmit(&huart1, (uint8_t *)"AT\r\n", 4, 1000);
            HAL_Delay(500);
            
            // 尝试接收
            char rx_buf[32] = {0};
            if (HAL_UART_Receive(&huart1, (uint8_t *)rx_buf, 31, 2000) == HAL_OK) {
                if (strlen(rx_buf) > 0) {
                    sprintf(debug_msg, "SUCCESS at %lu: [%s]\r\n", test_rates[i], rx_buf);
                    HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
                    
                    // 恢复这个波特率继续使用
                    return;
                }
            }
        }
    }
    
    // 恢复原始波特率
    huart1.Init.BaudRate = original_baud;
    HAL_UART_Init(&huart1);
    
    HAL_UART_Transmit(&huart6, (uint8_t *)"No response at any baudrate!\r\n", 30, 1000);
    
    // 4. 给出详细的检查建议
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== TROUBLESHOOTING GUIDE ===\r\n", 32, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"Please check the following:\r\n", 29, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"1. Physical connections:\r\n", 26, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"   STM32 PA9 (TX)  -> ESP32 GPIO7 (RX)\r\n", 40, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"   STM32 PA10(RX)  <- ESP32 GPIO6 (TX)\r\n", 40, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"   STM32 GND       -> ESP32 GND\r\n", 33, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"   STM32 3.3V      -> ESP32 3.3V\r\n", 34, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"2. ESP32 power: Check 3.3V with multimeter\r\n", 45, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"3. ESP32 firmware: Should run AT firmware\r\n", 43, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"4. Wire continuity: Test with multimeter\r\n", 42, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"5. Try direct connection: USB-TTL to ESP32\r\n", 44, 1000);
}

// GPIO引脚测试
void ESP32_GPIOTest(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== GPIO Pin Test ===\r\n", 24, 1000);
    
    // 临时将PA9配置为GPIO输出，PA10配置为GPIO输入
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 先禁用UART1
    HAL_UART_DeInit(&huart1);
    
    // 配置PA9为GPIO输出
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置PA10为GPIO输入
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_UART_Transmit(&huart6, (uint8_t *)"Testing PA9->PA10 connection...\r\n", 34, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"Please connect PA9 to PA10\r\n", 28, 1000);
    HAL_Delay(2000);
    
    // 测试高电平
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(100);
    GPIO_PinState pin_state_high = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10);
    
    // 测试低电平
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(100);
    GPIO_PinState pin_state_low = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10);
    
    // 输出测试结果
    char test_result[100];
    sprintf(test_result, "PA9 HIGH -> PA10: %s\r\n", pin_state_high ? "HIGH" : "LOW");
    HAL_UART_Transmit(&huart6, (uint8_t *)test_result, strlen(test_result), 1000);
    
    sprintf(test_result, "PA9 LOW  -> PA10: %s\r\n", pin_state_low ? "HIGH" : "LOW");
    HAL_UART_Transmit(&huart6, (uint8_t *)test_result, strlen(test_result), 1000);
    
    if (pin_state_high == GPIO_PIN_SET && pin_state_low == GPIO_PIN_RESET) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"GPIO TEST PASSED - Pins working!\r\n", 35, 1000);
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t *)"GPIO TEST FAILED - Check connections\r\n", 38, 1000);
    }
    
    // 恢复UART1配置
    MX_USART1_UART_Init();
    HAL_UART_Transmit(&huart6, (uint8_t *)"UART1 restored\r\n", 17, 1000);
}

// 回环测试
void ESP32_LoopbackTest(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== UART Loopback Test ===\r\n", 29, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"Connect PA9 to PA10 for this test\r\n", 36, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t *)"Testing in 3 seconds...\r\n", 25, 1000);
    HAL_Delay(3000);
    
    // 发送测试数据
    char test_msg[] = "LOOPBACK_TEST_123";
    HAL_UART_Transmit(&huart1, (uint8_t *)test_msg, strlen(test_msg), 1000);
    
    // 尝试接收
    char rx_buf[32] = {0};
    if (HAL_UART_Receive(&huart1, (uint8_t *)rx_buf, sizeof(rx_buf)-1, 2000) == HAL_OK) {
        if (strcmp(rx_buf, test_msg) == 0) {
            HAL_UART_Transmit(&huart6, (uint8_t *)"LOOPBACK TEST PASSED - STM32 UART OK\r\n", 39, 1000);
        } else {
            HAL_UART_Transmit(&huart6, (uint8_t *)"LOOPBACK TEST PARTIAL - Data corrupted\r\n", 41, 1000);
        }
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t *)"LOOPBACK TEST FAILED - Check UART config\r\n", 43, 1000);
    }
}

// UART寄存器详细检查
void ESP32_UARTRegisterCheck(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== UART Register Check ===\r\n", 30, 1000);
    
    char reg_info[100];
    
    // 检查RCC寄存器 - USART1时钟是否使能
    sprintf(reg_info, "RCC_APB2ENR: 0x%08lX (bit4=%s)\r\n", 
            RCC->APB2ENR, (RCC->APB2ENR & RCC_APB2ENR_USART1EN) ? "ON" : "OFF");
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    // 检查USART1寄存器
    sprintf(reg_info, "USART1->CR1: 0x%08lX\r\n", USART1->CR1);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    sprintf(reg_info, "USART1->CR2: 0x%08lX\r\n", USART1->CR2);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    sprintf(reg_info, "USART1->SR:  0x%08lX\r\n", USART1->SR);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    sprintf(reg_info, "USART1->BRR: 0x%08lX\r\n", USART1->BRR);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    // 检查GPIO复用功能寄存器
    sprintf(reg_info, "GPIOA->AFR[1]: 0x%08lX\r\n", GPIOA->AFR[1]);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    // PA9和PA10应该配置为AF7 (USART1)
    uint32_t pa9_af = (GPIOA->AFR[1] >> ((9-8)*4)) & 0xF;
    uint32_t pa10_af = (GPIOA->AFR[1] >> ((10-8)*4)) & 0xF;
    
    sprintf(reg_info, "PA9 AF: %lu (should be 7)\r\n", pa9_af);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
    
    sprintf(reg_info, "PA10 AF: %lu (should be 7)\r\n", pa10_af);
    HAL_UART_Transmit(&huart6, (uint8_t *)reg_info, strlen(reg_info), 1000);
}

// 强制重新初始化UART1
void ESP32_ForceUARTInit(void)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)"=== Force UART1 Reinitialization ===\r\n", 39, 1000);
    
    // 1. 完全去初始化UART1
    HAL_UART_DeInit(&huart1);
    HAL_Delay(100);
    
    // 2. 手动配置GPIO引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PA9配置为UART1_TX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA10配置为UART1_RX
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 3. 重新初始化UART1
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        HAL_UART_Transmit(&huart6, (uint8_t *)"UART1 init failed!\r\n", 20, 1000);
        return;
    }
    
    HAL_UART_Transmit(&huart6, (uint8_t *)"UART1 reinitialized successfully!\r\n", 36, 1000);
    
    // 4. 立即测试回环
    HAL_UART_Transmit(&huart6, (uint8_t *)"Testing loopback again...\r\n", 27, 1000);
    
    char test_msg[] = "TEST123";
    HAL_UART_Transmit(&huart1, (uint8_t *)test_msg, strlen(test_msg), 1000);
    
    char rx_buf[32] = {0};
    if (HAL_UART_Receive(&huart1, (uint8_t *)rx_buf, sizeof(rx_buf)-1, 2000) == HAL_OK) {
        if (strcmp(rx_buf, test_msg) == 0) {
            HAL_UART_Transmit(&huart6, (uint8_t *)"UART1 NOW WORKING! Ready for ESP32!\r\n", 38, 1000);
        } else {
            char debug_msg[100];
            sprintf(debug_msg, "Partial success: sent [%s] got [%s]\r\n", test_msg, rx_buf);
            HAL_UART_Transmit(&huart6, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        }
    } else {
        HAL_UART_Transmit(&huart6, (uint8_t *)"Still not working. Check CubeMX config.\r\n", 41, 1000);
    }
}

// UART6回环测试
void ESP32_TestUART6Loopback(void)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"=== UART6 Loopback Test ===\r\n", 30, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Please connect PC6 to PC7 with a wire\r\n", 39, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Testing in 3 seconds...\r\n", 25, 1000);
    HAL_Delay(3000);
    
    // 发送测试数据
    char test_msg[] = "UART6_TEST_OK";
    HAL_UART_Transmit(&huart6, (uint8_t *)test_msg, strlen(test_msg), 1000);
    
    // 尝试接收
    char rx_buf[32] = {0};
    if (HAL_UART_Receive(&huart6, (uint8_t *)rx_buf, sizeof(rx_buf)-1, 2000) == HAL_OK) {
        if (strcmp(rx_buf, test_msg) == 0) {
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"UART6 LOOPBACK PASSED - UART6 OK!\r\n", 36, 1000);
        } else {
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"UART6 PARTIAL - Data corrupted\r\n", 32, 1000);
        }
    } else {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"UART6 LOOPBACK FAILED\r\n", 23, 1000);
    }
}

// 测试不同波特率
void ESP32_TestDifferentBaudrates(void)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"=== Testing ESP32 with different baudrates ===\r\n", 49, 1000);
    
    uint32_t test_rates[] = {9600, 38400, 57600, 115200};
    uint32_t original_baud = huart6.Init.BaudRate;
    
    for (int i = 0; i < 4; i++) {
        char debug_msg[100];
        sprintf(debug_msg, "Testing %lu baud...\r\n", test_rates[i]);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
        
        // 重新配置UART6波特率
        huart6.Init.BaudRate = test_rates[i];
        if (HAL_UART_Init(&huart6) == HAL_OK) {
            
            // 发送AT命令
            HAL_UART_Transmit(&huart6, (uint8_t *)"AT\r\n", 4, 1000);
            HAL_Delay(500);
            
            // 尝试接收
            char rx_buf[64] = {0};
            if (HAL_UART_Receive(&huart6, (uint8_t *)rx_buf, 63, 2000) == HAL_OK) {
                if (strlen(rx_buf) > 0) {
                    sprintf(debug_msg, "SUCCESS at %lu baud: [%s]\r\n", test_rates[i], rx_buf);
                    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)debug_msg, strlen(debug_msg), 1000);
                    
                    // 保持这个波特率
                    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Found working baudrate!\r\n", 25, 1000);
                    return;
                }
            }
        }
    }
    
    // 恢复原始波特率
    huart6.Init.BaudRate = original_baud;
    HAL_UART_Init(&huart6);
    
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"No response at any baudrate!\r\n", 30, 1000);
}

// 初始化并连接WiFi
void ESP32_ConnectWiFi(void)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"=== ESP32 Weather Station ===\r\n", 32, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Using UART6 for ESP32 communication\r\n", 37, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Expected connections:\r\n", 22, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"  STM32 PC6 (UART6_TX) -> ESP32 GPIO7 (RX)\r\n", 44, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"  STM32 PC7 (UART6_RX) <- ESP32 GPIO6 (TX)\r\n", 44, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"  STM32 GND -> ESP32 GND\r\n", 26, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"  STM32 3.3V -> ESP32 3.3V\r\n", 28, 1000);
    
    // 1. 首先测试UART6回环
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\nStep 1: Testing UART6 functionality...\r\n", 42, 1000);
    ESP32_TestUART6Loopback();
    
    // 2. 测试不同波特率
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\nStep 2: Testing ESP32 with different baudrates...\r\n", 52, 1000);
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Make sure ESP32 is connected and powered on\r\n", 45, 1000);
    HAL_Delay(3000);
    
    ESP32_TestDifferentBaudrates();
    
    // 3. 尝试连接WiFi
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"\r\nStep 3: Attempting WiFi connection...\r\n", 40, 1000);
    
    if (ESP32_ConnectToWiFi()) {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"SUCCESS! WiFi connected!\r\n", 26, 1000);
        HAL_Delay(2000);
        ESP32_GetWeather();
    } else {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"Failed to connect. Troubleshooting:\r\n", 37, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"1. Check ESP32 power (LED should be on)\r\n", 41, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"2. Verify wire connections:\r\n", 29, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"   PC6 -> ESP32 GPIO7 (RX)\r\n", 28, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"   PC7 <- ESP32 GPIO6 (TX)\r\n", 28, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"3. ESP32 should run AT firmware\r\n", 33, 1000);
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)"4. Try USB-TTL direct connection to ESP32\r\n", 43, 1000);
    }
}

