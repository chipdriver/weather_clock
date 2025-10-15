/* USER CODE BEGIN Header */
/**
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_Driver.h"
#include "GUI.h" 
#include "font.h"
#include "esp32_weather.h"
#include "dht11.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Transmit(&huart6, (uint8_t *)"Hello from STM32!\n", 19, 1000);

  // 初始化LCD  
  Lcd_Init();
  DHT11_Init();

  int humidity, temperature;
  char buffer[100] = {0};

  // 清屏为黑色
  Lcd_Clear(WHITE);

  // 显示湿度图标
  Gui_DrawImage(1, 50, gImage_humo_nei);  // 在(5,50)位置显示湿度图标
  Gui_DrawImage(50, 50, gImage_temp_nei);  // 在(65,50)位置显示温度图标
  Gui_DrawImage(90, 40, gImage_1);  // 在(100,50)位置显示外部温度图标
  Gui_DrawImage(80, 10, gImage_temp_wai);  // 在(100,50)位置显示外部温度图标


  uint32_t weather_counter = 0;
  wifi_connect();
  get_weather();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /*====================测试================ */
    // Gui_Circle(20,20,1,RED);
    //Gui_DrawLine(10,10,50,50,BLUE);
    //Gui_box(20,20,30,30,GREEN);
    //Gui_box(20,20,30,30,GREEN);
    //DisplayButtonDown(20,20,30,30);
    //Gui_Draw8x16Char(30, 30, BLACK, WHITE, 'A');
    //Gui_DrawAsciiString(10, 10, BLACK, WHITE, "Hello!"); // 显示英文字符串
    //Gui_DrawFont_GBK16(10, 10, BLACK, WHITE, (uint8_t*)"Hello!涵涵"); // 显示中英文混合字符串
    Gui_DrawFont_Num32(10, 100, BLACK, WHITE, 1);
    /*====================测试================ */
    /*读取DHT11温湿度数据*/
    DHT11_Read(&humidity, &temperature);
    // 读取成功，显示数据-
    sprintf(buffer, "%d%%",humidity); 
    Gui_DrawAsciiString(25,55,BLACK, WHITE, buffer);

    /*显示温度*/
    sprintf(buffer, "%d",temperature);
    if(temperature>30)
    {
      Gui_DrawAsciiString(68,55,RED, WHITE, buffer);
      Gui_Circle(82, 52,1, RED);
      Gui_DrawAsciiChar(82, 55, RED, WHITE, 'c');  // 显示摄氏度符号
    }
    else if(temperature<10)
    {
      Gui_DrawAsciiString(68,55,BLUE, WHITE, buffer);
      Gui_Circle(82, 52,1, BLACK);
      Gui_DrawAsciiChar(82, 55, BLACK, WHITE, 'c');  // 显示摄氏度符号
    }
    else
    {
      Gui_DrawAsciiString(68,55,BLACK, WHITE, buffer);
      Gui_Circle(82, 52, 1, BLACK);
     Gui_DrawAsciiChar(82, 55, BLACK, WHITE, 'C');  // 显示摄氏度符号
    }

    get_weather();  // 重新获取天气
    // 等待2秒再读取
    HAL_Delay(2000);
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
