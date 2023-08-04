/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MotorControl.hpp"
#include "tim.h"
#include "MPU_6050_Driver.hpp"
#include "i2c.h"
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
/* USER CODE BEGIN Variables */
Motor right_b(&htim1, &htim3, TIM_CHANNEL_1, TIM_CHANNEL_4);
Motor right_f(&htim1, &htim1, TIM_CHANNEL_3, TIM_CHANNEL_4);

Motor left_f(&htim3, &htim2, TIM_CHANNEL_1, TIM_CHANNEL_2);
Motor left_b(&htim4, &htim3, TIM_CHANNEL_1, TIM_CHANNEL_2);

MPU_6050 mpu1(true, &hi2c1, GPIOC, GPIO_PIN_15);
MPU_6050 mpu2(false, &hi2c1, GPIOB, GPIO_PIN_2);

IMU_Data *imuData1, *imuData2;


const double lenBot = 0.2765;
Chasis car(lenBot, &right_b, &right_f, &left_b, &left_f, false);
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for controlSpeed */
osThreadId_t controlSpeedHandle;
const osThreadAttr_t controlSpeed_attributes = {
  .name = "controlSpeed",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for microROS */
osThreadId_t microROSHandle;
const osThreadAttr_t microROS_attributes = {
  .name = "microROS",
  .stack_size = 3000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void startMicroROS(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
//  right_b.init();
//  right_f.init();
//  left_b.init();
//  left_f.init();
  car.init();
  mpu1.startPins();
  mpu2.startPins();
  mpu1.initialize();
  mpu2.initialize();
  imuData1 = mpu1.generateNewResult();
  imuData2 = mpu2.generateNewResult();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of controlSpeed */
  controlSpeedHandle = osThreadNew(StartTask02, NULL, &controlSpeed_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  microROSHandle = osThreadNew(startMicroROS, NULL, &microROS_attributes);
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    car.drive();
	osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the controlSpeed thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  car.steer(-0.3,1);
  for(;;)
  {
	mpu1.updateData();
    mpu2.updateData();
    osDelay(20);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_startMicroROS */
/**
* @brief Function implementing the microROS thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_startMicroROS */
void startMicroROS(void *argument)
{
  /* USER CODE BEGIN startMicroROS */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END startMicroROS */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

