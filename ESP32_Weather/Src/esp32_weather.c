#include "esp32_weather.h"
#include "lcd_Driver.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include "GUI.h"  

static char esp32_rx_buffer[RXBUFFER];
static uint16_t esp32_rx_index = 0;
char weather_msg[256];


/*====================================================================第0层：通信基础=======================================================================*/
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
    uint32_t start_time;

    //参数检查
    if(cmd == NULL || expect == NULL || timeout_ms == 0)
    {
        return 2; // 参数错误
    }

    //清空接收缓冲区
    memset(esp32_rx_buffer,0,sizeof(esp32_rx_buffer));
    esp32_rx_index = 0;

    //发送AT指令
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 1000);

    //开始计时
    start_time = HAL_GetTick();

    //循环接收响应
    while(HAL_GetTick() - start_time <timeout_ms)
    {
        if(HAL_UART_Receive(&huart1,&esp32_rx_char,1,10) == HAL_OK)
        {
            //防止缓冲区溢出
            if(esp32_rx_index < sizeof(esp32_rx_buffer) - 1)
            {
                esp32_rx_buffer[esp32_rx_index++] = esp32_rx_char; 
                esp32_rx_buffer[esp32_rx_index] = '\0'; //保持字符串结束符

                if(strstr(esp32_rx_buffer,expect) != NULL)
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



/*====================================================================WiFi连接=======================================================================*/
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

/*====================================================================天气信息=======================================================================*/
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

        HAL_UART_Transmit(&huart6, (uint8_t*)weather_msg, strlen(weather_msg), 1000);
        char lcd_temp[20];
        sprintf(lcd_temp, "%s", temp_str);  // 英文显示，无换行符
        Gui_DrawAsciiString(100, 10, BLACK, WHITE, lcd_temp);

    }
    
    if(condition_pos) {
        sscanf(condition_pos, "\"text\":\"%63[^\"]\"", condition_str);
        sprintf(weather_msg, "天气状况: %s\n", condition_str);
        HAL_UART_Transmit(&huart6, (uint8_t*)weather_msg, strlen(weather_msg), 1000);
        char lcd_weather[20];
        sprintf(lcd_weather, "%s", condition_str);  // 英文显示，无换行符
        Gui_DrawAsciiString(10, 10, BLACK, WHITE, lcd_weather);
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"天气数据解析完成\n", 24, 1000);
}


static char time_msg[256];
static uint16_t time_rx_index = 0;
/**
 * @brief 获取日期时间
 * @param 无
 * @return 无
 */
void get_time(void)
{
    // 声明单字节接收缓冲变量，用于逐字节接收UART数据
    uint8_t time_rx_char;
    
    // 声明时间戳变量，用于记录开始接收数据的时刻（单位：毫秒）
    uint32_t start_time;
    
    // 清空时间消息缓冲区，将所有256个字节设置为0
    // 这样可以确保不会残留上次的数据
    memset(time_msg, 0, sizeof(time_msg));
    
    // 重置接收索引为0，准备从缓冲区开头开始存储新数据
    time_rx_index = 0;
    
    // 发送SNTP配置命令到ESP32
    // AT+CIPSNTPCFG=1,8,"pool.ntp.org","time.google.com"
    // 参数说明：1=启用SNTP, 8=时区(东八区北京时间), 后面是两个NTP服务器地址
    // 等待"OK"响应，超时时间5000ms
    AT_SendFormatAndWait("OK", 5000, "AT+CIPSNTPCFG=1,8,\"%s\",\"%s\"", 
                        "pool.ntp.org", "time.google.com");
    
    // 延时15秒，等待ESP32与NTP服务器同步时间
    // 这个延时很关键，SNTP协议需要时间进行网络通信和时间同步
    HAL_Delay(15000);
    
    // 定义查询时间的AT命令字符串（包含\r\n结束符）
    char time_AT[] = "AT+CIPSNTPTIME?\r\n";
    
    // 通过UART1向ESP32发送查询时间命令
    // 发送整个命令字符串，超时时间1000ms
    HAL_UART_Transmit(&huart1, (uint8_t*)time_AT, strlen(time_AT), 1000);
    
    // 记录当前系统时间戳，作为接收数据的起始时间点
    start_time = HAL_GetTick();
    
    // 进入接收循环，最多等待3000ms（3秒）
    // 只要当前时间与起始时间的差值小于3000ms，就继续尝试接收
    while(HAL_GetTick() - start_time < 3000)
    {
        // 尝试从UART1接收1个字节的数据，超时时间10ms
        // 如果成功接收到数据，HAL_UART_Receive返回HAL_OK
        if(HAL_UART_Receive(&huart1, &time_rx_char, 1, 10) == HAL_OK)
        {
            // 检查缓冲区是否还有空间（预留1个字节给'\0'结束符）
            // 防止数组越界，确保安全性
            if(time_rx_index < sizeof(time_msg) - 1)
            {
                // 将接收到的字节存入缓冲区当前位置
                time_msg[time_rx_index++] = time_rx_char;
                
                // 在新位置添加字符串结束符'\0'
                // 这样time_msg始终是一个有效的C字符串，可以随时使用字符串函数
                time_msg[time_rx_index] = '\0';
            }
        }
        // 如果HAL_UART_Receive超时（10ms内未收到数据），继续循环等待
        // 直到总超时时间3000ms到达
    }
    
    // 接收完成后，调用解析函数处理time_msg中的时间数据
    parse_time_data();
}

/**
 * @brief 解析时间数据并显示在LCD
 * @param 无
 * @return 无
 */
void parse_time_data(void)
{
    // 在time_msg缓冲区中查找"+CIPSNTPTIME:"字符串的位置
    // ESP32返回的完整格式：AT+CIPSNTPTIME?\r\n+CIPSNTPTIME:Fri Oct 17 11:20:01 2025\r\nOK\r\n
    // strstr函数返回找到的子串的起始指针，如果未找到则返回NULL
    char *time_start = strstr(time_msg, "+CIPSNTPTIME:");
    
    // 检查是否成功找到时间数据
    if(time_start == NULL) {
        return;  // 未找到时间数据，直接返回，不进行后续处理
    }
    
    // 指针向后移动13个字符，跳过 "+CIPSNTPTIME:" 这个前缀
    // 移动后指针指向时间数据的开始位置
    time_start += 13;
    
    // 循环跳过所有前导空格字符
    // *time_start 获取指针当前位置的字符
    // 如果是空格，指针向后移动一位，直到遇到非空格字符
    while(*time_start == ' ') time_start++;
    
    // 声明变量用于存储解析出的时间各部分
    // weekday：星期几的英文缩写（如Fri）
    // month：月份的英文缩写（如Oct）
    char weekday[16] = {0};  // 初始化为全0，确保字符串以'\0'结尾
    char month[16] = {0};
    
    // day：日期（1-31）
    // hour：小时（0-23）
    // minute：分钟（0-59）
    // second：秒（0-59）
    // year：年份（如2025）
    int day, hour, minute, second, year;
    
    // 使用sscanf从字符串中按格式解析时间数据
    // 格式："Fri Oct 17 11:20:01 2025"
    // %15s：读取最多15个字符的字符串（星期）
    // %15s：读取最多15个字符的字符串（月份）
    // %d：读取整数（日期）
    // %d:%d:%d：读取三个用冒号分隔的整数（时:分:秒）
    // %d：读取整数（年份）
    // 返回值parsed：成功解析的参数个数，应该为7
    int parsed = sscanf(time_start, "%15s %15s %d %d:%d:%d %d",
                       weekday, month, &day, &hour, &minute, &second, &year);
    
    // 检查是否成功解析了全部7个字段
    if(parsed == 7) {
        // 初始化月份数字为1（默认值）
        int month_num = 1;
        
        // 通过一系列if-else语句将英文月份缩写转换为数字1-12
        // strcmp函数比较两个字符串，相等返回0
        if(strcmp(month, "Jan") == 0) month_num = 1;       // January 一月
        else if(strcmp(month, "Feb") == 0) month_num = 2;  // February 二月
        else if(strcmp(month, "Mar") == 0) month_num = 3;  // March 三月
        else if(strcmp(month, "Apr") == 0) month_num = 4;  // April 四月
        else if(strcmp(month, "May") == 0) month_num = 5;  // May 五月
        else if(strcmp(month, "Jun") == 0) month_num = 6;  // June 六月
        else if(strcmp(month, "Jul") == 0) month_num = 7;  // July 七月
        else if(strcmp(month, "Aug") == 0) month_num = 8;  // August 八月
        else if(strcmp(month, "Sep") == 0) month_num = 9;  // September 九月
        else if(strcmp(month, "Oct") == 0) month_num = 10; // October 十月
        else if(strcmp(month, "Nov") == 0) month_num = 11; // November 十一月
        else if(strcmp(month, "Dec") == 0) month_num = 12; // December 十二月
        
        // 声明16字节的字符数组用于存储格式化后的日期字符串
        char date_display[16];
        
        // 使用sprintf格式化日期字符串为 "2025/10/17" 格式
        // %04d：4位数字，不足前面补0（年份）
        // %02d：2位数字，不足前面补0（月份和日期）
        sprintf(date_display, "%04d/%02d/%02d", year, month_num, day);
        
        // 声明16字节的字符数组用于存储格式化后的时间字符串
        char time_display[16];
        
        // 使用sprintf格式化时间字符串为 "11:20:01" 格式
        // %02d：2位数字，不足前面补0（时、分、秒）
        sprintf(time_display, "%02d:%02d:%02d", hour, minute, second);
        
        // 在LCD屏幕上显示日期
        // 参数：x=10像素, y=90像素, 前景色=黑色, 背景色=白色, 显示内容=日期字符串
        Gui_DrawAsciiString(10, 90, BLACK, WHITE, date_display);
        
        // 在LCD屏幕上显示时间
        // 参数：x=20像素, y=110像素, 前景色=黑色, 背景色=白色, 显示内容=时间字符串
        Gui_DrawAsciiString(20, 110, BLACK, WHITE, time_display);
    }
    // 如果parsed != 7，说明解析失败，函数直接结束，不显示任何内容
}