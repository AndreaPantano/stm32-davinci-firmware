/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

USART_HandleTypeDef husart1;
UART_HandleTypeDef huart2;

bool has_flown;

/* Definitions for StartupTask */
osThreadId_t StartupTaskHandle;
const osThreadAttr_t StartupTask_attributes = {
  .name = "StartupTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for FlashWriteTask */
osThreadId_t FlashWriteTaskHandle;
const osThreadAttr_t FlashWriteTask_attributes = {
  .name = "FlashWriteTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SensorsReadTask */
osThreadId_t SensorsReadTaskHandle;
const osThreadAttr_t SensorsReadTask_attributes = {
  .name = "SensorsReadTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for COMBoardTask */
osThreadId_t COMBoardTaskHandle;
const osThreadAttr_t COMBoardTask_attributes = {
  .name = "COMBoardTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for FlightFSMTask */
osThreadId_t FlightFSMTaskHandle;
const osThreadAttr_t FlightFSMTask_attributes = {
  .name = "FlightFSMTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SystemHealthCheckTask */
osThreadId_t SystemHealthCheckTaskHandle;
const osThreadAttr_t SystemHealthCheckTask_attributes = {
  .name = "SystemHealthCheckTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */
/* SENSORS AND DEVICES DECLARATION */
W25Q128_t flash = {0};
ExtU rtU = {0};
ExtY rtY = {0};

struct bmp3_dev bmp390_1 = {0}, bmp390_2 = {0};
stmdev_ctx_t imu_1 = {0}, imu_2 = {0};

pitot_sensor_t pitot = {0};
uint8_t output[4000] = {0};
servo_t servo = {0};

buzzer_t buzzer = {0};

/* DECLARATION AND DEFINITION OF THE GLOBAL VARIABLES FOR THE FLIGHT */

flight_phase_t flight_phase = CALIBRATING;

flight_fsm_t flight_state;


//char num_acc_modify[]={0,0,0};
//char num_gyro_modify[]={0,0,0};

struct bmp3_data barometer_data_1 = {-1, -1};
struct bmp3_data barometer_data_2 = {-1, -1};

static int16_t data_raw_angular_rate_1[3] = {0};
static int16_t data_raw_acceleration_1[3] = {0};
static int16_t data_raw_angular_rate_2[3] = {0};
static int16_t data_raw_acceleration_2[3] = {0};

static float_t acceleration_mg_1[3] = {0};
static float_t angular_rate_mdps_1[3] = {0};
static float_t acceleration_mg_2[3] = {0};
static float_t angular_rate_mdps_2[3] = {0};

linear_acceleration_t curr_acc = {0};
linear_acceleration_t prev_acc = {0};

typedef struct {
	float_t acc_x;
	float_t acc_y;
	float_t acc_z;

	float_t dps_x;
	float_t dps_y;
	float_t dps_z;

	float_t temperature;
	float_t pressure;
	float_t altitude;

	bool invalid;
	bool calibrating;
	bool ready;
	bool jelqing;
	bool burning;
	bool coasting;
	bool drogue;
	bool main;
	bool touchdown;

} sensor_data;


uint16_t num_meas_stored_in_buffer = 0;
uint16_t num_meas_stored_in_flash = 0;

uint8_t *measurements_buffer;
uint8_t flash_address[3] = {0};

float_t altitude;
bool first_measure = true;
float_t velocity;
float_t Pressure_1;
float_t Pressure_2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM4_Init(void);
void Startup(void *argument);
void FlashWrite(void *argument);
void SensorsRead(void *argument);
void CommunicationBoard(void *argument);
void FlightFSM(void *argument);
void SystemHealthCheck(void *argument);

/* USER CODE BEGIN PFP */

//static uint32_t GetSector(uint32_t Address)
//{
//  uint32_t sector = 0;
//
//  if((Address < 0x08003FFF) && (Address >= 0x08000000))
//  {
//    sector = FLASH_SECTOR_0;
//  }
//  else if((Address < 0x08007FFF) && (Address >= 0x08004000))
//  {
//    sector = FLASH_SECTOR_1;
//  }
//  else if((Address < 0x0800BFFF) && (Address >= 0x08008000))
//  {
//    sector = FLASH_SECTOR_2;
//  }
//  else if((Address < 0x0800FFFF) && (Address >= 0x0800C000))
//  {
//    sector = FLASH_SECTOR_3;
//  }
//  else if((Address < 0x0801FFFF) && (Address >= 0x08010000))
//  {
//    sector = FLASH_SECTOR_4;
//  }
//  else if((Address < 0x0803FFFF) && (Address >= 0x08020000))
//  {
//    sector = FLASH_SECTOR_5;
//  }
//  else if((Address < 0x0805FFFF) && (Address >= 0x08040000))
//  {
//    sector = FLASH_SECTOR_6;
//  }
//  else if((Address < 0x0807FFFF) && (Address >= 0x08060000))
//  {
//    sector = FLASH_SECTOR_7;
//  }
//
//  return sector;
//}
//
//
//uint32_t Flash_Write_Data (uint32_t StartSectorAddress, uint32_t *Data, uint16_t numberofwords)
//{
//
//	static FLASH_EraseInitTypeDef EraseInitStruct;
//	uint32_t SECTORError;
//	int sofar=0;
//
//
//	 /* Unlock the Flash to enable the flash control register access *************/
//	  HAL_FLASH_Unlock();
//
//	  /* Erase the user Flash area */
//
//	  /* Get the number of sector to erase from 1st sector */
//
//	  uint32_t StartSector = GetSector(StartSectorAddress);
//	  uint32_t EndSectorAddress = StartSectorAddress + numberofwords*4;
//	  uint32_t EndSector = GetSector(EndSectorAddress);
//
//	  /* Fill EraseInit structure*/
//	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
//	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
//	  EraseInitStruct.Sector        = StartSector;
//	  EraseInitStruct.NbSectors     = (EndSector - StartSector) + 1;
//
//	  /* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
//	     you have to make sure that these data are rewritten before they are accessed during code
//	     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
//	     DCRST and ICRST bits in the FLASH_CR register. */
//	  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
//	  {
//		  return HAL_FLASH_GetError ();
//	  }
//
//	  /* Program the user Flash area word by word
//	    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/
//
//	   while (sofar<numberofwords)
//	   {
//	     if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartSectorAddress, Data[sofar]) == HAL_OK)
//	     {
//	    	 StartSectorAddress += 4;  // use StartPageAddress += 2 for half word and 8 for double word
//	    	 sofar++;
//	     }
//	     else
//	     {
//	       /* Error occurred while writing data in Flash memory*/
//	    	 return HAL_FLASH_GetError ();
//	     }
//	   }
//
//	  /* Lock the Flash to disable the flash control register access (recommended
//	     to protect the FLASH memory against possible unwanted operation) *********/
//	  HAL_FLASH_Lock();
//
//	   return 0;
//}
//
//void Flash_Read_Data (uint32_t StartSectorAddress, uint32_t *RxBuf, uint16_t numberofwords)
//{
//	while (1)
//	{
//
//		*RxBuf = *(__IO uint32_t *)StartSectorAddress;
//		StartSectorAddress += 4;
//		RxBuf++;
//		if (!(numberofwords--)) break;
//	}
//}
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
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART1_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  	codegen_model_initialize();
  	rtU.Altitudeinput = (double)0.0;
  	rtU.Verticalvelocityinput = (double)0.0;
  	size_t i = 0;
	int8_t result = 0;
	int8_t result2 = 0;
	measurements_buffer =malloc(sizeof(sensor_data) * (FLASH_NUMBER_OF_STORE_EACH_TIME * 2));


	/* deactivate chip select of IMU and BMP390 */
	IMU1_CS_HIGH;
	IMU2_CS_HIGH;
	BMP390_1_CS_HIGH;
	BMP390_2_CS_HIGH;

	LED_ON(DEBUG_LED_FLASH_GPIO_Port, DEBUG_LED_FLASH_Pin);

	result = init_imu1(&imu_1, LSM6DSO32_16g, LSM6DSO32_2000dps, LSM6DSO32_XL_ODR_208Hz_NORMAL_MD, LSM6DSO32_GY_ODR_208Hz_HIGH_PERF);
	result = init_imu2(&imu_2, LSM6DSO32_16g, LSM6DSO32_2000dps, LSM6DSO32_XL_ODR_208Hz_NORMAL_MD, LSM6DSO32_GY_ODR_208Hz_HIGH_PERF);

	result = init_bmp390_1(&bmp390_1);
	result = init_bmp390_2(&bmp390_2);

//	result = init_flash(&flash, ERASE); //XXX use this once to erase the chip
	result = init_flash(&flash, BYPASS);//XXX use this everytime the chip does not need to be erased

	if (result == 0)	LED_OFF(DEBUG_LED_FLASH_GPIO_Port, DEBUG_LED_FLASH_Pin);
  	uint8_t angle = 0;
	uint8_t dataflag[1];
	uint8_t address[3] = {0, 0, 0};
	uint8_t result_first_read = W25Q128_read_flown_flag(&flash, address, dataflag, 1);

	if (result_first_read != HAL_OK){
		return HAL_ERROR;
	}

	if (dataflag[0]==1){
		has_flown = true;
	}else{
		has_flown = false;
		result2 = init_flash(&flash, ERASE);
		if (result2 == 0)	LED_OFF(DEBUG_LED_FLASH_GPIO_Port, DEBUG_LED_FLASH_Pin);
	}

	flight_state.flight_state = flight_phase;


	calibrateIMU(&imu_1, 1000, HWOFFSET);
//	calibrateIMU(&imu_1, 1, RESET_HWOFFSET);


	buzzerInit(&buzzer);
	servo_init(&servo);
	servo_rawmove(&servo, 2000);

	note_t notes[20] = {
			C1, G1, C2, E2,
			D1, A1, D2, F2,
			E1, B1, E2, G2,
			F1, C2, F2, A2,
			G1, D2, G2, B2
	};

//	for (size_t i = 0; i < 20; i++) {
//		LED_ON(Status_LED_GPIO_Port, Status_LED_Pin);
//		beepBuzzer(&buzzer, 250, 10, notes[i]);
//		LED_OFF(Status_LED_GPIO_Port, Status_LED_Pin);
//		HAL_Delay(75);
//	}
	
//  	HAL_Delay(1000);
//
//  	servo_moveto_deg(&servo, 90); // Muovi servo a 90°
//
//  	HAL_Delay(1000);
//
//  	servo_moveto_deg(&servo, 135); // Muovi servo a 135°
	
//	beepBuzzer(&buzzer, 500, 100, C5);
//	HAL_Delay(500);
//	beepBuzzer(&buzzer, 1000, 100, C5);
//	HAL_Delay(500);
//	beepBuzzer(&buzzer, 1000, 100, C5);
//	HAL_Delay(500);
//	beepBuzzer(&buzzer, 1000, 100, C5);
//	HAL_Delay(500);


//
//	uint8_t buf[32] = {0};
//
//	result = W25Q128_read_data(&flash, 32, buf, 32);
//
//	float_t temperature = 0;
//	memcpy(&temperature, buf + 24, sizeof(float_t));
//
//	float_t pressure = 0;
//	memcpy(&pressure, buf + 28, sizeof(float_t));

//  	HAL_Delay(1000);
//
//  	servo_moveto_deg(&servo, 90);
//
//  	HAL_Delay(1000);
//
//  	servo_moveto_deg(&servo, 0);


//	char *data = "internal flash writing test\0";
//
//	uint8_t Rx_Data[30] = {0};
//
//	int numofwords = (strlen(data)/4)+((strlen(data)%4)!=0);
//	Flash_Write_Data(0x08008100 , (uint32_t *)data, numofwords);
//	Flash_Read_Data(0x08008100 , Rx_Data, numofwords);












//  uint16_t diff_pressure = 0;
//
//  uint8_t result = init_pitot_sensor(&pitot, &hadc1);
//
//  for (int i = 0; i < 100; i++) {
//
//	  result = read_diff_pressure(&pitot);
//
//	  diff_pressure = pitot.diff_pressure > diff_pressure ? pitot.diff_pressure : diff_pressure;
//
//	  HAL_Delay(10);
//
//  }
//
//  uint16_t velocity = compute_velocity(temperature,pression,diff_pressure); /* measurement unit m/s */













  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
//  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
//  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
//  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
//  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of StartupTask */
//  StartupTaskHandle = osThreadNew(Startup, NULL, &StartupTask_attributes);

  /* creation of FlashWriteTask */
  FlashWriteTaskHandle = osThreadNew(FlashWrite, NULL, &FlashWriteTask_attributes);

  /* creation of SensorsReadTask */
  SensorsReadTaskHandle = osThreadNew(SensorsRead, NULL, &SensorsReadTask_attributes);

  /* creation of COMBoardTask */
//  COMBoardTaskHandle = osThreadNew(CommunicationBoard, NULL, &COMBoardTask_attributes);

  /* creation of FlightFSMTask */
  FlightFSMTaskHandle = osThreadNew(FlightFSM, NULL, &FlightFSMTask_attributes);

  /* creation of SystemHealthCheckTask */
//  SystemHealthCheckTaskHandle = osThreadNew(SystemHealthCheck, NULL, &SystemHealthCheckTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
//  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
//  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  rtU.Altitudeinput += 10.0;
	  rtU.Verticalvelocityinput = 200.0;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  codegen_model_step();

	  output[i++] = rtY.Airbrakesextoutput * 100;

	  servo_moveto_deg(&servo, rtY.Airbrakesextoutput * 60);

	  HAL_Delay(10);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) //ABBOT DEBUG
//  {
//    Error_Handler();
//  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
//  {
//    Error_Handler();
//  }
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
  sConfig.Channel = ADC_CHANNEL_11;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 0;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  husart1.Instance = USART1;
  husart1.Init.BaudRate = 115200;
  husart1.Init.WordLength = USART_WORDLENGTH_8B;
  husart1.Init.StopBits = USART_STOPBITS_1;
  husart1.Init.Parity = USART_PARITY_NONE;
  husart1.Init.Mode = USART_MODE_TX_RX;
  husart1.Init.CLKPolarity = USART_POLARITY_LOW;
  husart1.Init.CLKPhase = USART_PHASE_1EDGE;
  husart1.Init.CLKLastBit = USART_LASTBIT_DISABLE;
  if (HAL_USART_Init(&husart1) != HAL_OK)
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
  huart2.Init.BaudRate = 9600;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, E_Match_Parachute_1_Pin|E_Match_Parachute_2_Pin|IMU_1_nCS_Pin|BARO_1_nCS_Pin
                          |BARO_2_nCS_Pin|IMU_2_nCS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Channel1_PYRO_Pin|Channel2_PYRO_Pin|Status_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DEBUG_LED_FLASH_GPIO_Port, DEBUG_LED_FLASH_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : E_Match_Parachute_1_Pin E_Match_Parachute_2_Pin IMU_1_nCS_Pin BARO_1_nCS_Pin
                           BARO_2_nCS_Pin IMU_2_nCS_Pin */
  GPIO_InitStruct.Pin = E_Match_Parachute_1_Pin|E_Match_Parachute_2_Pin|IMU_1_nCS_Pin|BARO_1_nCS_Pin
                          |BARO_2_nCS_Pin|IMU_2_nCS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC15 PC0 PC2 PC3
                           PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB12 PB3 PB5
                           PB6 PB7 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_3|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Channel1_PYRO_Pin Channel2_PYRO_Pin Status_LED_Pin */
  GPIO_InitStruct.Pin = Channel1_PYRO_Pin|Channel2_PYRO_Pin|Status_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FLASH_nCS_Pin */
  GPIO_InitStruct.Pin = FLASH_nCS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLASH_nCS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DEBUG_LED_FLASH_Pin */
  GPIO_InitStruct.Pin = DEBUG_LED_FLASH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DEBUG_LED_FLASH_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_Startup */
/**
  * @brief  Function implementing the StartupTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Startup */
void Startup(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */

	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_FlashWrite */
/**
* @brief Function implementing the FlashWriteTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FlashWrite */
void FlashWrite(void *argument)
{
  /* USER CODE BEGIN FlashWrite */
	UBaseType_t uxHighWaterMark;
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	uint32_t tick;

	tick = osKernelGetTickCount();
  /* Infinite loop */
	for(;;)
	{
//		do {
//		if (num_meas_stored > (FLASH_NUMBER_OF_STORE_EACH_TIME - 1) && (flight_phase > READY)) {
		if (num_meas_stored_in_buffer > (FLASH_NUMBER_OF_STORE_EACH_TIME - 1))
		{
			/* this value 7 depends on the number of measurements are performed
			* between each time the flash task is invoked. In this case it is 7
			* because flash task has a period of 80 ticks while sensor task of
			* 10 ticks therefore 80/10 = 8 measurements, so > 7, means that when
			* 8 measurements are performed we can enter. In general the value will
			* be (flash_task_period / sensor_task_period) - 1
			*/

			size_t addr = (num_meas_stored_in_flash + num_meas_stored_in_buffer - FLASH_NUMBER_OF_STORE_EACH_TIME) * sizeof(sensor_data);

			flash_address[0] = addr;
			flash_address[1] = addr >> 8;
			flash_address[2] = addr >> 16;

			uint8_t *testiamo;
			testiamo = malloc(sizeof(sensor_data) * (FLASH_NUMBER_OF_STORE_EACH_TIME * 2));

			uint8_t result = W25Q128_write_data(&flash, flash_address, measurements_buffer, 264);
			uint8_t result2 = W25Q128_read_data(&flash,flash_address,testiamo,352);
	        sensor_data tesstttt;
			memcpy(&tesstttt,testiamo,sizeof(sensor_data));
	        sensor_data tesstttt2;
			memcpy(&tesstttt2,testiamo+sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt3;
			memcpy(&tesstttt3,testiamo+2*sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt4;
			memcpy(&tesstttt4,testiamo+3*sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt5;
			memcpy(&tesstttt5,testiamo+4*sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt6;
			memcpy(&tesstttt6,testiamo+5*sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt7;
			memcpy(&tesstttt7,testiamo+6*sizeof(sensor_data),sizeof(sensor_data));
	        sensor_data tesstttt8;
			memcpy(&tesstttt8,testiamo+7*sizeof(sensor_data),sizeof(sensor_data));


			if (result == 0) {
				num_meas_stored_in_buffer -= FLASH_NUMBER_OF_STORE_EACH_TIME;
				num_meas_stored_in_flash += FLASH_NUMBER_OF_STORE_EACH_TIME;
			}



		}
//		} while ((num_meas_stored > (FLASH_NUMBER_OF_STORE_EACH_TIME - 1)));

		tick += FLASH_WRITE_TASK_PERIOD;
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		osDelayUntil(tick);
	}
  /* USER CODE END FlashWrite */
}


float_t readAltitude(float_t seaLevelPa,float_t currentPa) {
  float_t altitude;

  altitude = 44330.0 * (1.0 - pow(currentPa / seaLevelPa, 0.1903));

  return altitude;
}

/* USER CODE BEGIN Header_SensorsRead */
/**
* @brief Function implementing the SensorsReadTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SensorsRead */
void SensorsRead(void *argument)
{
  /* USER CODE BEGIN SensorsRead */
	UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	uint32_t tick;

	tick = osKernelGetTickCount();
  /* Infinite loop */
	for(;;)
	{
		sensor_data data_1 = {0}, data_2 = {0};
		uint8_t result = 1;

		/* retrieving data from a couple of sensor and doing required conversions */
		result = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &barometer_data_1, &bmp390_1);
		result = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &barometer_data_2, &bmp390_2);

		result = lsm6dso32_angular_rate_raw_get(&imu_1, data_raw_angular_rate_1);
		result = lsm6dso32_acceleration_raw_get(&imu_1, data_raw_acceleration_1);
		result = lsm6dso32_angular_rate_raw_get(&imu_2, data_raw_angular_rate_2);
		result = lsm6dso32_acceleration_raw_get(&imu_2, data_raw_acceleration_2);

		angular_rate_mdps_1[0] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_1[0]);
		angular_rate_mdps_1[1] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_1[1]);
		angular_rate_mdps_1[2] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_1[2]);
		angular_rate_mdps_2[0] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_2[0]);
		angular_rate_mdps_2[1] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_2[1]);
		angular_rate_mdps_2[2] = lsm6dso32_from_fs2000_to_mdps(data_raw_angular_rate_2[2]);

		acceleration_mg_1[0] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_1[0]);
		acceleration_mg_1[1] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_1[1]);
		acceleration_mg_1[2] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_1[2]);
		acceleration_mg_2[0] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_2[0]);
		acceleration_mg_2[1] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_2[1]);
		acceleration_mg_2[2] = lsm6dso32_from_fs16_to_mg(data_raw_acceleration_2[2]);

		/* storing measurements to sensor_data variable */

		data_1.acc_x = acceleration_mg_1[0];
		data_1.acc_y = acceleration_mg_1[1];
		data_1.acc_z = acceleration_mg_1[2];
		data_1.dps_x = angular_rate_mdps_1[0];
		data_1.dps_y = angular_rate_mdps_1[1];
		data_1.dps_z = angular_rate_mdps_1[2];
		data_1.temperature = barometer_data_1.temperature;
		data_1.pressure = barometer_data_1.pressure;

		data_2.acc_x = acceleration_mg_2[0];
		data_2.acc_y = acceleration_mg_2[1];
		data_2.acc_z = acceleration_mg_2[2];
		data_2.dps_x = angular_rate_mdps_2[0];
		data_2.dps_y = angular_rate_mdps_2[1];
		data_2.dps_z = angular_rate_mdps_2[2];
		data_2.temperature = barometer_data_2.temperature;
		data_2.pressure = barometer_data_2.pressure;

		prev_acc.accX = curr_acc.accX;
		prev_acc.accY = curr_acc.accY;
		prev_acc.accZ = curr_acc.accZ;

		curr_acc.accX = acceleration_mg_1[0];
		curr_acc.accY = acceleration_mg_1[1];
		curr_acc.accZ = acceleration_mg_1[2];

		if(first_measure){
			Pressure_1 = data_1.pressure;
			Pressure_2 = data_2.pressure;
		}

		altitude = readAltitude(Pressure_1,data_1.pressure);
		data_1.altitude = altitude;
		altitude = readAltitude(Pressure_2,data_2.pressure);
		data_2.altitude = altitude;



		switch (flight_state.flight_state){

			case INVALID:
				data_1.invalid = 1;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 1;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case CALIBRATING:
				data_1.invalid = 0;
				data_1.calibrating = 1;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 1;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case READY:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 1;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 1;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case JELQING:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 1;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 1;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
			case BURNING:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 1;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 1;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case COASTING:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 1;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 1;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case DROGUE:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 1;
				data_1.main = 0;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 1;
				data_2.main = 0;
				data_2.touchdown = 0;
				break;
			case MAIN:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 1;
				data_1.touchdown = 0;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 1;
				data_2.touchdown = 0;
				break;
			case TOUCHDOWN:
				data_1.invalid = 0;
				data_1.calibrating = 0;
				data_1.ready = 0;
				data_1.jelqing = 0;
				data_1.burning = 0;
				data_1.coasting = 0;
				data_1.drogue = 0;
				data_1.main = 0;
				data_1.touchdown = 1;
				data_2.invalid = 0;
				data_2.calibrating = 0;
				data_2.ready = 0;
				data_2.jelqing = 0;
				data_2.burning = 0;
				data_2.coasting = 0;
				data_2.drogue = 0;
				data_2.main = 0;
				data_2.touchdown = 1;
				break;
		}


		/* computing offset of the buffer and storing the data to the buffer */

		size_t offset = num_meas_stored_in_buffer * sizeof(sensor_data);
//
		memcpy(measurements_buffer + offset, &data_2, sizeof(sensor_data));
        sensor_data buffer_data;
		memcpy(&buffer_data,measurements_buffer+offset,sizeof(sensor_data));




		/*
		 * increment of the number of stored measurements and
		 * modulo operation to avoid buffer overflow in the offset computation
		 */
		/*
		 * During READY phase data are not saved in the flash, therefore the data
		 * are always overwritten to the previous ones
		 */
		num_meas_stored_in_buffer++;
		num_meas_stored_in_buffer %= 16;



		tick += SENSORS_TASK_PERIOD;
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		osDelayUntil(tick);
	}
  /* USER CODE END SensorsRead */
}

/* USER CODE BEGIN Header_CommunicationBoard */
/**
* @brief Function implementing the COMBoardTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CommunicationBoard */
void CommunicationBoard(void *argument)
{
  /* USER CODE BEGIN CommunicationBoard */
	UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	uint32_t tick;

	tick = osKernelGetTickCount();
	  /* Infinite loop */
	for(;;)
	{


		tick += COMMUNICATION_BOARD_TASK_PERIOD;
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		osDelayUntil(tick);
	}
  /* USER CODE END CommunicationBoard */
}

/* USER CODE BEGIN Header_FlightFSM */
/**
* @brief Function implementing the FlightFSMTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_FlightFSM */
void FlightFSM(void *argument)
{
  /* USER CODE BEGIN FlightFSM */

	UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	uint32_t tick;

	tick = osKernelGetTickCount();
	/* Infinite loop */
	for(;;)
	{
		linear_acceleration_t acc1;
		acc1.accX = acceleration_mg_1[0];
		acc1.accY = acceleration_mg_1[1];
		acc1.accZ = acceleration_mg_1[2];

		linear_acceleration_t acc2;
		acc2.accX = angular_rate_mdps_1[0];
		acc2.accY = angular_rate_mdps_1[1];
		acc2.accZ = angular_rate_mdps_1[2];

		estimation_output_t MotionData;
		MotionData.acceleration = sqrt(acceleration_mg_1[0]*acceleration_mg_1[0] + acceleration_mg_1[1]*acceleration_mg_1[1] + acceleration_mg_1[2]*acceleration_mg_1[2]);
		MotionData.height = altitude;

		/* Check Flight Phases */
		    	check_flight_phase(&flight_state,MotionData, acc1, acc2);



		tick += FLIGHT_FSM_TASK_PERIOD;
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		osDelayUntil(tick);
	}
  /* USER CODE END FlightFSM */
}

/* USER CODE BEGIN Header_SystemHealthCheck */
/**
* @brief Function implementing the SystemHealthCheckTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SystemHealthCheck */
void SystemHealthCheck(void *argument)
{
  /* USER CODE BEGIN SystemHealthCheck */
	UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

	uint32_t tick;

	tick = osKernelGetTickCount();
	/* Infinite loop */
	for(;;)
	{


		tick += FLIGHT_HEALTH_CHECK_TASK_PERIOD;
		uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
		osDelayUntil(tick);
	}
  /* USER CODE END SystemHealthCheck */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

W25Q128_t* get_flash(){
	return &flash;
}

servo_t* get_servo(){
	return &servo;
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
