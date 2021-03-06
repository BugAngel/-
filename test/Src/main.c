/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "BlueTooth.h"
#include "oled.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "ccd.h"
#include "Control.h"
#include "Motor.h"
#include "wifi.h"
#include "Encoder.h"
#include "pid.h"
#include <arm_math.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

osThreadId defaultTaskHandle;
osThreadId WIFIHandle;
osThreadId StartHandle;
osThreadId StatusHandle;
osThreadId RunHandle;
osThreadId SignalHandle;
osTimerId PIDControlHandle;
osSemaphoreId HandshakeSignalFirstHandle;
osSemaphoreId HandshakeSignalThirdHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
volatile  unsigned int OSIdleCtr;    /* 空闲计数器  */
unsigned int OSIdleCtrMax;             /* 一秒最大计数值    */
unsigned int OSIdleCtrRun;             /* 运行时一秒计数值  */
float     OSCPUUsage;               /* CPU占用率  */
float FreeHeapSpace;     //空闲内存空间

float pitch, roll, yaw;     //欧拉角
short aacx, aacy, aacz;     //加速度传感器原始数据
short gyrox, gyroy, gyroz;  //陀螺仪原始数据

uint8_t usart1_temp;        //USART1临时接收
uint8_t usart3_temp;        //USART3临时接收

uint8_t HandshakeSignalFirst = 0xf0;//与扫描单片机一次握手信号
uint8_t HandshakeSignalSecond = 0x01;//与扫描单片机二次握手信号
uint8_t HandshakeSignalThird = 0x0f;//与扫描单片机三次握手信号

uint8_t HandshakeSignalFirstSignal = 0;//与扫描单片机一次握手完成标志信号
uint8_t HandshakeSignalThirdSignal = 0;//与扫描单片机三次握手完成标志信号

uint8_t MotorFlag = 0;     //小车启动信号

int16_t MotorSpeed1;//右前编码器速度  正转负数，倒转正数
int16_t MotorSpeed2;//左后编码器速度  正转负数，倒转正数
int16_t MotorSpeed3;//左前编码器速度  正转正数，倒转负数
int16_t MotorSpeed4;//右后编码器速度  正转负数，倒转正数


PidTypeDef RFMotorPID;//右前轮PID
PidTypeDef LBMotorPID;//左后轮PID
PidTypeDef LFMotorPID;//左前轮PID
PidTypeDef RBMotorPID;//右后轮PID
PidTypeDef YawPID;    //角度PID

int32_t UpperSetSpeed = 20;//设定速度
float UpperSetYaw = 0.0; //设定角度
int32_t LeftForwardSpeed;//左前轮改变速度
int32_t RightForwardSpeed;//右前轮改变速度
uint8_t Judge[4] = {0}; //车库障碍判断
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM5_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM8_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void const * argument);
void StartWIFI(void const * argument);
void StartTask(void const * argument);
void StartStatus(void const * argument);
void StartRun(void const * argument);
void StartSignal(void const * argument);
void CallbackControl(void const * argument);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);


/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
#include "stdio.h"
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1 , (uint8_t *)&ch, 1 , 0xffff);
    return ch;
}


/* USER CODE END 0 */

int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration----------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_TIM5_Init();
    MX_USART1_UART_Init();
    MX_TIM1_Init();
    MX_TIM8_Init();
    MX_USART3_UART_Init();
    MX_ADC1_Init();

    /* USER CODE BEGIN 2 */
    OLED_Init();                    //初始化OLED
    MPU_Init();                 //初始化MPU6050

    while (mpu_dmp_init())
    {
        OLED_ShowString(4, 20, "MPU6050 Error", 16);
        OLED_Refresh_Gram();//更新显示到OLED
        HAL_Delay(200);
    }
    OLED_Clear();

    Encoder_init();
    Motor_init();
    PID_init();
    while (HAL_UART_Receive_IT(&huart1, &usart1_temp,
                               sizeof(usart1_temp)) != HAL_OK);//开启串口1中断
    while (HAL_UART_Receive_IT(&huart3, &usart3_temp,
                               sizeof(usart3_temp)) != HAL_OK);//开启串口3中断

    /* USER CODE END 2 */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* Create the semaphores(s) */
    /* definition and creation of HandshakeSignalFirst */
    osSemaphoreDef(HandshakeSignalFirst);
    HandshakeSignalFirstHandle = osSemaphoreCreate(osSemaphore(
                                     HandshakeSignalFirst), 1);

    /* definition and creation of HandshakeSignalThird */
    osSemaphoreDef(HandshakeSignalThird);
    HandshakeSignalThirdHandle = osSemaphoreCreate(osSemaphore(
                                     HandshakeSignalThird), 1);

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* Create the timer(s) */
    /* definition and creation of PIDControl */
    osTimerDef(PIDControl, CallbackControl);
    PIDControlHandle = osTimerCreate(osTimer(PIDControl), osTimerPeriodic, NULL);

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* Create the thread(s) */
    /* definition and creation of defaultTask */
    osThreadDef(defaultTask, StartDefaultTask, osPriorityIdle, 0, 256);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* definition and creation of WIFI */
    osThreadDef(WIFI, StartWIFI, osPriorityNormal, 0, 512);
    WIFIHandle = osThreadCreate(osThread(WIFI), NULL);

    /* definition and creation of Start */
    osThreadDef(Start, StartTask, osPriorityRealtime, 0, 256);
    StartHandle = osThreadCreate(osThread(Start), NULL);

    /* definition and creation of Status */
    osThreadDef(Status, StartStatus, osPriorityBelowNormal, 0, 256);
    StatusHandle = osThreadCreate(osThread(Status), NULL);

    /* definition and creation of Run */
    osThreadDef(Run, StartRun, osPriorityLow, 0, 512);
    RunHandle = osThreadCreate(osThread(Run), NULL);

    /* definition and creation of Signal */
    osThreadDef(Signal, StartSignal, osPriorityHigh, 0, 256);
    SignalHandle = osThreadCreate(osThread(Signal), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */


    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        printf("我不知道自己在找什么\r\n");
        printf("只觉得丢了很多\r\n");
        printf("二八二五六 二四二五七 二八二九三十一\r\n");
    }
    /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

    ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
    sConfig.Channel = ADC_CHANNEL_11;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

    TIM_MasterConfigTypeDef sMasterConfig;
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;
    TIM_OC_InitTypeDef sConfigOC;

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 16;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 1000;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim1);

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

    TIM_Encoder_InitTypeDef sConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 65535;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;
    if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM3 init function */
static void MX_TIM3_Init(void)
{

    TIM_Encoder_InitTypeDef sConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 65535;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;
    if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM4 init function */
static void MX_TIM4_Init(void)
{

    TIM_Encoder_InitTypeDef sConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 65535;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;
    if (HAL_TIM_Encoder_Init(&htim4, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM5 init function */
static void MX_TIM5_Init(void)
{

    TIM_Encoder_InitTypeDef sConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 0;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 65535;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;
    if (HAL_TIM_Encoder_Init(&htim5, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM8 init function */
static void MX_TIM8_Init(void)
{

    TIM_MasterConfigTypeDef sMasterConfig;
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;
    TIM_OC_InitTypeDef sConfigOC;

    htim8.Instance = TIM8;
    htim8.Init.Prescaler = 16;
    htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim8.Init.Period = 1000;
    htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim8.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim8);

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
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

}

/* USART3 init function */
static void MX_USART3_UART_Init(void)
{

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }

}

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, CCD_CLK_Pin | CCD_SI_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB,
                      MPU_SDA_Pin | MPU_SCL_Pin | OLED_SDA_Pin | OLED_SCL_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, OLED_DC_Pin | OLED_RST_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : CCD_CLK_Pin CCD_SI_Pin */
    GPIO_InitStruct.Pin = CCD_CLK_Pin | CCD_SI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : KEY_Pin */
    GPIO_InitStruct.Pin = KEY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : LED_Pin */
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : PE14 */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : MPU_SDA_Pin MPU_SCL_Pin OLED_SDA_Pin OLED_SCL_Pin */
    GPIO_InitStruct.Pin = MPU_SDA_Pin | MPU_SCL_Pin | OLED_SDA_Pin | OLED_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PA8 */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : OLED_DC_Pin OLED_RST_Pin */
    GPIO_InitStruct.Pin = OLED_DC_Pin | OLED_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pins : PB5 PB7 */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{

    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    for (;;)
    {
        osDelay(1);
    }
    /* USER CODE END 5 */
}

/* StartWIFI function */
void StartWIFI(void const * argument)
{
    /* USER CODE BEGIN StartWIFI */
    /* Infinite loop */
    for (;;)
    {
        Send_Float_Data(OSCPUUsage, CPUStatus);
        Send_Float_Data(FreeHeapSpace, LeftRoom);
        Send_Float_Data(yaw, Yaw);
        osDelay(500);
        Send_Uint8_Data(MotorSpeed3, LSpeed);
        Send_Uint8_Data(MotorSpeed1, RSpeed);
        osDelay(500);
    }
    /* USER CODE END StartWIFI */
}

/* StartTask function */
void StartTask(void const * argument)
{
    /* USER CODE BEGIN StartTask */
    /* Infinite loop */
    for (;;)
    {
        osThreadSuspend(RunHandle);
        osThreadSuspend(WIFIHandle);
        osThreadSuspend(StatusHandle);
        osThreadSuspend(SignalHandle);

        taskENTER_CRITICAL();  //进入临界区
        OSIdleCtr = 0;
        taskEXIT_CRITICAL();  //退出临界区

        osDelay(1000);

        taskENTER_CRITICAL();
        OSIdleCtrMax = OSIdleCtr;//延时一秒后取得一秒运行次数最大值
        taskEXIT_CRITICAL();

        OSIdleCtrMax /= 100;  //方便算百分率
        if (OSIdleCtrMax == 0)
        {
            OSCPUUsage = 0;
        }


        //被挂起任务的恢复
        //osThreadResume(SignalHandle);
        osThreadResume(RunHandle);
        osThreadResume(WIFIHandle);
        osThreadResume(StatusHandle);
        osTimerStart(PIDControlHandle, 10);

        osThreadTerminate( NULL);    //任务删除

        printf("不知不觉不问不痛不痒多少的时光\r\n");
        printf("不确定的夜晚小巷泪悄悄流淌\r\n");

        osDelay(1);
    }
    /* USER CODE END StartTask */
}

/* StartStatus function */
void StartStatus(void const * argument)
{
    /* USER CODE BEGIN StartStatus */
    /* Infinite loop */
    for (;;)
    {
        taskENTER_CRITICAL();
        FreeHeapSpace = xPortGetFreeHeapSize() / 1024; //bytes转Kbytes
        OSIdleCtrRun =
            OSIdleCtr;                /*获得上一秒运行数 */
        OSIdleCtr    =
            0;                      /* 下一秒清零 */
        taskEXIT_CRITICAL();
        OSCPUUsage   = (100 - OSIdleCtrRun / (float) OSIdleCtrMax) ;
        OSCPUUsage = ABS(OSCPUUsage); //取绝对值，防止意外产生

        osDelay(1000);       /* 1s统计一次   */
    }
    /* USER CODE END StartStatus */
}

/* StartRun function */
void StartRun(void const * argument)
{
    /* USER CODE BEGIN StartRun */
    uint8_t  temp;
    Stop();
    OLED_ShowString(0, 10, "Bluetooth Error", 16);
    OLED_Refresh_Gram();//更新显示到OLED
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);//关灯
    /*握手协议
    扫描单片机发送HandshakeSignalFirst至小车，
      小车接收到这个信号则发送HandshakeSignalSecond至扫描单片机
    扫描单片机发送HandshakeSignalThird至小车，小车收到就开始下一步
    */
//  while(osSemaphoreWait(HandshakeSignalFirstHandle, 0xff)!=osOK);
//  while(osSemaphoreWait(HandshakeSignalThirdHandle, 0xff)!=osOK)
//  {
//          HAL_UART_Transmit(&huart1 , &HandshakeSignalSecond, 1, 0xff);
//              osDelay(10);
//  }


    OLED_Clear();

    OLED_ShowString(0, 10, "Ready", 24);
    OLED_Refresh_Gram();//更新显示到OLED


    while (!MotorFlag)
    {
        if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)//按下
        {
            MotorFlag = 1;
        }
    }

    /* Infinite loop */
    for (;;)
    {
        if (MotorFlag)
        {
            //出库
            for (temp = 0; temp < 80; temp++)
            {
                UpperSpeedControl(-20);
                osDelay(10);
            }
            //转90度  车屁股向右
            for (temp = 0; temp < 120; temp++)
            {
                GyrateRight();
                osDelay(10);
            }
            //一二没有障碍
//            if (Judge[2] && Judge[3])
//            {
//							Send_Uint8_Data(0x12,JUDGE);
////倒车走一段
//                for (temp = 0; temp < 80; temp++)
//                {
//                    UpperSpeedControl(-20);
//                    osDelay(10);
//                }
////右拐入库
//                for (temp = 0; temp < 120; temp++)
//                {
//                    GyrateRight();
//                    osDelay(10);
//                }
////暴力右拐  进侧位车库
//                for (temp = 0; temp < 120; temp++)
//                {
//                    GyrateRight();
//                    osDelay(10);
//                }
//回去
                {

                }
//            }
#if 0           
						//一三没有障碍
            else if (Judge[1] && Judge[3])
            {
							Send_Uint8_Data(0x13,JUDGE);
                //倒车走一段
                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(-20);
                    osDelay(10);
                }
//右拐入库
                for (temp = 0; temp < 120; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
                //出库
                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(20);
                    osDelay(10);
                }
                //车屁股校正
                for (temp = 0; temp < 80; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
//停进第三个车库
                for (temp = 0; temp < 80; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
//回去
                {

                }
            }
            //二四没有障碍
            else if (Judge[0] && Judge[2])
            {
							Send_Uint8_Data(0x24,JUDGE);
//倒车走一段  先进四
                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(-20);
                    osDelay(10);
                }
//右拐入库
                for (temp = 0; temp < 120; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
	//停进二 先向前走一段
	                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(20);
                    osDelay(10);
                }	
//然后右转强行进								
	                for (temp = 0; temp < 120; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
//回去
{

}								
            }
            //三四没有障碍
            else if (Judge[0] && Judge[1])
            {
							Send_Uint8_Data(0x34,JUDGE);
//倒车走一段  先进四
                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(-20);
                    osDelay(10);
                }
//右拐入库
                for (temp = 0; temp < 120; temp++)
                {
                    GyrateRight();
                    osDelay(10);
                }
	//停进三 先向前走一段
	                for (temp = 0; temp < 80; temp++)
                {
                    UpperSpeedControl(20);
                    osDelay(10);
                }	
//然后左转强行进								
	                for (temp = 0; temp < 120; temp++)
                {
                    GyrateLeft();
                    osDelay(10);
                }
//回去
								{
								
								}
            }
            #endif
						Stop();
            MotorFlag = 0;
						HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);//开灯
						Send_Uint8_Data(0xff,OK);
        }
        osDelay(1);
    }

    /* USER CODE END StartRun */
}

/* StartSignal function */
void StartSignal(void const * argument)
{
    /* USER CODE BEGIN StartSignal */
    uint8_t temp[2] = {0};
    osSemaphoreWait(HandshakeSignalFirstHandle, 0xff);
    osSemaphoreWait(HandshakeSignalThirdHandle, 0xff);
    osThreadResume(RunHandle);
    /* Infinite loop */
    for (;;)
    {

        if (HandshakeSignalFirstSignal == 1)
        {
            osSemaphoreRelease(HandshakeSignalFirstHandle);
            temp[0] = 1;
        }
        if (HandshakeSignalThirdSignal == 1)
        {
            osSemaphoreRelease(HandshakeSignalThirdHandle);
            temp[1] = 1;
        }
        if (temp[0] && temp[1])
        {
            osThreadTerminate( NULL);    //任务删除
            printf("我他妈就是一个傻叉");
        }
        osDelay(10);
    }
    /* USER CODE END StartSignal */
}

/* CallbackControl function */
void CallbackControl(void const * argument)
{
    /* USER CODE BEGIN CallbackControl */
    MotorSpeed1 = GetEncoderValue(htim2);
    MotorSpeed2 = GetEncoderValue(htim3);
    MotorSpeed3 = GetEncoderValue(htim4);
    MotorSpeed4 = GetEncoderValue(htim5);

    if (mpu_dmp_get_data(&pitch, &roll, &yaw) == 0)
    {
        MPU_Get_Accelerometer(&aacx, &aacy, &aacz); //得到加速度传感器数据
        MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);  //得到陀螺仪数据
        myOLED_Decimals(40, 40, yaw);
        OLED_Refresh_Gram();//更新显示到OLED

    }
//    if (MotorFlag==1)
//    {
//        Pid_Control(yaw,  UpperSetYaw, UpperSetSpeed);
//    }
//      else if(MotorFlag==2)
//      {
//              UpperSpeedControl(UpperSetSpeed);
//      }
//    else
//    {
//        Stop();
//    }
    /* USER CODE END CallbackControl */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM6)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler */
    /* User can add his own implementation to report the HAL error return state */
    while (1)
    {
    }
    /* USER CODE END Error_Handler */
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
      ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */

}

#endif

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
