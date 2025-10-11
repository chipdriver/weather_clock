/**
 * @file dht11.c
 * @brief DHT11温湿度传感器驱动实现文件
 * @author Weather Station Project
 * @date 2024
 * @version 1.0
 * 
 * 该文件实现了DHT11传感器的完整驱动程序，包括初始化、数据读取、
 * 时序控制等功能。DHT11使用单总线通信协议，需要精确的时序控制。
 */

#include "dht11.h"  // 包含DHT11驱动头文件

/*============================================================================ 底层硬件抽象层 ============================================================================*/

/* ==================== 私有函数声明 ==================== */
static void DHT_SetOutput(void);  // 设置GPIO为输出模式
static void DHT_SetInput(void);   // 设置GPIO为输入模式
static bool DHT_ReadBit(void);    // 读取单个数据位

/* ==================== DWT延时相关函数 ==================== */

/**
 * @brief 初始化DWT(Data Watchpoint and Trace)单元用于微秒级延时
 * @param 无
 * @return 无
 * @note DWT是ARM Cortex-M内核的调试单元，可提供高精度计时功能
 *       该函数检查DWT是否已启用，如未启用则进行初始化配置
 */
void DWT_Delay_Init(void) {
    // 检查跟踪使能位是否已设置
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        // 启用DWT和ITM单元的跟踪功能
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        // 复位循环计数器
        DWT->CYCCNT = 0;
        // 启用循环计数器
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

/**
 * @brief 微秒级精确延时函数
 * @param us 延时时间，单位为微秒(μs)
 * @return 无
 * @note 基于DWT循环计数器实现，精度高于HAL_Delay
 *       延时精度取决于系统时钟频率，通常为纳秒级
 */
void DWT_Delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;                    // 记录起始时刻的计数值
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);  // 计算需要的时钟周期数
    // 等待直到经过足够的时钟周期
    while ((DWT->CYCCNT - startTick) < delayTicks);
}

/* ==================== GPIO配置相关函数 ==================== */

/**
 * @brief 设置DHT11数据引脚为输出模式
 * @param 无
 * @return 无
 * @note 在发送起始信号和拉高总线时使用
 *       配置为推挽输出，无上下拉，低速模式
 */
static void DHT_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};              // GPIO初始化结构体，清零
    GPIO_InitStruct.Pin = DHT_Pin;                       // 设置要配置的引脚
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;          // 推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;                  // 无上下拉电阻
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速模式，满足DHT11时序要求
    HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);      // 应用GPIO配置
}

/**
 * @brief 设置DHT11数据引脚为输入模式
 * @param 无
 * @return 无  
 * @note 在读取DHT11响应和数据时使用
 *       配置为输入模式，内部上拉，低速模式
 */
static void DHT_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};              // GPIO初始化结构体，清零
    GPIO_InitStruct.Pin = DHT_Pin;                       // 设置要配置的引脚
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;              // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;                  // 内部上拉电阻
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速模式
    HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);      // 应用GPIO配置
}

/* ==================== DHT11通信协议相关函数 ==================== */

/**
 * @brief 读取DHT11发送的单个数据位
 * @param 无
 * @return bool 读取到的位值
 * @retval true  数据位为1
 * @retval false 数据位为0或读取超时
 * @note DHT11数据位编码：
 *       - 数据位0: 低电平50μs + 高电平26-28μs
 *       - 数据位1: 低电平50μs + 高电平70μs
 *       通过检测高电平持续时间判断位值
 */
static bool DHT_ReadBit(void) {
    uint32_t t = 0;                                      // 超时计数器
    
    // 等待每位数据开始前的低电平信号结束
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_RESET) {
        if (++t > 200) return false;                     // 超时保护，防止死循环
        DWT_Delay_us(1);                                 // 1μs精确延时
    }
    
    t = 0;                                               // 重置计数器，开始计时高电平持续时间
    // 等待高电平信号结束，同时计算持续时间
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 200) return false;                     // 超时保护
        DWT_Delay_us(1);                                 // 1μs精确延时
    }
    
    // 根据高电平持续时间判断数据位
    // 如果高电平持续时间大于30μs，则为数据位1，否则为数据位0
    return t > 30;
}
/*============================================================================ 通信协议层 ============================================================================*/
/**
 * @brief 初始化DHT11温湿度传感器
 * @param 无
 * @return bool 初始化结果
 * @retval true  初始化成功
 * @retval false 初始化失败（当前版本总是返回true）
 * @note 初始化步骤：
 *       1. 初始化DWT延时功能
 *       2. 配置GPIO为输出模式
 *       3. 拉高数据线并保持2秒，让传感器稳定
 */
bool DHT11_Init(void) {
    // 初始化DWT延时功能，为后续微秒级延时做准备
    DWT_Delay_Init();
    
    // 设置数据引脚为输出模式
    DHT_SetOutput();
    
    // 拉高数据线并保持2秒，让DHT11传感器进入稳定工作状态
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_SET);
    HAL_Delay(2000);                                     // 等待2秒让传感器稳定
    
    return true;                                         // 返回初始化成功
}

/**
 * @brief 读取DHT11温湿度数据
 * @param humidity    指向湿度变量的指针，用于返回湿度值(%)
 * @param temperature 指向温度变量的指针，用于返回温度值(°C)
 * @return bool 读取结果
 * @retval true  读取成功，数据有效
 * @retval false 读取失败，数据无效
 * @note DHT11通信时序：
 *       1. 主机发送起始信号：低电平18ms + 高电平30μs
 *       2. DHT11响应信号：低电平80μs + 高电平80μs
 *       3. DHT11发送40位数据：5字节（湿度整数+湿度小数+温度整数+温度小数+校验和）
 *       4. 数据校验：前4字节之和应等于第5字节
 */
bool DHT11_Read(int *humidity, int *temperature) {
    uint8_t data[5] = {0};                               // 存储接收的5字节数据
    uint32_t t = 0;                                      // 超时计数器
    
    // ========== 第1步：主机发送开始信号 ==========
    DHT_SetOutput();                                     // 设置GPIO为输出模式
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_RESET);  // 拉低数据线
    HAL_Delay(18);                                       // 保持低电平18ms，通知DHT11开始测量
    
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_SET);    // 拉高数据线
    DWT_Delay_us(30);                                    // 保持高电平30μs，完成起始信号
    
    // ========== 第2步：设置为输入模式，等待DHT11响应 ==========
    DHT_SetInput();                                      // 设置GPIO为输入模式，释放总线控制权
    
    // 等待DHT11拉低数据线开始响应信号（低电平80μs）
    t = 0;                                               // 重置超时计数器
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 100) return false;                     // 超时保护，防止DHT11无响应
        DWT_Delay_us(1);                                 // 1μs延时
    }
    
    // 等待DHT11响应信号的低电平部分结束
    t = 0;                                               // 重置计数器
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_RESET) {
        if (++t > 100) return false;                     // 超时保护
        DWT_Delay_us(1);                                 // 1μs延时
    }
    
    // 等待DHT11响应信号的高电平部分结束（高电平80μs）
    t = 0;                                               // 重置计数器
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 100) return false;                     // 超时保护
        DWT_Delay_us(1);                                 // 1μs延时
    }
    
    // ========== 第3步：读取40位数据 ==========
    // DHT11发送5字节数据，每字节8位，共40位
    for (int i = 0; i < 5; i++) {                        // 遍历5个字节
        for (int j = 7; j >= 0; j--) {                   // 遍历每字节的8位，从高位开始
            if (DHT_ReadBit()) {                         // 读取一个数据位
                data[i] |= (1 << j);                     // 如果是'1'，设置对应位
            }
            // 如果是'0'，该位保持默认值0
        }
    }
    
    // ========== 第4步：数据校验 ==========
    // 校验和 = 湿度高字节 + 湿度低字节 + 温度高字节 + 温度低字节
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {                           // 校验和不匹配
        return false;                                    // 数据错误，返回失败
    }
    
    // ========== 第5步：解析并返回数据 ==========
    // DHT11数据格式：
    // data[0] = 湿度整数部分
    // data[1] = 湿度小数部分（DHT11中为0）
    // data[2] = 温度整数部分  
    // data[3] = 温度小数部分（DHT11中为0）
    // data[4] = 校验和
    *humidity = data[0];                                 // 返回湿度值（%RH）
    *temperature = data[2];                              // 返回温度值（℃）
    
    return true;                                         // 读取成功
}
