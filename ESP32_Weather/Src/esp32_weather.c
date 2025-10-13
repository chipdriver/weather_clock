#include "esp32_weather.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

static char esp32_rx_buffer[RXBUFFER];
static uint16_t esp32_rx_index = 0;//1.为什么加static？ 2.这个是干嘛的？

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

    if(AT_SendFormatAndWait("OK",15000,"AT+CWJAP=\"%s\",\"%s\"","Niceday","178536586479") == 0)
        HAL_UART_Transmit(&huart6,(uint8_t *)"wifi连接成功\n", 18, 1000);
}