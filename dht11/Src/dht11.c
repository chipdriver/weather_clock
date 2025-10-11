#include "dht11.h"

// 初始化 DWT 用于微秒延时
void DWT_Delay_Init(void) {
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

// 微秒级延时
void DWT_Delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - startTick) < delayTicks);
}

// 设置引脚为输出模式
static void DHT_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);
}

// 设置引脚为输入模式
static void DHT_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT_GPIO_Port, &GPIO_InitStruct);
}

// 读取一个位
static bool DHT_ReadBit(void) {
    uint32_t t = 0;
    
    // 等待低电平结束
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_RESET) {
        if (++t > 200) return false;
        DWT_Delay_us(1);
    }
    
    t = 0;
    // 等待高电平结束
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 200) return false;
        DWT_Delay_us(1);
    }
    
    // 如果高电平持续时间大于30us，则为1，否则为0
    return t > 30;
}

// 初始化DHT11
bool DHT11_Init(void) {
    // 初始化DWT延时
    DWT_Delay_Init();
    
    // 设置为输出模式
    DHT_SetOutput();
    
    // 拉高电平，稳定2秒
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_SET);
    HAL_Delay(2000);
    
    return true;
}

// 读取DHT11数据
bool DHT11_Read(int *humidity, int *temperature) {
    uint8_t data[5] = {0};
    uint32_t t = 0;
    
    // 1. 主机发送开始信号
    DHT_SetOutput();
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_RESET);
    HAL_Delay(18);  // 保持低电平18ms
    
    HAL_GPIO_WritePin(DHT_GPIO_Port, DHT_Pin, GPIO_PIN_SET);
    DWT_Delay_us(30);  // 拉高30us
    
    // 2. 设置为输入模式，等待DHT11响应
    DHT_SetInput();
    
    // 等待DHT11响应信号（低电平）
    t = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 100) return false;  // 超时
        DWT_Delay_us(1);
    }
    
    // 等待响应信号结束（高电平）
    t = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_RESET) {
        if (++t > 100) return false;  // 超时
        DWT_Delay_us(1);
    }
    
    // 等待准备发送数据信号结束（低电平）
    t = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_Port, DHT_Pin) == GPIO_PIN_SET) {
        if (++t > 100) return false;  // 超时
        DWT_Delay_us(1);
    }
    
    // 3. 读取40位数据
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            if (DHT_ReadBit()) {
                data[i] |= (1 << j);
            }
        }
    }
    
    // 4. 校验数据
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return false;
    }
    
    // 5. 解析数据
    *humidity = data[0];
    *temperature = data[2];
    
    return true;
}
