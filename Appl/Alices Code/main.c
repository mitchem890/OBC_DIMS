/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @file main.c
* @brief 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @author            Vasil Milev
* @version           1.0.0
* @date              2018.07.04
*
* @copyright         (C) Copyright Endurosat
*
*                    Contents and presentations are protected world-wide.
*                    Any kind of using, copying etc. is prohibited without prior permission.
*                    All rights - incl. industrial property rights - are reserved.
*
* @history
* @revision{         1.0.0  , 2018.07.04, author Vasil Milev, Initial revision }
* @endhistory
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INCLUDES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include "main.h"
#include "MCU_Init.h"
#include "fatfs.h"
#include "ESTTC.h"
#include "AppTasks.h"
#include "TaskMonitor.h"
#include  <errno.h>
#include  <sys/unistd.h>
#include  <stdarg.h>
#include  <EEPROM_emul.h>
#include  <S_Band_Trnsm.h>
#include  <X_Band_Trnsm.h>
#include <libADCS/ADCS.h>

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL DEFINES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define RST_NUMBER_MAX_COUNT    (10)    /* maximum number of reset that have to occur to consider the application invalid */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL TYPES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* No internal types definitions */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL VARIABLES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/**
* @brief RTC calendar format
*/
uint32_t calendar_format = RTC_FORMAT_BIN;
/**
* @brief Protoboard communication UART interface
*/
FILE *COMM = COM1;       //H1:33, 35

/**
* @brief System console USART interface
*/
FILE *SYSCON = COM4;     //H1:19, 20
/**
* @brief Payload USART interface
*/
FILE *PAYLOAD = COM6;   //USB connector     //H1:39, 40
/**
* @brief S-Band USART interface
*/
FILE *COM_SBAND = COM7;  //H2:11, 15
/**
* @brief FreeRTOS Default Task's handler
*/
osThreadId defaultTaskHandle;
/**
* @brief FreeRTOS Service Task's handler
*/
osThreadId ServiceTaskHandle;

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) VARIABLES DEFINITION/DECLARATION 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* No internal variable definitions */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) ROUTINES DECLARATION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* No internal routine definitions */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL (NONE STATIC) ROUTINES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief MAIN - First executed method after boot/startup - general initialisation, followed by running the RTOS kernel
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      none
* @param[output]     none
* @return            none
* @note              none
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int main(void)
{
  BootData = (boot_struct *)RTC_INIT_ADDRESS;    /* set boot data at the beginning of the RTC backup registers RTC_BKPxR */

  MX_IWDG_Init();
  SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk); // Enable All fault handlers

#ifdef NO_BOOTLOADER_ENABLED
__disable_interrupt();
  SCB->VTOR = BOOT_ADDRESS; // Redirect the Vector table to these of the Application, not to stay at these of the bootloader
__enable_interrupt();
#else
__disable_interrupt();
  SCB->VTOR = APPL_ADDRESS; // Redirect the Vector table to these of the Application, not to stay at these of the bootloader
__enable_interrupt();
#endif

#ifdef DEBUG_ENABLED
SCB->CCR |= SCB_CCR_DIV_0_TRP_Pos;     // Enable zero devision fault
#endif

  /* Initialize Eeprom emulation */
  EEPROM_Emul_Init();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_FMC_Init();
  //MX_I2C1_Init(); // I2C1 will be enabled only during use and disabled all the time while not used
  MX_I2C2_Init();
  MX_I2C3_Init();  // For XCAM
  MX_SPI1_Init();  // For XCAM
  MX_SPI2_Init();
  MX_SPI6_Init();
  MX_TIM5_Init();
  MX_USART1_Init();
  MX_USART4_Init();
  MX_USART6_Init();
  MX_RTC_Init();

  HAL_RTC_GetTime(&hrtc,&sTime,calendar_format);  // must be before GetDate
  HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
  fprintf(PAYLOAD,"\t MX_RTC_Init time is %04d-%02d-%02d %02d:%02d:%02d\r",
      sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
  sDate.Year    = 0x22; // in RTC_FORMAT_BCD 0x20 = 20 etc
  sDate.Month   = 0x01;
  sDate.Date    = 0x10;
  sTime.Hours   = 0x13;
  sTime.Minutes = 0x04;
  sTime.Seconds = 0x00;
  //HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BCD);
  //HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BCD);

  HAL_RTC_GetTime(&hrtc,&sTime,calendar_format);  // must be before GetDate
  HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
  fprintf(PAYLOAD,"\t NEW             time is %04d-%02d-%02d %02d:%02d:%02d\r",
      sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);

#ifdef ENABLE_S_BAND_TRANSMITTER
  S_BAND_TRNSM_Init();
#endif


#ifdef ENABLE_X_BAND_TRANSMITTER
  X_BAND_TRNSM_Init();
#endif
  
  ESTTC_PrintVersion(PAYLOAD);   /* Print SW version to the USB port */
  ESTTC_PrintVersion(COMM);      /* Print SW version to the Communication port */

  AMBER_LED_OFF();

  /* Create the thread(s) */
  /* definition and creation of defaultTask */

  osThreadDef(myStartDefaultTask, StartDefaultTask, osPriorityLow, 0, 6*128);
  defaultTaskHandle = osThreadCreate(osThread(myStartDefaultTask), NULL);
#if !defined(ENABLE_OBC_ADCS) || (defined(ENABLE_OBC_ADCS) && !ADCS_SIM_DEBUG)
  ESTTC_InitTask();
#endif
  TaskMonitor_Init();

  osThreadDef(myServicesTask, ServicesTask, osPriorityLow, 0, 6*128);
  ServiceTaskHandle = osThreadCreate(osThread(myServicesTask), NULL);


#if (defined(ENABLE_S_BAND_TRANSMITTER) || defined(ENABLE_X_BAND_TRANSMITTER))
  /* Create the task of that component */
  osThreadDef(myS_X_BAND_Task, S_X_BAND_Task, osPriorityLow, 1, 12*128);
  osThreadCreate(osThread(myS_X_BAND_Task), NULL);
#endif

#ifdef ENABLE_OBC_ADCS
  ADCS_Init (NULL);
#endif

  POWER_ON_UHF_1;   // Enable the power supply of the UHF transmitter.


  /* Start scheduler */
  osKernelStart();
  
  
  while (1)
  {

  }
}

/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief Period elapsed callback in non blocking mode
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      TIM_HandleTypeDef *htim - Timer handle
* @param[output]     none
* @return            none
* @note              This function is called  when TIM1 interrupt took place, inside
*                    HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
*                    a global variable "uwTick" used as application time base.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

#ifdef USE_FULL_ASSERT

/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief Reports the name of the source file and the source line number where the assert_param error has occurred.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      uint8_t* file - source file pointer, uint32_t line - source file line number
* @param[output]     none
* @return            none
* @note              !!! Stub method !!!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief This function is called when an error occurs. It executes a specific sequence of LED blips.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      none
* @param[output]     none
* @return            none
* @note              none
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
      /* Stay here until WatchDog Reset */
      GREEN_LED_ON();
      AMBER_LED_ON();
      HAL_Delay(2000);
      GREEN_LED_OFF();
      AMBER_LED_OFF();
      HAL_Delay(2000);
  }
  /* USER CODE END Error_Handler */
}

/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief This function is called in case of insufficient stack memory for a task. The stack must be increased at the point of creating the task.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      none
* @param[output]     none
* @return            none
* @note              none
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void vApplicationMallocFailedHook( void )
{
    Error_Handler();
}

/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief This function is executed in case of insufficient stack memory for the Operation system. The stack of the OS (configTOTAL_HEAP_SIZE) must be increased.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      none
* @param[output]     none
* @return            none
* @note              none
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    Error_Handler();
}

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) ROUTINES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
