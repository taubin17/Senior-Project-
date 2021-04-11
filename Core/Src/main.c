/* USER CODE BEGIN Header */
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SerialDebug.h"
#include "VL6180X.h"
#include "bme280.h"

#define RANGE_SAMPLES 500
#define TEST_SAMPLES 200

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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_UART6_Init(void);
static void MX_I2C1_Init(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

bool CheckTestButton();
bool check_range();
float ** TestMask();
int TransmitData(float ** mask_data);

void ConfigureTransmission();
void EnableRegulator();

static void TOGGLE_DISTANCE_LED();
static void DISTANCE_LED_ON();

int main(void)
{

	  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	  HAL_Init();

	  /* Configure the system clock */
	  SystemClock_Config();

	  /* Initialize all configured peripherals */
	  MX_GPIO_Init();
	  MX_USART2_UART_Init();
	  MX_UART4_Init();
	  MX_I2C1_Init();
	  MX_GPIO_Init();
	  MX_USART1_UART_Init();

	  VL6180X_init();
	  BME280_init();

	  // Initialize our calibration data struct, then get the calibration data from the BME280


	  // Array to hold temperature and RH data
	  float ** BME280_data;

	  // Variable to check if test button is pressed, indicating a test should be started
	  bool test_started = false;
	
	  // Outputs high signal to enable pin of regulator.
	  EnableRegulator();
	
	  // Add newline to debug serial port to increase readability between tests
	  DebugLog("\r\n");
	
	  /* USER CODE BEGIN WHILE */
	  while (1)
	  {

		  // Check for if a test should be started
		  test_started = CheckTestButton();
		  
		  if (test_started){
			  // Tell the ESP8266 how many samples will be measured during test (how many bytes will be sent)
			  ConfigureTransmission();
			  
			  DebugLog("Beginning Test Now!\r\n");
			  //HAL_UART_Transmit(&huart4, buffer, strlen((const char *)buffer), HAL_MAX_DELAY);
			  //HAL_Delay(500);
			  //sprintf(buffer, "%d\r\n", range_test);
			  //DebugLog(buffer);
			  
			  // Check if testee is in range. Once in range, read mask data TEST_SAMPLES times 
			  BME280_data = TestMask();

			  // For each sample, debug the samples RH and temp
			  for (int k = 0; k < TEST_SAMPLES; k++)
			  {
				  sprintf(buffer, "Temperature: %f --- Humidity: %f\r\n", BME280_data[TEMPERATURE][k], BME280_data[HUMIDITY][k]);
				  DebugLog(buffer);
			  }
			  DebugLog("Test Complete!\r\nSending Data to RF transmitter\r\n");
			  // Send the data to ESP8266 over serial.
			  TransmitData(BME280_data);


		  }


	  }
}

// Function outputs high signal to TLV62569
void EnableRegulator() {
	// Sets PC4 pin to High, driving enable pin of TLV62569
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);

}
/**
  * @brief System Clock Configuration
  * @retval None
  */

bool CheckTestButton()
{
	/*
	 * This function will be changed in the place of an actual push button
	 * For now, it simply reads when the button is pressed down.
	 * Upon press, the test begins
	 */
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_SET)
	{

		return true;

	}

	return false;

}

// Writes to the ESP8266 how many samples are being transmitted
void ConfigureTransmission()
{
	union {
		int samples_taken;
		uint8_t samples_taken_to_send[4];
	} samples;

	samples.samples_taken = TEST_SAMPLES;

	// Send out the number of samples to take
	HAL_UART_Transmit(&huart4, samples.samples_taken_to_send, 4, HAL_MAX_DELAY);

	return;
}


int TransmitData(float ** mask_data)
{

	// Union to convert floating point temp data to byte array to be transferred via UART
	union {

		//float test_num;
		float temp_to_send;
		unsigned char bytes_to_send[4];

	} temp_out;

	// Similar union to convert floating point humidity data to byte array for UART transfer
	union {

		//float test_num;
		float humidity_to_send;
		unsigned char bytes_to_send[4];

	} humidity_out;


	// For each sample, convert humidity and temperature to byte array, then send it to ESP8266 via UART
	for (int i = 0; i < TEST_SAMPLES; i++)
	{
		temp_out.temp_to_send = mask_data[TEMPERATURE][i];
		humidity_out.humidity_to_send = mask_data[HUMIDITY][i];

		HAL_UART_Transmit(&huart4, temp_out.bytes_to_send, 4, HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart4, humidity_out.bytes_to_send, 4, HAL_MAX_DELAY);

	}
	DebugLog("Done sending data to ESP8266!\r\n");

	// Transmission Completed! Cleanup current test data in preparation for another
	free(mask_data[TEMPERATURE]);
	free(mask_data[HUMIDITY]);
	free(mask_data);

	return 0;
}


float ** TestMask()
{

	bool in_range = false;
	char countdown[50];

	// Get our factory written BME280 calibration data. Need it to read data
	struct BME280_calib_data calib;
	BME280_get_calib_data(&calib);

	// Create a parent array for all the data to be sent to base station
	float ** mask_data = (float **)malloc(2 * sizeof(float *));

	// Create sub arrays for each sensor type (TEMP and HUMIDITY)
	mask_data[TEMPERATURE] = (float *) malloc(sizeof(float) * TEST_SAMPLES);
	mask_data[HUMIDITY] = (float *) malloc(sizeof(float) * TEST_SAMPLES);

	// Array to hold current temp and humidity read back from sensor, and to be appended to list of data
	float * current_data;

	// Wait until testee is in range
	while (in_range == false)
	{
		in_range = check_range();
	}


	// Countdown for user to see, will be replaced in future more than likely by LED's
	DebugLog("Please wait until the countdown reaches 0 to begin breathing into the device\r\n");
	for (int iter = 3; iter > 0; iter--)
	{
		sprintf(countdown, "BEGIN TESTING IN: %d\r\n", iter);
		DebugLog(countdown);

		//Sleep for 1 second
		HAL_Delay(1000);
	}

	// Now begin sampling the relative humidity and temperature data
	for (int sample_number = 0; sample_number < TEST_SAMPLES; sample_number++)
	{
		current_data = BME280_read_data(&calib);

		HAL_Delay(30);

		// Add to our array of measurements to be sent to ESP via Serial
		mask_data[TEMPERATURE][sample_number] = current_data[TEMPERATURE];
		mask_data[HUMIDITY][sample_number] = current_data[HUMIDITY];
		//sprintf(countdown, "Sample Number: %d --- HUMIDITY: %f\r\n", sample_number, mask_data[HUMIDITY][sample_number]);
		//DebugLog(countdown);
	}

	return mask_data;
}

static void DISTANCE_LED_OFF()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
	return;
}

static void DISTANCE_LED_ON()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
	return;
}

bool check_range()
{
	// Hold previous RANGE_SAMPLES in an array, and check that all meet spec (are within 0.5 to 4 inches). This should conclude the testee is in the right spot
	uint8_t range_readings[RANGE_SAMPLES];

	// First fill our buffer of measurements
	for (int iter = 0; iter < RANGE_SAMPLES; iter++)
	{
		// Get a new reading
		range_readings[iter] = VL6180X_single_range_measurement();

		// If any reading is less than 13 mm or greater than 101 mm (Roughly 0.5 to 4 inches)
		if ((range_readings[iter] < 13) || (range_readings[iter] > 101))
		{
			// Out of range. Try checking range once more

			DISTANCE_LED_OFF();
			return false;
		}

		DISTANCE_LED_ON();

	}

	DISTANCE_LED_OFF();
	return true;

}
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
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

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
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
