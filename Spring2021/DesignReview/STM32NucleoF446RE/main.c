#include "main.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "SerialDebug.h"
#include "VL6180X.h"
#include "bme280.h"

#define RANGE_SAMPLES 500
#define TEST_SAMPLES 200
#define MIN_OPERATING_VOLTAGE 3.3

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

ADC_HandleTypeDef hadc1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);

/* User private function prototypes */
bool CheckTestButton();
bool check_range();
float ** TestMask();
float ** GetBaselineReadings(struct BME280_calib_data calib);
int TransmitData(float ** mask_data, int sizeInBytes);
void ConfigureTransmission();
void EnableRegulator(float batteryVoltage);
void DISTANCE_LED_OFF();
void DISTANCE_LED_ON();
void RESPIRATION_LED_ON();
void RESPIRATION_LED_OFF();
void COMPLETE_LED_ON();
void COMPLETE_LED_OFF();
void Free2DFloat(float ** floatToClear);
float CheckBatteryVoltage();
float CheckRegulatorVoltage();

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
	  MX_USART6_UART_Init();
	  MX_ADC1_Init();

	  VL6180X_init();
	  BME280_init();


	  // Array to hold temperature and RH data
	  float ** BME280_data;
	  float ** baselineReadings;
	  float batteryVoltage;

	  // Buffer to be used for debugging
	  char buffer[50];

	  // Variable to check if test button is pressed, indicating a test should be started
	  bool test_started = false;

	  // Get the battery voltage, and determine if battery is at or exceeding an acceptable operating voltage
	  batteryVoltage = CheckBatteryVoltage();
	  EnableRegulator(batteryVoltage);

	  // Get our factory written BME280 calibration data. Need it to read data
	  struct BME280_calib_data calib;
	  BME280_get_calib_data(&calib);

	  /* USER CODE BEGIN WHILE */
	  while (1)
	  {
		  // Check for if a test should be started
		  test_started = CheckTestButton();
		  if (test_started){

			  // Delay for 3 seconds so ESP8266 can catch up after reset
			  HAL_Delay(3000);
			  COMPLETE_LED_OFF();

			  // Get baseline temperature and humidity, send it to ESP8266, and free that memory
			  baselineReadings = GetBaselineReadings(calib);
			  TransmitData(baselineReadings, 1);
			  Free2DFloat(baselineReadings);

			  // Send ESP8266 the number of samples to be taken
			  ConfigureTransmission();

			  // A nice debug to alert BME280 now being polled for test data
			  DebugLog("Beginning Test Now!\r\n");

			  RESPIRATION_LED_ON();
			  BME280_data = TestMask(calib);
			  RESPIRATION_LED_OFF();

			  for (int k = 0; k < TEST_SAMPLES; k++)
			  {
				  // Again, a nice Debug feature that sends all the measurements to Debug line. Useful for when ESP8266 has hiccup.
				  sprintf(buffer, "Temperature: %f --- Humidity: %f\r\n", BME280_data[TEMPERATURE][k], BME280_data[HUMIDITY][k]);
				  DebugLog(buffer);
			  }
			  
			  // Transmit our newly acquired test data, and proceed to free the memory
			  DebugLog("Test Complete!\r\nSending Data to RF transmitter\r\n");
			  TransmitData(BME280_data, TEST_SAMPLES);
			  Free2DFloat(BME280_data);
			  COMPLETE_LED_ON();


		  }


	  }
}

// Clears the standard BME280 double pointer float. Frees the inner arrays, then frees parent array
void Free2DFloat(float ** floatToClear)
{
	free(floatToClear[TEMPERATURE]);
	free(floatToClear[HUMIDITY]);

	free(floatToClear);

	return;
}

float ** GetBaselineReadings(struct BME280_calib_data calib)
{

	// Array to hold current temp and humidity read back from sensor, and to be appended to list of data
	float * current_data;

	// Create a parent array for all the data to be sent to base station
	float ** baseline_data = (float **)malloc(2 * sizeof(float *));

	// Check to make sure malloc was successful, otherwise return appropriate error
	if (baseline_data == NULL)
	{

		DebugLog("Error allocating memory!\r\n");
		exit(-1);

	}

	// Create sub arrays for each sensor type (TEMP and HUMIDITY)
	baseline_data[TEMPERATURE] = (float *) malloc(sizeof(float));
	baseline_data[HUMIDITY] = (float *) malloc(sizeof(float));

	// Check to make sure malloc was successful, otherwise return appropriate error
	if (baseline_data[TEMPERATURE] == NULL || baseline_data[HUMIDITY] == NULL)
	{

		DebugLog("Error allocating memory!\r\n");
		exit(-1);

	}

	// First read all data, then split accordingly
	current_data = BME280_read_data(&calib);
	baseline_data[HUMIDITY][0] = current_data[HUMIDITY];
	baseline_data[TEMPERATURE][0] = current_data[TEMPERATURE];

	// And return the data measured from the BME280
	return baseline_data;

}

// Function reads ADC for battery voltage, and performs necessary software manipulations to get actual battery voltage (approximation)
float CheckBatteryVoltage()
{
	float real;
	uint16_t raw;
	uint8_t result;

	// Begin polling ADC for input voltage
	result = HAL_ADC_Start(&hadc1);

	if (result != HAL_OK)
	{

		DebugLog("Error starting ADC to ensure input voltage is above threshold. Shutting down to prevent damage!\r\n");
		exit(-1);

	}

	result = HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

	if (result != HAL_OK)
	{

		DebugLog("ADC started correctly, but could not poll for conversion. Shutting down to prevent damage\r\n");
		exit(-1);

	}

	// Get the raw value
	raw = HAL_ADC_GetValue(&hadc1);

	// With raw value from voltage divider, convert back to voltage using STM datasheet ADC equation
    real = (((float)raw / 4095)) * 3.3;

    // And multiply by 2 to get the voltage into the board (Undo the voltage division done in hardware)
    real *= 2;
    //HAL_Delay(100);

	return real;
}

// Function outputs high signal to TLV62569 permitted battery voltage is greater than MIN_OPERATING_VOLTAGE
void EnableRegulator(float batteryVoltage)
{
	char buffer[200];
	if (batteryVoltage < MIN_OPERATING_VOLTAGE)
	{

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		sprintf(buffer ,"Battery Voltage: %fV, acceptable, enabling regulator!\r\n", batteryVoltage);
		DebugLog(buffer);

	}

	else
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
		sprintf(buffer, "Battery Voltage: %fV, did not meet threshold. Powering down!\r\n", batteryVoltage);
		DebugLog(buffer);

	}

	return;
}

bool CheckTestButton()
{
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) != GPIO_PIN_SET)
	{

		return true;

	}

	return false;

}

// Writes to the ESP8266 how many samples are being transmitted
void ConfigureTransmission()
{
	uint8_t result;

	union {
		int samples_taken;
		uint8_t samples_taken_to_send[4];
	} samples;

	samples.samples_taken = TEST_SAMPLES;

	// Send out the number of samples to take
	result = HAL_UART_Transmit(&huart6, samples.samples_taken_to_send, 4, HAL_MAX_DELAY);

	if (result != HAL_OK)
	{

		DebugLog("Error sending number of samples to ESP8266!\r\n");

	}

	return;
}

// Function takes in a 2D float, and a number of floats to send, and converts to bytes and then sends said bytes over UART
int TransmitData(float ** mask_data, int sizeInBytes)
{
	// Used to check return of HAL functions
	uint8_t result;

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
	for (int i = 0; i < sizeInBytes; i++)
	{
		temp_out.temp_to_send = mask_data[TEMPERATURE][i];
		humidity_out.humidity_to_send = mask_data[HUMIDITY][i];

		result = HAL_UART_Transmit(&huart6, temp_out.bytes_to_send, 4, HAL_MAX_DELAY);

		if (result != HAL_OK)
		{

			DebugLog("Error sending temperature to ESP8266!\r\n");

		}

		result = HAL_UART_Transmit(&huart6, humidity_out.bytes_to_send, 4, HAL_MAX_DELAY);

		if (result != HAL_OK)
		{

			DebugLog("Error sending humidity to ESP8266!\r\n");

		}
		
	}
	
	DebugLog("Done sending data to ESP8266!\r\n");

	return 0;
}

// Function used BME280 calibration struct, and returns a 2D float of BME280 temp and humidity samples of size TEST_SAMPLES
float ** TestMask(struct BME280_calib_data calib)
{

	bool in_range = false;
	char countdown[50];


	// Create a parent array for all the data to be sent to base station
	float ** mask_data = (float **)malloc(2 * sizeof(float *));

	// Check to make sure malloc was successful, otherwise return appropriate error
	if (mask_data == NULL)
	{

		DebugLog("Error allocating memory!\r\n");
		exit(-1);

	}

	// Create sub arrays for each sensor type (TEMP and HUMIDITY)
	mask_data[TEMPERATURE] = (float *) malloc(sizeof(float) * TEST_SAMPLES);
	mask_data[HUMIDITY] = (float *) malloc(sizeof(float) * TEST_SAMPLES);

	// Check to make sure malloc was successful, otherwise return appropriate error
	if (mask_data[TEMPERATURE] == NULL || mask_data[HUMIDITY] == NULL)
	{

		DebugLog("Error allocating memory!\r\n");
		exit(-1);

	}

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

void DISTANCE_LED_OFF()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
	return;
}

void DISTANCE_LED_ON()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	return;
}

void RESPIRATION_LED_OFF()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	return;
}

void RESPIRATION_LED_ON()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	return;
}


void COMPLETE_LED_OFF()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	return;
}

void COMPLETE_LED_ON()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	return;
}

// Function uses the VL6180X to check if anyone is in range, and if they stay in range for RANGE_SAMPLES, returns true
bool check_range()
{
	// Hold previous RANGE_SAMPLES in an array, and check that all meet spec (are within 0.5 to 4 inches). This should conclude the testee is in the right spot
	uint8_t range_readings[RANGE_SAMPLES];

	DISTANCE_LED_ON();

	// First fill our buffer of measurements
	for (int iter = 0; iter < RANGE_SAMPLES; iter++)
	{
		// Get a new reading
		range_readings[iter] = VL6180X_single_range_measurement();

		// If any reading is less than 13 mm or greater than 101 mm (Roughly 0.5 to 4 inches)
		if ((range_readings[iter] < 13) || (range_readings[iter] > 101))
		{
			// Out of range. Try checking range once more
			return false;
		}

	}

	DISTANCE_LED_OFF();
	return true;

}

/* STM32 Peripheral Setup Functions */

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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.ClockSpeed = 100000;
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
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC1 PC2 PC3 PC4
                           PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4
                          |GPIO_PIN_8;
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
