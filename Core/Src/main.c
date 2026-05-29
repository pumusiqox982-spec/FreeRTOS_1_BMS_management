/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "freertos_demo.h"
#include "semphr.h"
#include "Get_and_Convert.h"
#include "APP_UPDATA.h"
#include "Safety_Protection.h"
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
CAN_HandleTypeDef hcan;

IWDG_HandleTypeDef hiwdg;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
SemaphoreHandle_t uart_mutex;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_CAN_Init(void);
/* USER CODE BEGIN PFP */
int uart_printf(const char* format, ...);
void CAN_Init(void);
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
  MX_IWDG_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  /*********************在进入freertos_start前，直接初始化所有数据**********************/

    uart_mutex = xSemaphoreCreateMutex();  // 创建互斥锁
    CAN_Init();                       // 初始化 CAN过滤器
    BQ76920_Init_SOC();  
   // uart_printf("=== System START ===\n");
  
  /* 启动FreeRTOS */
  freertos_start();
  /*进入freertos_start后，下面的代码不会被执行*/

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 6;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_8TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
  hiwdg.Init.Reload = 2000;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  通用UART发送函数（类似printf）
  * @param  format: 格式化字符串
  * @param  ...: 可变参数
  * @retval 发送的字节数
  */
int uart_printf(const char* format, ...)
{
 if (xSemaphoreTake(uart_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        int len = vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, len, HAL_MAX_DELAY);
        xSemaphoreGive(uart_mutex);
        return len;
    }
    return 0;
}




/*
 * @brief 发送BMS数据到CAN总线
*/ 
 void BMS_CAN_SendData(void)
{
    CAN_TxHeaderTypeDef txHeader;   // 定义元数据结构
    uint8_t txData[8];             // 定义发送数据数组
    uint32_t txMailbox;            // 定义发送邮箱变量

    // ----- 帧0x181: 电流、温度、SOC -----
    // ----- 帧0x181: 电流、温度、SOC -----
    txHeader.StdId = 0x181;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;

    int16_t current_int = (int16_t)BQ76920_Data.CC;
    int16_t temp_int = (int16_t)BQ76920_Data.Temp;
    uint16_t soc_int = (uint16_t)BQ76920_Data.SOC;

    txData[0] = (current_int >> 8) & 0xFF;
    txData[1] = current_int & 0xFF;
    txData[2] = (temp_int >> 8) & 0xFF;
    txData[3] = temp_int & 0xFF;
    txData[4] = (soc_int >> 8) & 0xFF;
    txData[5] = soc_int & 0xFF;

    // 读取 MOS 状态寄存器 SYS_CTRL2 (地址 0x05)
    uint8_t sys_ctrl2 = 0;
    BQ76920_Read_Reg(0x05, &sys_ctrl2);
    txData[6] = sys_ctrl2;   // bit0=CHG_ON, bit1=DSG_ON

    // 读取均衡状态 (0x01) 放入第7字节
    uint8_t cell_bal = 0;
    BQ76920_Read_Reg(0x01, &cell_bal);
    txData[7] = cell_bal;   // 低5位表示均衡掩码

    HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox);

    // ----- 帧0x182: 前四节电压 -----
    txHeader.StdId = 0x182;
    txData[0] = (BQ76920_Data.Cell_V[0] >> 8) & 0xFF;
    txData[1] = BQ76920_Data.Cell_V[0] & 0xFF;
    txData[2] = (BQ76920_Data.Cell_V[1] >> 8) & 0xFF;
    txData[3] = BQ76920_Data.Cell_V[1] & 0xFF;
    txData[4] = (BQ76920_Data.Cell_V[2] >> 8) & 0xFF;
    txData[5] = BQ76920_Data.Cell_V[2] & 0xFF;
    txData[6] = (BQ76920_Data.Cell_V[3] >> 8) & 0xFF;
    txData[7] = BQ76920_Data.Cell_V[3] & 0xFF;
    HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox);

    // ----- 帧0x183: 第五节 + 最小/最大/压差 -----
    txHeader.StdId = 0x183;
    txData[0] = (BQ76920_Data.Cell_V[4] >> 8) & 0xFF;
    txData[1] = BQ76920_Data.Cell_V[4] & 0xFF;
    txData[2] = ((uint16_t)BQ76920_Data.MinVolt >> 8) & 0xFF;
    txData[3] = (uint16_t)BQ76920_Data.MinVolt & 0xFF;
    txData[4] = ((uint16_t)BQ76920_Data.MaxVolt >> 8) & 0xFF;
    txData[5] = (uint16_t)BQ76920_Data.MaxVolt & 0xFF;
    txData[6] = ((uint16_t)BQ76920_Data.DiffVolt >> 8) & 0xFF;
    txData[7] = (uint16_t)BQ76920_Data.DiffVolt & 0xFF;
    HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox);
} 


/**
 * @brief  CAN 命令接收与处理（非阻塞，轮询方式）
 * @note   在 FreeRTOS 任务中周期调用（如每 10ms 调用一次）
 */
void CAN_ProcessCommands(void)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];
    if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
        return;
    if (rxHeader.StdId != 0x201 || rxHeader.DLC < 2)
        return;

    // 收到任何命令，都激活手动模式并重置计时器
    Manual_Mode_Active = 1;
    Manual_Mode_Timer = 0;

    switch (rxData[0])
    {
        case 0x01: // 充电MOS
            if (rxData[1]) Bms_Turn_On_Charge_MOS();
            else Bms_Turn_Off_Charge_MOS();
            break;
        case 0x02: // 放电MOS
            if (rxData[1]) Bms_Turn_On_Discharge_MOS();
            else Bms_Turn_Off_Discharge_MOS();
            break;
        case 0x03: // 均衡
            BQ76920_Write_Reg(0x01, rxData[1]);  // 直接写均衡寄存器
            break;
        case 0x04: // 复位锁定
            Bms_Reset_Safety_Lock();
            break;
        default: break;
    }
}





 

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
