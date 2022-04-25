/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @file AppTasks.c
* @brief Implementation of common task primitives
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
* @revision{         1.0.1  , 2020.01.16, author Georgi Georgiev, Moved everything, except StartDefaultTask() to DefTasks.c }
* @endhistory
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INCLUDES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include "AppTasks.h"
#include "fatfs.h"
#include "ESTTC.h"
#include "TaskMonitor.h"
#include "Sensors.h" //Mitch
#include "EPS.h" //mitch's
#include "XCAM.h" //Mitch's
#include "User_types.h"
#include "LIS3MDL_MAG_driver.h"
#include "DAT_Inputs.h"
#include "Panels.h"
#include "version.h"
#include "S_Band_Trnsm.h"
#include "X_Band_Trnsm.h"
#include "stm32f4xx_hal_i2c.h"  // alice added XCAM
#include "MCU_Init.h"           // alice added XCAM
#include  <Svc_RTC.h>

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL DEFINES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define APP_LED_ON_TIME             (50)     /* given time in ms */
#define APP_TASK_CALL_PERIOD        (1000)   /* given time in ms */

// These defines below are used for XCAM errors
// The actual values are completely ad hoc
#define AOK                     (1)
#define COMMAND_ACCEPTED        (1)
#define PARAMETER_UPLOADED      (1)
#define ACKNOWLEDGE_RECEIVED    (1)
#define PAYLOAD_ERROR_CONDITION (5)
#define INVALID_MODE_RETURNED   (6)
#define CRC_CHECK_FAILED        (7)
#define BAD_RETURN              (250)
#define UNSPECIFIED_ERROR       (255)
// These are XCAM registers
#define XCAM_SENSOR        (0)   // use 1, WFI at HAO
#define XCAM_INTTIME       (1)
#define XCAM_GRAB          (2)
#define XCAM_AUTOEXP       (3)
#define XCAM_HEALTH        (4)
#define XCAM_COMPRESS      (7)
#define XCAM_THUMB         (8)
#define XCAM_WINDOW       (11)   // 0x0B
#define XCAM_TIMEOUT      (18)   // 0x12
#define XCAM_INTTIMEFRAC  (19)   // 0x13
#define XCAM_OP_ERROR     (21)   // 0x15

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL TYPES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL VARIABLES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) VARIABLES DEFINITION/DECLARATION 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

uint16_t XCAM_i2cAddress     = 0x66;
XCAM_STATUS xstat;
int     blen;
uint8_t rx_buffer[260];
uint8_t ignore_tx[260];
uint8_t tx_buffer[30];
uint8_t spi_buffer[2600];
uint8_t  XCAM_status[260];
uint8_t  XCAM_partial_data[520]; // [260];  TODO
// interface_number = aps_number = 1 = WFI
uint16_t XCAM_sensor   =  1; // 1 == WFI
uint16_t XCAM_autoexp  =  0; // 1 == use auto exposure
uint16_t XCAM_timeout  = 30; // the time C3D goes back to standby
                             // after a grab command
uint16_t XCAM_inttime  =  200;
// defined in Middlewares/ESTTC.h
// extern volatile uint16_t XCAM_inttime_desired;
uint16_t XCAM_inttimefrac = 0;  // just an adhoc value for now
uint16_t XCAM_window   =  0;
uint16_t XCAM_compress =  0; // = 0 no compression, 1=compression
uint16_t XCAM_thumb    =  0; // = 0 no thumbnail  , 1=thumbnail
uint16_t XCAM_grab     =  1; // = 0 no grab, 1=grab an image
uint8_t  XCAM_mode     =  1; // 1=imaging mode
int      exposureIsDone = 1;


char     battery_text[100];

uint16_t addrLUP5V = 0x06;
uint16_t addrLUP3V = 0x05;

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) ROUTINES DECLARATION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL (NONE STATIC) ROUTINES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @brief That is a task used as example to configure all sensors and actuators and blinks the green LED.
* That can be changed freely as needed depending on the project.
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @param[input]      argument - not used
* @param[output]     none
* @return            none
* @note              none
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void StartDefaultTask(void const * argument)
{
	//Initialize Magnitometer Acc Sensors 1 and 2, Gyro 1, 2, 3
	Sensors_Init();
      
      Panels_Init();
      

	//Turn off magnetorquer
	Magnetorquer_Disable();


      //----------------------------sensors init end
      TaskMonitor_TaskInitialized(TASK_MONITOR_DEFAULT);   /* The task is initialized and is ready */

      // D_XCAM_Example();   // Damon's nice code

      /* XCAM */
      XCAM_inttime_desired = XCAM_inttime;
      int do_XCAM=1;
      int do_XCAM_Exposure=1;
      int do_XCAM_PowerOff=1;
      int do_XCAM_PowerOn =1;
      if (do_XCAM == 1 ) {
		Toggle_XCAM_Power(XCAM_PowerOff = do_XCAM_PowerOff, XCAM_PowerOn = do_XCAM_PowerOn);
       
        // ES+W1800000501 turns LUP3.3 off
        // ES+W1800000500 turns LUP3.3 on
        // ES+W1800000601 turns LUP5   off
        // ES+W1800000600 turns LUP5   on

        fprintf(PAYLOAD,"Starting XCAM_Init in AppTasks\r");
        uint8_t XCAM_error;
        XCAM_Init(&XCAM_error);
        fprintf(PAYLOAD,"         XCAM_Init completed in AppTasks\r");
        if (XCAM_error == AOK) {

          XCAM_WaitSeconds(2,0);

          if (do_XCAM_Exposure == 1 ) {
            fprintf(PAYLOAD,"\rStarting XCAM_Exposure in AppTasks\r");
            XCAM_Exposure();
            fprintf(PAYLOAD,"         XCAM_Exposure completed in AppTasks\r\r");
          }
        }
      }
      /* XCAM done */

      EPS_check(0,1);

      //uint8_t  myPanel = 0;
      //uint16_t myValue;
      //int pp;
      int pcounter = 0;
      int xcounter = 0;
      for( ; ; )
      {
          TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT); /* Prevent from WatchDog reset */

          /* blink the Green LED for 50ms to indicate the OBC is running */
          GREEN_LED_ON();
          osDelay(APP_LED_ON_TIME);
          GREEN_LED_OFF();

          // run a very simple exposure program
          // TODO a better exposure program should run in its own thread
          if((xcounter==0) && (exposureIsDone==1)){
             if ( XCAM_inttime  != XCAM_inttime_desired) {
                  XCAM_inttime = XCAM_inttime_desired;
                  XCAM_AdjustExposureTime();
              }
              // fprintf(PAYLOAD,"XCAM_Exposure running\r");
              // XCAM_Exposure();  TODO uncomment when all is working
              exposureIsDone=1;  // TODO remove when all is working
          }
          xcounter++;
          xcounter = xcounter%120;  // take an image every 120 seconds

          if(pcounter==0) {
              EPS_check(0,1);
          }
          pcounter++;
          //pcounter = pcounter%30;  // every 30 seconds
          pcounter = pcounter%5;  // every 5 seconds


          osDelay(APP_TASK_CALL_PERIOD);    /* Give processing time for the other tasks */

          /*
          if(pcounter>10) {
              pcounter = 0;
              myPanel++;
          }
          if(pcounter==0)
              fprintf(PAYLOAD,"\rChecking panel %d: ",myPanel);
          Pan_PD_ADC_Measure(myPanel, &myValue);
          fprintf(PAYLOAD,"--  %d --",myValue);
          pcounter++;
           */
          /*
          Panel_GetPhotodiodesLum();
          fprintf(PAYLOAD,"PanelLight: ");
          for(pp=0;pp<MAX_PAN;pp++){
              fprintf(PAYLOAD,"%d--",PanelLight[pp]);
          }
          fprintf(PAYLOAD,"\r");
          */


          // fprintf(PAYLOAD,"Hi in payload\r");
          // fprintf(COMM,"Hi in comm\r");
          // Use terminal.exe to see the "PAYLOAD" prints
          // terminal.exe settings are:
          // COM Port COM7
          // Baud rate is 115200
          // Data bits is 8
          // Parity is none
          // Stop bits is 1
          // Handshaking is none
          // Under special settings,
          // everything is unchecked
          // custom BR is 9600
          // Rx Clear is -1
          // Under Receive,
          // Reset Cnt is 13 and ASCII is checked
      }

}