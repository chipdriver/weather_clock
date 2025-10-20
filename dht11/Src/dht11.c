/**
 * @file dht11.c
 * @brief DHT11温湿度传感器驱动实现文件
 * @author Chipdriver
 * @date 2024
 * @version 1.0
 * 
 * 该文件实现了DHT11传感器的完整驱动程序，包括初始化、数据读取、
 * 时序控制等功能。DHT11使用单总线通信协议，需要精确的时序控制。
 * 
 * ============================================================================
 * 【代码分层架构说明】
 * ============================================================================
 * 
 * 本驱动采用分层设计，符合软件工程最佳实践：
 * 
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                    第2层：通信协议层（对外API）                          │
 * │  ┌─────────────────────────────────────────────────────────────────┐   │
 * │  │ DHT11_Init()    - 初始化DHT11传感器                             │   │
 * │  │ DHT11_Read()    - 读取温湿度数据                                │   │
 * │  └─────────────────────────────────────────────────────────────────┘   │
 * │  特点：实现DHT11单总线通信协议、数据解析、校验等业务逻辑               │
 * └─────────────────────────────────────────────────────────────────────────┘
 *                                    ↓ 调用
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │               第1层：底层硬件抽象层（内部实现，static）                  │
 * │  ┌─────────────────────────────────────────────────────────────────┐   │
 * │  │ DWT_Delay_Init()  - 初始化DWT高精度计时器                       │   │
 * │  │ DWT_Delay_us()    - 微秒级精确延时                              │   │
 * │  │ DHT_SetOutput()   - 设置GPIO为输出模式                          │   │
 * │  │ DHT_SetInput()    - 设置GPIO为输入模式                          │   │
 * │  │ DHT_ReadBit()     - 读取单个数据位（含时序判断）                │   │
 * │  └─────────────────────────────────────────────────────────────────┘   │
 * │  特点：直接操作硬件寄存器、高复用性、硬件相关                           │
 * └─────────────────────────────────────────────────────────────────────────┘
 *                                    ↓ 调用
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │                      STM32 HAL库 + 硬件寄存器                           │
 * │  HAL_GPIO_Init(), HAL_GPIO_ReadPin(), DWT->CYCCNT 等                   │
 * └─────────────────────────────────────────────────────────────────────────┘
 * 
 * 【分层优势】
 * ✅ 高内聚低耦合：每层职责明确，层间通过函数调用隔离
 * ✅ 易于移植：换芯片只需修改底层，换传感器只需修改协议层
 * ✅ 代码复用：底层函数可用于其他需要精确延时或GPIO控制的项目
 * ✅ 易于维护：修改底层不影响上层，符合开闭原则
 * 
 * 【函数可见性】
 * - 公开API（用户调用）：DHT11_Init(), DHT11_Read()
 * - 内部实现（static）：DHT_SetOutput(), DHT_SetInput(), DHT_ReadBit()
 * - 通用工具（public）：DWT_Delay_Init(), DWT_Delay_us()（可供其他模块使用）
 * ============================================================================
 */

#include "dht11.h"  // 包含DHT11驱动头文件
#include "stm32f4xx_hal_gpio.h"

/*============================================================================
 *                          底层硬件抽象层
 * ============================================================================
 * 本层直接操作硬件，提供基础功能：
 * 1. 精确延时（DWT）
 * 2. GPIO方向切换（输入/输出）
 * 3. 单位时序控制（读取单个位）
 * 
 * 特点：
 * - 硬件相关（依赖STM32F4的DWT和GPIO）
 * - 高复用性（可用于其他单总线协议，如DS18B20）
 * - 大部分使用static修饰，仅供内部使用
 * ============================================================================
 */

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
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) { //CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk的意思是检查是否开启了DWT，0：关闭，1：开启
        // 启用DWT和ITM单元的跟踪功能
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;    //打开 DWT（数据追踪单元）等调试功能的“总开关”
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
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);  // 计算需要的时钟周期数，为什么/1000000，因为1s=1000000us，单位转换
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
 * 
 * 【可调参数说明】适配不同批次DHT11
 * - 超时阈值 200：可改为100-300（根据传感器响应速度）
 * - 位判断阈值 30：标准值，适用于大多数DHT11
 *   如遇到误判：质量差的传感器可能需要调整为 35-40
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
    // 【可调】标准阈值30μs，适用于大多数DHT11
    // 数据位0约27μs，数据位1约70μs，选择中间值30μs作为判断阈值
    return t > 30;
}

/*============================================================================
 *                          通信协议层
 * ============================================================================
 * 本层基于底层硬件抽象层，实现DHT11特定的通信协议：
 * 1. 按照DHT11数据手册的时序要求发送起始信号
 * 2. 接收DHT11的响应信号
 * 3. 读取40位数据（5字节）
 * 4. 进行数据校验和解析
 * 
 * 特点：
 * - 协议相关（实现DHT11单总线通信时序）
 * - 对外接口（公开API，供main.c等用户代码调用）
 * - 包含业务逻辑（数据校验、解析、错误处理）
 * 
 * 依赖关系：
 * DHT11_Init() → DWT_Delay_Init(), DHT_SetOutput()
 * DHT11_Read() → DHT_SetOutput(), DHT_SetInput(), DHT_ReadBit(), DWT_Delay_us()
 * ============================================================================
 */

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
 * 
 * 【可调参数说明】适配不同批次DHT11
 * - 稳定时间 2000ms：标准值，某些传感器可能需要1000-3000ms
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
 * 
 * 【可调参数说明】适配不同批次DHT11
 * - 起始低电平 18ms：数据手册要求至少18ms，可调整为18-20ms
 * - 起始高电平 30μs：标准值20-40μs，当前30μs适用于大多数DHT11
 * - 超时阈值 100：响应慢的传感器可能需要150-200
 * 
 * 【兼容性说明】
 * ✅ 适用于大多数DHT11传感器（包括山寨版）
 * ✅ 自动校验数据，过滤错误读数
 * ✅ 超时保护，避免死循环
 */

 /**
  * @brief 读取DHT11的温湿度
  * @param humidity 指向湿度变量的指针，用于返回湿度值（%）
  * @param temperature 指向温度变量的指针，用于返回温度值（°C）
  * @return bool 读取结果
  * @retval true 读取成功，数据有效
  * @retval false 读取失败，数据无效
  * @note 此函数根据DHT11的通信时序编写
  */
 bool DHT11_Read(int *humidity, int *temperature)
 {
    uint8_t data[5] = {0}; //存储5字节数据

    //主机发送开始信号-告诉dht11我要读取数据了
    DHT11_SendStartSignal();

    //DHT11响应信号-告诉主机我准备好了
    if(DHT11_WaitForResponse() == false)
    {
        return false; //响应失败
    }

    //读取40位数据
    

    //解析40位数据
 }

 /**
  * @brief 主机发送开始信号
  * @param 无
  * @return 无
  * @note 主机通过拉低数据线至少18ms，然后拉高30μs，通知DHT11准备发送数据
  */
 void DHT11_SendStartSignal(void)
 {
    HAL_GPIO_WritePin(DHT_GPIO_Port,DHT_Pin,GPIO_PIN_RESET); //拉低总线
    DWT_Delay_us(18000); //保持18ms
    HAL_GPIO_WritePin(DHT_GPIO_Port,DHT_Pin,GPIO_PIN_SET); //拉高总线
    DWT_Delay_us(30); //保持30us
 }

 /**
  * @brief DHT11响应信号
  * @param 无
  * @return bool 响应结果
  * @retval true 响应成功
  * @note DHT11在接收到主机的开始信号后，会拉低总线80μs，然后拉高80μs，表示准备好发送数据 
  */
bool DHT11_WaitForResponse(void)
{
    uint32_t t = 0;//超时计数器

    //等待dht11的低电平
    while(HAL_GPIO_ReadPin(DHT_GPIO_Port,DHT_Pin) == GPIO_PIN_SET)
    {
        if(++t > 100) return false; //超时保护
        DWT_Delay_us(1); //1us精确延时
    }

    //dht11的低电平响应结束->开始高电平响应
    t = 0;
    while(HAL_GPIO_ReadPin(DHT_GPIO_Port,DHT_Pin) == GPIO_PIN_RESET)
    {
        if(++t > 100) return false; //超时保护
        DWT_Delay_us(1); //1us精确延时
    }

    //dht11的高电平响应结束
    t = 0;
    while(HAL_GPIO_ReadPin(DHT_GPIO_Port,DHT_Pin) == GPIO_PIN_SET)
    {
        if(++t > 100) return false; //超时保护
        DWT_Delay_us(1); //1us精确延时
    }

    return true;
}

/**
 * @brief 读取DHT11发送的5字节数据，每个字节8位，共40位
 * @param data 指向5字节数据数组的指针
 * @return 无
 */
void DHT11_ReadData(uint8_t *data)
{
    for(int i = 0; i < 5 ; i++) //遍历5个字节
    {
        for(int j = 7; j >= 0; j--)   //遍历每个字节的8位，从高位开始
        {
            if((DHT_ReadBit))
            {
                data[i] |= (1 <<j); //读取到1，设置对应位
            }
            //如果是‘0’，不需要操作，默认就是0
        }
    }
}