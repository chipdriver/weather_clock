#include "esp32_weather.h"
#include "lcd_Driver.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include "GUI.h"  
static char esp32_rx_buffer[RXBUFFER];
static uint16_t esp32_rx_index = 0;//1.为什么加static？ 2.这个是干嘛的？
char weather_msg[256];
/**
 * @brief 发送AT指令并等待指定响应
 * @param cmd AT指令（不需要加\r\n)
 * @param expect 期望的响应字符串
 * @param timeout_ms: 超时时间（毫秒）
 * @return 0 成功找到期望响应
 * @return 1 超时或未找到期望响应
 * @return 2 参数错误
 */
uint8_t AT_SendAndWait(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    uint8_t esp32_rx_char;
    uint32_t start_time;//这个为什么是32位的

    //参数检查
    if(cmd == NULL || expect == NULL || timeout_ms == 0)
    {
        return 2; // 参数错误
    }

    //清空接收缓冲区
    memset(esp32_rx_buffer,0,sizeof(esp32_rx_buffer));
    esp32_rx_index = 0;

    //发送AT指令
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);//3.strlen什么作用
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 1000);

    //开始计时
    start_time = HAL_GetTick();//4.这个函数是干嘛的

    //循环接收响应
    while(HAL_GetTick() - start_time <timeout_ms)
    {
        if(HAL_UART_Receive(&huart1,&esp32_rx_char,1,10) == HAL_OK)
        {
            //防止缓冲区溢出
            if(esp32_rx_index < sizeof(esp32_rx_buffer) - 1)
            {
                esp32_rx_buffer[esp32_rx_index++] = esp32_rx_char; //5.esp32_rx_buffer[esp32_rx_index++]  与 esp32_rx_buffer[++esp32_rx_index]是什么关系？
                esp32_rx_buffer[esp32_rx_index] = '\0'; //保持字符串结束符

                if(strstr(esp32_rx_buffer,expect) != NULL)//6.这个函数是干嘛的
                {
                    return 0; // 找到期望响应
                }
            }
        }
    }
    return 1; // 超时未找到期望响应
}

/**
 * @brief 发送格式化AT指令并等待指定响应
 * @param expect 期望的响应字符串
 * @param timeout_ms 超时时间（毫秒）
 * @param format 格式化字符串（类似printf）
 * @param ... 可变参数列表
 * @return 0 成功找到期望响应
 * @return 1 超时或未找到期望响应
 * @return 2 参数错误
 */
uint8_t AT_SendFormatAndWait(const char *expect, uint32_t timeout_ms, const char *format, ...)
{
    va_list args;                    // 声明可变参数列表
    char cmd_buffer[256];            // 格式化后的指令缓冲区
    
    // 参数检查
    if (format == NULL || expect == NULL || timeout_ms == 0) {
        return 2;
    }
    
    // 开始处理可变参数
    va_start(args, format);//告诉编译器“从 format 之后的那个参数开始取”。   -------开始
    
    // 将格式化字符串和可变参数组合成完整指令
    vsnprintf(cmd_buffer, sizeof(cmd_buffer), format, args);//像 sprintf，但多一个 size，防止溢出。
    
    // 结束可变参数处理                         -------结束
    va_end(args);
    
    // 调用基础AT指令函数
    return AT_SendAndWait(cmd_buffer, expect, timeout_ms);
}


/**
 * @brief 连接wifi
 * @param None
 * @return None
 */
void wifi_connect(void)
{   
    //1.探活
    if(AT_SendAndWait("AT","OK",1000) == 0)
        HAL_UART_Transmit(&huart6,(uint8_t *)"ESP32响应正常\n" , 19, 1000);

    if(AT_SendFormatAndWait("OK",15000,"AT+CWJAP=\"%s\",\"%s\"","Niceday","17853658647") == 0)
        HAL_UART_Transmit(&huart6,(uint8_t *)"wifi连接成功\n", 18, 1000);
}

/**
 * @brief 获取天气以及温湿度
 * @param 
 * @return
 */
void get_weather(void)
{
    char http_request[512];

    int len = snprintf(http_request, sizeof(http_request),
        "GET /v3/weather/now.json?key=%s&location=%s&language=en&unit=c HTTP/1.1\r\n"
        "Host: api.seniverse.com\r\n"
        "Connection: close\r\n"
        "\r\n",
        "SS_d-8jLMrtu_Qb0m", "qingdao");  

    //多连接模式启动
    if(AT_SendAndWait("AT+CIPMUX=1","OK",1000) == 0)
        HAL_UART_Transmit(&huart6,(uint8_t *)"多连接模式启动成功\n", 17, 1000);

    //连接到天气服务器
    if(AT_SendAndWait("AT+CIPSTART=0,\"TCP\",\"api.seniverse.com\",80","OK",10000) == 0)
        HAL_UART_Transmit(&huart6,(uint8_t *)"连接到天气服务器成功\n", 22, 1000);

     if(AT_SendFormatAndWait(">",5000,"AT+CIPSEND=0,%d",len) == 0) {
        HAL_UART_Transmit(&huart6, (uint8_t*)"准备发送数据\n", 18, 1000);
        
        // ✅ 立即启动接收（在发送前准备好）
        memset(esp32_rx_buffer, 0, sizeof(esp32_rx_buffer));
        esp32_rx_index = 0;
        
        // 发送HTTP GET请求
        HAL_UART_Transmit(&huart1, (uint8_t*)http_request, len, 1000);

        // ✅ 立即开始接收，不要延时！
        receive_weather_data_immediate();
        parse_weather_json();
    }
}

/**
 * @brief 立即接收天气数据（改进版）
 */
void receive_weather_data_immediate(void)
{
    uint32_t start_time = HAL_GetTick();
    uint32_t last_rx_time = start_time;
    const uint32_t total_timeout = 15000;  // 总超时15秒
    const uint32_t silence_timeout = 500;  // ✅ 减少静默超时到500ms

    HAL_UART_Transmit(&huart6, (uint8_t*)"立即开始接收天气数据...\n", 30, 1000);

    int receive_count = 0;
    uint8_t first_data_received = 0;  // 标记是否收到第一个数据

    while((HAL_GetTick() - start_time) < total_timeout) {
        uint8_t ch;
        // ✅ 减少单次超时到20ms，提高响应速度
        if(HAL_UART_Receive(&huart1, &ch, 1, 20) == HAL_OK) {
            
            if(!first_data_received) {
                first_data_received = 1;
                HAL_UART_Transmit(&huart6, (uint8_t*)"收到第一个数据包!\n", 22, 1000);
            }
            
            last_rx_time = HAL_GetTick();
            receive_count++;
            
            if(esp32_rx_index < sizeof(esp32_rx_buffer) - 1) {
                esp32_rx_buffer[esp32_rx_index++] = ch;
                esp32_rx_buffer[esp32_rx_index] = '\0';
            }
            
            // 每接收50字节输出一次进度
            if(receive_count % 50 == 0) {
                char progress[50];
                sprintf(progress, "已接收: %d字节\n", esp32_rx_index);
                HAL_UART_Transmit(&huart6, (uint8_t*)progress, strlen(progress), 1000);
            }
            
            // ✅ 检查是否收到JSON结束标志
            if(esp32_rx_index > 10 && strstr(esp32_rx_buffer, "}]}") != NULL) {
                HAL_UART_Transmit(&huart6, (uint8_t*)"检测到JSON结束标志\n", 25, 1000);
                break;
            }
        }
        
        // ✅ 只有在收到过数据后才检查静默超时
        if(first_data_received && (HAL_GetTick() - last_rx_time) > silence_timeout) {
            HAL_UART_Transmit(&huart6, (uint8_t*)"静默超时，结束接收\n", 23, 1000);
            break;
        }
    }

    char debug_msg[100];
    sprintf(debug_msg, "接收完成: %d字节, 接收次数: %d\n", esp32_rx_index, receive_count);
    HAL_UART_Transmit(&huart6, (uint8_t*)debug_msg, strlen(debug_msg), 1000);
    
    // 输出前100字节看看收到了什么
    int output_len = (esp32_rx_index > 100) ? 100 : esp32_rx_index;
    HAL_UART_Transmit(&huart6, (uint8_t*)"前100字节:\n", 12, 1000);
    HAL_UART_Transmit(&huart6, (uint8_t*)esp32_rx_buffer, output_len, 1000);
}

/**
 * @brief 解析JSON天气数据
 * @param None  
 * @return None
 */
void parse_weather_json(void)
{
    char *json_start = strstr(esp32_rx_buffer, "\r\n\r\n");
    if (json_start) {
        json_start += 4;
        json_start = strchr(json_start, '{');
    }
    if (json_start == NULL) {
        // ✅ 修复：发送到调试串口
        HAL_UART_Transmit(&huart6, (uint8_t*)"未找到JSON数据\n", 20, 1000);
        return;
    }

    char temp_str[32] = {0};
    char condition_str[64] = {0};
    
    char *temp_pos = strstr(json_start, "\"temperature\":");
    char *condition_pos = strstr(json_start, "\"text\":");

    if(temp_pos) {
        sscanf(temp_pos, "\"temperature\":\"%31[^\"]\"", temp_str);
        sprintf(weather_msg, "温度: %s°C\n", temp_str);
        // ✅ 修复：发送到调试串口
        HAL_UART_Transmit(&huart6, (uint8_t*)weather_msg, strlen(weather_msg), 1000);
        char lcd_temp[20];
        sprintf(lcd_temp, "%s", temp_str);  // 英文显示，无换行符
        Gui_DrawString(100, 10, BLACK, WHITE, lcd_temp);

    }
    
    if(condition_pos) {
        sscanf(condition_pos, "\"text\":\"%63[^\"]\"", condition_str);
        sprintf(weather_msg, "天气状况: %s\n", condition_str);
        HAL_UART_Transmit(&huart6, (uint8_t*)weather_msg, strlen(weather_msg), 1000);
        char lcd_weather[20];
        sprintf(lcd_weather, "%s", condition_str);  // 英文显示，无换行符
    }

    // ✅ 修复：发送到调试串口
    HAL_UART_Transmit(&huart6, (uint8_t*)"天气数据解析完成\n", 24, 1000);
}



