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

#include "D_XCAM.h"
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

  //-------------------- sensors init start and print to both communication channels ------------------
  // That is just an example how to initialise all sensors and to print to two USART channels if needed

      if (Magnitometers_LIS3MDL_Init(LIS3MDL_MAG_I2C_ADDRESS_LOW) == E_OK)
      {
          fprintf(PAYLOAD,"MAG1_INIT_OK\r");
          fprintf(COMM  ,"MAG1_INIT_OK\r");
      }
      else{
          fprintf(PAYLOAD, "  MAG1 fail\r");
          fprintf(COMM  , "  MAG1 fail\r");
      }
      if (Magnitometers_LIS3MDL_Init(LIS3MDL_MAG_I2C_ADDRESS_HIGH) == E_OK)
      {
          fprintf(PAYLOAD,"MAG2_INIT_OK\r");
          fprintf(COMM  ,"MAG2_INIT_OK\r");
      }
      else{
          fprintf(PAYLOAD, "  MAG2 fail\r");
          fprintf(COMM  , "  MAG2 fail\r");
      }

      //Inizialize ACC Sensor 1
      if (AIS328DQ_Init(AIS328DQ_1_MEMS_I2C_ADDRESS) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"ACC1_INIT_OK\r");
        fprintf(COMM  ,"ACC1_INIT_OK\r");
      }
      else{
        I2C_Reset(&hi2c2);
      }

      //Inizialize ACC Sensor 2
      if (AIS328DQ_Init(AIS328DQ_2_MEMS_I2C_ADDRESS) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"ACC2_INIT_OK\r");
        fprintf(COMM  ,"ACC2_INIT_OK\r");
      }
      else{
        I2C_Reset(&hi2c2);
      }

      Panels_Init();

      /* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_X_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_1);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_1);
      }

      /* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_Y_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_2);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_2);
      }

      /* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_Z_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_3);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_3);
      }

      //Inizialize GYR Sensor X
      if (ADIS16265_Init(PAN_X_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_1);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_1);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_1);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_1);
      }

      //Inizialize GYR Sensor Y
      if (ADIS16265_Init(PAN_Y_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_2);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_2);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_2);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_2);
      }

      //Inizialize GYR Sensor Z
      if (ADIS16265_Init(PAN_Z_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_3);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_3);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_3);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_3);
      }

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
        // To turn off --> 5v then 3v 
        // To turn on  --> 3v then 5v
        if (do_XCAM_PowerOff == 1 ) {
          fprintf(PAYLOAD,"\r\tTurning off LUP in AppTasks\r");
          EPS_write(6,1);  //  --> turn off LUP 5v
          XCAM_WaitSeconds(1,0);
          EPS_write(5,1);  //  --> turn off LUP 3.3v
          // wait for everything to settle
          XCAM_WaitSeconds(30,1);
          EPS_check(0,1);
        }
        if (do_XCAM_PowerOn  == 1 ) {
          fprintf(PAYLOAD,"\r\tTurning on  LUP in AppTasks\r");
          EPS_write(5,0);  //  --> turn  on LUP 3.3v
          XCAM_WaitSeconds(5,1);
          EPS_write(6,0);  //  --> turn  on LUP 5v
          XCAM_WaitSeconds(5,1);
          EPS_check(0,1);
        }
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




/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INTERNAL (STATIC) ROUTINES DEFINITION
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


// 20201013
/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * XCAM code alice@ucar.edu 2020 09 17
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
void XCAM_Init(uint8_t *pError) {
    //int ii;
    HAL_StatusTypeDef HAL_val;

    unsigned long  ukube_time;
    uint8_t  time1, time2, time3, time4;
    uint16_t crc_raw;
    uint8_t  crc1, crc2;

    *pError = BAD_RETURN;;

    XCAM_zero_tx_rx ( &tx_buffer[0] , &rx_buffer[0] );


    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format); // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
    fprintf(PAYLOAD,"\tXCAM_Init time is %02d-%02d %02d:%02d:%02d\r",
            sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);

    ukube_time =   (long)sTime.Seconds
                 +((long)sTime.Minutes*60L)
                 +((long)sTime.Hours*3600L)
                 +((long)sDate.Date*24L*3600L)
                 +((long)sDate.Month*30L*24L*3600L);


    time1 = (uint8_t)((ukube_time      ) & 255);
    time2 = (uint8_t)((ukube_time >>  8) & 255);
    time3 = (uint8_t)((ukube_time >> 16) & 255);
    time4 = (uint8_t)((ukube_time >> 24) & 255);

    //fprintf(PAYLOAD,"XCAM_Init time4 time3 time2 time1: 0x%x 0x%x 0x%x 0x%x\r",
    //    time4,time3,time2,time1);

    tx_buffer[0] = 144; // 0x90
    tx_buffer[1] = 1;
    tx_buffer[5] = time4;
    tx_buffer[6] = time3;
    tx_buffer[7] = time2;
    tx_buffer[8] = time1;

    crc_raw = XCAM_crc16(65535, &tx_buffer[0] , 23);
    crc1    = (uint8_t)((crc_raw >> 8) & 255);
    crc2    = (uint8_t)( crc_raw       & 255);
    tx_buffer[23] = crc1;
    tx_buffer[24] = crc2;
    HAL_val = XCAM_transmit( &tx_buffer[0] );
    if (HAL_val != HAL_OK) return;
    osDelay(2); // in ms
    HAL_val = XCAM_receive ( &rx_buffer[0] );
    if (HAL_val != HAL_OK) return;



    fprintf(PAYLOAD,
            "\tactual  rx_buffer[0]=%d rx_buffer[1]=%d rx_buffer[2]=%d \r",
                rx_buffer[0],rx_buffer[1], rx_buffer[2]);
    fprintf(PAYLOAD,
            "\tdesired rx_buffer[0]=144 rx_buffer[1]=1 rx_buffer[2]=126\r");
    if(rx_buffer[0]==151) {
        uint8_t  XCAM_error;
        XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
        /*
        fprintf(PAYLOAD,"Sending back a 151 0x97\r");
        tx_buffer[0] = 151;  // 0x97
        tx_buffer[1] = 9;
        tx_buffer[2] = 1;
        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);
        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;
        XCAM_transmit(&tx_buffer[0]);
        */
    }
    //fprintf(PAYLOAD,"\tdesired that crc values = rx_buffer[3] and rx_buffer[4] \r");
    if ((rx_buffer[1] == 1) && (rx_buffer[0] == 144)) *pError = AOK;

}

void XCAM_Exposure(void) {
    fprintf(PAYLOAD,"\rInside XCAM_Exposure\r");
    uint16_t XCAM_health;
    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    uint32_t byte1, byte2, byte3, byte4, data_waiting;
    uint32_t data_count, bytes_read, payload_data_position;
    uint32_t bytes_written;
    char filenm[80];

    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format); // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);

    /* The FatFS filesystem needs short filenames (8.3 SFN)
     * so can't do anythiing fancy with the filenames until one
     * enables LFN  (LFN==longfilenames)
    sprintf(filenm,"0:/%04d%02d%02d_%02d%02d%02d_XCAM.raw",
      sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
     */
    /* One can use the filename structure below, but to avoid filling
     * up the SD card, for now, just make the files "0:/XCAM.raw"
    sprintf(filenm,"0:/%02d%02dX.raw",sTime.Hours,sTime.Minutes);
     */
    
    sprintf(filenm,"0:/XCAM.raw");

    exposureIsDone = 0;

    FIL  fid;
    UINT btw , bw;
    FRESULT fresult;
    bytes_written = 0;

    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
    if(XCAM_error != AOK) {
        fprintf(PAYLOAD,"XCAM_Exposure can't read status -- returning\r");
    }
    if((xstat.dataIsWaiting == 1) || (xstat.data_waiting > 0)) {
        fprintf(PAYLOAD,"XCAM_Exposure data is already waiting -- returning\r");
    }

    // XCAM_HEALTH
    // #define XCAM_HEALTH        (4)
    XCAM_payload_parameter_read  (XCAM_HEALTH,  &XCAM_health,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_health %d 0x%x\r",XCAM_health,XCAM_health);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_SENSOR
    // #define XCAM_SENSOR        (0)
    XCAM_payload_parameter_update(XCAM_SENSOR,   XCAM_sensor,  &XCAM_error);

    XCAM_WaitSeconds(1,0);

    // XCAM_TIMEOUT
    // #define XCAM_TIMEOUT      (18)
    XCAM_payload_parameter_update(XCAM_TIMEOUT,  XCAM_timeout, &XCAM_error);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);


    XCAM_payload_parameter_read  (XCAM_SENSOR,  &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_sensor %d = %d ? \r",XCAM_sensor,XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_WINDOW
    // #define XCAM_WINDOW       (11)
    XCAM_payload_parameter_update(XCAM_WINDOW ,  XCAM_window, &XCAM_error);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_INTTIME  integration time
    // #define XCAM_INTTIME       (1)
    XCAM_payload_parameter_update(XCAM_INTTIME,  XCAM_inttime, &XCAM_error);
    osDelay(2);
    XCAM_payload_parameter_read  (XCAM_INTTIME, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_inttime %d = %d ? \r",XCAM_inttime,XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_INITTIMEFRAC fractional part of the integration time
    // #define XCAM_INTTIMEFRAC  (19)
    XCAM_payload_parameter_update(XCAM_INTTIMEFRAC,  XCAM_inttimefrac, &XCAM_error);
    osDelay(2);
    XCAM_payload_parameter_read  (XCAM_INTTIMEFRAC, &XCAM_check,  &XCAM_error);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_AUTOEXP
    // #define XCAM_AUTOEXP       (3)
    XCAM_payload_parameter_update(XCAM_AUTOEXP,  XCAM_autoexp, &XCAM_error);
    osDelay(2);
    XCAM_payload_parameter_read  (XCAM_AUTOEXP, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_autoexp %d = %d ? \r",XCAM_autoexp,XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_COMPRESS
    // #define XCAM_COMPRESS      (7)
    XCAM_payload_parameter_update(XCAM_COMPRESS,  XCAM_compress, &XCAM_error);
    osDelay(2);
    XCAM_payload_parameter_read  (XCAM_COMPRESS, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_compress %d = %d ? \r",XCAM_compress,XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // XCAM_THUMB
    // #define XCAM_THUMB         (8)
    XCAM_payload_parameter_update(XCAM_THUMB,  XCAM_thumb, &XCAM_error);
    osDelay(2);
    XCAM_payload_parameter_read  (XCAM_THUMB, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_thumb %d = %d ? \r",XCAM_thumb,XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);


    // XCAM_mode = 1 (imaging)
    XCAM_payload_operations_update(XCAM_mode, &XCAM_error);
    if (XCAM_error != AOK ) {
        fprintf(PAYLOAD,"XCAM_payload_operations_update not AOK --> %d 0x%x\r",
                XCAM_error,XCAM_error);
        return;
    }


    // XCAM_GRAB
    // #define XCAM_GRAB  (2)
    fprintf(PAYLOAD,"xcam Initiating Grab command\r");
    XCAM_payload_parameter_update(XCAM_GRAB,  XCAM_grab, &XCAM_error);
    osDelay(100);
    XCAM_payload_parameter_read  (XCAM_GRAB, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_grab %d = %d ? \r",XCAM_grab,XCAM_check);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    // Usually need to initiate the grab command twice.
    if (XCAM_check != XCAM_grab) {
        fprintf(PAYLOAD,"xcam Initiating Grab command try2 \r");
        XCAM_payload_parameter_update(XCAM_GRAB,  XCAM_grab, &XCAM_error);
        osDelay(100);
        XCAM_payload_parameter_read  (XCAM_GRAB, &XCAM_check,  &XCAM_error);
        fprintf(PAYLOAD,"XCAM_grab %d = %d ? \r",XCAM_grab,XCAM_check);
        TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    }
    if (XCAM_check != XCAM_grab) return;

    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);

    // Wait 5 seconds for the grab execute.
    // TODO it might be possible to make this shorter.
    XCAM_WaitSeconds(5,1);

    // Start waiting for XCAM to indicate that there is data
    // on the SPI interface.
    fprintf(PAYLOAD,"\r\rStarting the waiting for SPI data ready status\r");
    int p = 0;
    while (1) {
        XCAM_WaitSeconds(1,1);
        fprintf(PAYLOAD,"Waiting for SPI data ready status -- %d\r",p);
        p++;
        XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
        if ( (p==60)                   // timeout
            || ((XCAM_status[2] & 2) ==  2) // 0x02 --> operation finished
            ||  (XCAM_status[2]      ==  6) // 0x04 + 0x02
            ||  (XCAM_status[2]      == 18) // 0x10(op error) + 0x02
            )
        {
            break;
        }
    }
    fprintf(PAYLOAD,"SPI data ready status -- XCAM_status[2] = 0x%x\r",XCAM_status[2]);

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);


    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);

    // obtain the number of data packets waiting
    // from XCAM_status bytes 5-8
    byte4 = (((uint32_t)XCAM_status[5]) <<24);
    byte3 = (((uint32_t)XCAM_status[6]) <<16);
    byte2 = (((uint32_t)XCAM_status[7]) << 8);
    byte1 = (((uint32_t)XCAM_status[8])     );
    fprintf(PAYLOAD,"byte4 byte3 byte2 byte1: %ld %ld %ld %ld\r",
        byte4,byte3,byte2,byte1);
    data_waiting = (byte4 | byte3) | (byte2 | byte1);
    fprintf(PAYLOAD,"Number of payload data packets to transfer: %ld\r",
        data_waiting);

    if ( data_waiting != 0 ) {

        // double check for an error
        if (XCAM_status[2] == 146) {
            XCAM_payload_parameter_read  (XCAM_OP_ERROR, &XCAM_check,  &XCAM_error);
            fprintf(PAYLOAD,"XCAM_op_err 0x%x\r",XCAM_check);
        }


        XCAM_WaitSeconds(1,0);
        int writeToFile = 1;
        fresult = 55;  // just some value
        if (writeToFile== 1) {
            fresult = f_open(&fid, filenm,
                         FA_WRITE|FA_READ|FA_OPEN_ALWAYS|FA_OPEN_EXISTING);
        }
        if( fresult == FR_OK ) {
            fprintf(PAYLOAD,"\t**************** %s open success\r", filenm);

            payload_data_position = 6;
            bytes_read            = 0;
            data_count            = data_waiting;
            uint32_t nn=0;
            while ( nn <= data_count) { // Note! WARNING TODO
                                        // This could turn into an infinte loop
                                        // if the "nn" returned by 
                                        // XCAM_payload_data_transfer are
                                        // not incrementing because the
                                        // data packet is not good.

                TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
                if((nn % 1000) == 0 )
                    fprintf(PAYLOAD,"data count so far %ld\r",nn);

                XCAM_payload_data_transfer( &XCAM_partial_data[0], &XCAM_error, &nn);

                /* Without any error checking, write any data returned
                 * to disk.
                 * This is a debug effort to be able to look at whatever
                 * was transferred via SPI.
                 *
                 * Note, since the entire 260 byte packet is returned,
                 * one could verify that the 0th byte = 0x97
                 * and if it does, then write to disk.
                 * i.e.
                 * if ( XCAM_partial_data[0] == 0x97 ) {
                 *     btw = 260;
                 *     bw  =   0;
                 *     if (writeToFile==1) {
                 *         f_write(&fid,(const void *)(&XCAM_partial_data[0]),
                 *                 btw , &bw);
                 *     }
                 *     if (bw != btw) {
                 *         fprintf(PAYLOAD,
                 *             "%s only wrote %d bytes out of %d\r",filenm,
                 *             bw, btw);
                 *     }
                 * }
                 */
                btw = 260; // bytes to write
                bw  =   0; // bytes written -- will be filled by f_write
                if (writeToFile==1) {
                    f_write(&fid,(const void *)(&XCAM_partial_data[0]),
                            btw , &bw);
                }
                if (bw != btw) {
                    fprintf(PAYLOAD,
                        "%s only wrote %d bytes out of %d\r",filenm,
                        bw, btw);
                }
                bytes_written += bw;
            }
            if (writeToFile==1) {
                fprintf(PAYLOAD,"\t**************** %s closed %ld bytes written\r",
                    filenm,bytes_written);
                f_close(&fid);
            }
        }
        else {
            fprintf(PAYLOAD,"**************** %s could not be opened!\r",filenm);
        }
    }

    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);


    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    exposureIsDone = 1;

} // XCAM_Exposure end

void XCAM_AdjustExposureTime()
{
    uint16_t XCAM_check;
    uint8_t  XCAM_error;
    XCAM_payload_parameter_update(XCAM_INTTIME,  XCAM_inttime, &XCAM_error);
    osDelay(100);
    XCAM_payload_parameter_read  (XCAM_INTTIME, &XCAM_check,  &XCAM_error);
    fprintf(PAYLOAD,"XCAM_inttime %d = %d ? \r",XCAM_inttime,XCAM_check);
}

// Type 2 command
void XCAM_payload_parameter_read(uint8_t   parameter_id, 
                                 uint16_t *pparameter_value,
                                 uint8_t  *pError)
{
    HAL_StatusTypeDef HAL_val;
    *pparameter_value = 0;
    *pError           = BAD_RETURN;

    uint8_t crc1, crc2;

    XCAM_zero_tx_rx ( &tx_buffer[0] , &rx_buffer[0] );

    tx_buffer[0] = 148;  // 0x94
    tx_buffer[1] = 1;
    tx_buffer[2] = parameter_id;

    XCAM_get_crc1andcrc2(&tx_buffer[0], 3, &crc1 , &crc2);

    tx_buffer[3] = crc1;
    tx_buffer[4] = crc2;

    HAL_val = XCAM_transmit_receive( &tx_buffer[0] , &rx_buffer[0]);
    if (HAL_val != HAL_OK) return;

    fprintf(PAYLOAD,"XCAM_payload_parameter_read ");
    if (rx_buffer[1] == 9) {

        XCAM_get_crc1andcrc2(&rx_buffer[0],3,&crc1,&crc2);

        if((crc1 == rx_buffer[3]) && (crc2 == rx_buffer[4])){
            if( rx_buffer[0] == 148 ) {
                fprintf(PAYLOAD,"error9: rx_buffer[1] %d 0x%x\r",
                        rx_buffer[1], rx_buffer[1]);
                fprintf(PAYLOAD,"error9: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                    rx_buffer[2],
                    rx_buffer[2]);
            }
            else{
                fprintf(PAYLOAD,"error9: INVALID_MODE_RETURNED\r");
            }
        }
        else {
            fprintf(PAYLOAD,"error9: CRC_CHECK_FAILED\r");
        }
    }
    else{
        XCAM_get_crc1andcrc2(&rx_buffer[0],5,&crc1,&crc2);

        if((crc1 == rx_buffer[5]) && (crc2 == rx_buffer[6])){
            if(rx_buffer[0] == 148) {
                if(rx_buffer[1] == 1) {
                    *pError = ACKNOWLEDGE_RECEIVED;
                }
                else{
                    fprintf(PAYLOAD,"error: rx_buffer[1] %d 0x%x\r",
                            rx_buffer[1], rx_buffer[1]);
                    fprintf(PAYLOAD,"error: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                        rx_buffer[2],
                        rx_buffer[2]);
                }
            }
            else {
                fprintf(PAYLOAD,"error: INVALID_MODE_RETURNED\r");
            }
        }
        else {
            fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
        }
    }

    if ( *pError ==  ACKNOWLEDGE_RECEIVED) {
        uint16_t msbyte = (rx_buffer[3] << 8) & 65535;
        uint16_t lsbyte =  rx_buffer[4]       & 65535;
        *pparameter_value = msbyte | lsbyte;

        tx_buffer[0] = 148;
        tx_buffer[1] = 1;
        tx_buffer[2] = 126; // 0x7e Acknowledge

        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;

        XCAM_transmit(&tx_buffer[0]); // handshake an Acknowledge

        fprintf(PAYLOAD,"good return: ACKNOWLEDGE_RECEIVED\r");
    }

    return;
}


void XCAM_payload_parameter_update(uint8_t  parameter_id,
                                   uint16_t parameter_value,
                                   uint8_t  *pError)
{
    HAL_StatusTypeDef HAL_val;
    *pError           = BAD_RETURN;

    uint8_t  crc1, crc2;
    uint8_t  lsbyte, msbyte;

    XCAM_zero_tx_rx ( &tx_buffer[0] , &rx_buffer[0] );

    tx_buffer[0] = 147; // 0x93
    tx_buffer[1] = 1;
    tx_buffer[2] = parameter_id;

    msbyte = (uint8_t)((parameter_value >> 8) & 255);
    lsbyte = (uint8_t)( parameter_value       & 255);

    tx_buffer[3] = msbyte;
    tx_buffer[4] = lsbyte;

    XCAM_get_crc1andcrc2(&tx_buffer[0],5,&crc1,&crc2);

    tx_buffer[5] = crc1;
    tx_buffer[6] = crc2;

    HAL_val = XCAM_transmit_receive( &tx_buffer[0] , &rx_buffer[0]);
    if (HAL_val != HAL_OK) return;

    XCAM_get_crc1andcrc2(&rx_buffer[0],3,&crc1,&crc2);

    fprintf(PAYLOAD,"XCAM_payload_parameter_update ");
    if((crc1 == rx_buffer[3]) && (crc2 == rx_buffer[4])){
        if( rx_buffer[0] == 147 ) {
            if (rx_buffer[1] == 1 ) {
                *pError = PARAMETER_UPLOADED;
                fprintf(PAYLOAD,"GOOD: PARAMETER_UPLOADED\r");
            }
            else {
                fprintf(PAYLOAD,"error1: rx_buffer[1] %d 0x%x\r",
                        rx_buffer[1], rx_buffer[1]);
                fprintf(PAYLOAD,"error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                    rx_buffer[2],
                    rx_buffer[2]);
            }
        }
        else {
             fprintf(PAYLOAD,"error1: INVALID_MODE_RETURNED\r");
        }
    }
    else {
        fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
    }

    return;
}



// This handles Type 1 commands.
void XCAM_payload_operations_update(uint8_t operation_mode, uint8_t  *pError)
{
    *pError           = BAD_RETURN;


    HAL_StatusTypeDef HAL_val;
    unsigned long  ukube_time;
    uint8_t  time1, time2, time3, time4;
    uint8_t  crc1, crc2;

    XCAM_zero_tx_rx ( &tx_buffer[0] , &rx_buffer[0] );

    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format);  // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);

    ukube_time =   (long)sTime.Seconds
                 +((long)sTime.Minutes*60L)
                 +((long)sTime.Hours*3600L)
                 +((long)sDate.Date*24L*3600L)
                 +((long)sDate.Month*30L*24L*3600L);


    time1 = (uint8_t)((ukube_time      ) & 255);
    time2 = (uint8_t)((ukube_time >>  8) & 255);
    time3 = (uint8_t)((ukube_time >> 16) & 255);
    time4 = (uint8_t)((ukube_time >> 24) & 255);
    fprintf(PAYLOAD,"XCAM_payload_operations_update time4 time3 time2 time1: 0x%x 0x%x 0x%x 0x%x\r",
        time4,time3,time2,time1);

    tx_buffer[0] = 146; // 0x92
    tx_buffer[1] = 1;
    tx_buffer[2] = operation_mode;
    tx_buffer[5] = time4;
    tx_buffer[6] = time3;
    tx_buffer[7] = time2;
    tx_buffer[8] = time1;

    XCAM_get_crc1andcrc2(&tx_buffer[0],23,&crc1,&crc2);
    tx_buffer[23] = crc1;
    tx_buffer[24] = crc2;
    HAL_val = XCAM_transmit_receive( &tx_buffer[0] , &rx_buffer[0]);
    if (HAL_val != HAL_OK) return;

    XCAM_get_crc1andcrc2(&rx_buffer[0],3,&crc1,&crc2);

    fprintf(PAYLOAD,"XCAM_payload_operations_update ");
    if((crc1 == rx_buffer[3]) && (crc2 == rx_buffer[4])){
        if( rx_buffer[0] == 146 ) {
            if (rx_buffer[1] == 1 ) {
                *pError = COMMAND_ACCEPTED;
                fprintf(PAYLOAD,"GOOD: COMMAND_ACCEPTED\r");
            }
            else {
                fprintf(PAYLOAD,"error1: rx_buffer[1] %d 0x%x\r",
                        rx_buffer[1], rx_buffer[1]);
                fprintf(PAYLOAD,"error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                        rx_buffer[2],
                        rx_buffer[2]);
            }
        }
        else {
             fprintf(PAYLOAD,"error1: INVALID_MODE_RETURNED\r");
        }
    }
    else {
        fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
    }

    return;
}


// This is a Type 2 command
void XCAM_payload_status_update(uint8_t *pstatus, uint8_t *pError)
{
    HAL_StatusTypeDef HAL_val;
    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    *pError           = BAD_RETURN;
    uint32_t byte1, byte2, data_waiting, payload_flags;
    uint8_t *pstatus_in;
    pstatus_in = pstatus;

    int ii;
    uint8_t  crc1, crc2;

    fprintf(PAYLOAD,"XCAM_payload_status_update ");

    XCAM_zero_tx_rx ( &tx_buffer[0] , &rx_buffer[0] );

    tx_buffer[0] = 145; // 0x91
    tx_buffer[1] = 1;

    XCAM_get_crc1andcrc2(&tx_buffer[0],2,&crc1,&crc2);

    tx_buffer[2] = crc1;
    tx_buffer[3] = crc2;

    HAL_val = XCAM_transmit_receive( &tx_buffer[0] , &rx_buffer[0]);
    if (HAL_val != HAL_OK) return;

    XCAM_get_crc1andcrc2(&rx_buffer[0],20,&crc1,&crc2);

    if((crc1 == rx_buffer[20]) && (crc2 == rx_buffer[21])){
        if( rx_buffer[0] == 145 ) {
            if (rx_buffer[1] == 1 ) {
                *pError = ACKNOWLEDGE_RECEIVED;
                // copy the receive buffer to pstatus
                // status = rx_buffer[2:19]
                uint8_t *prx;
                prx = &rx_buffer[2];
                for (ii=2;ii<20;ii++) { *pstatus++ = *prx++; }
            }
        }
        else {
             fprintf(PAYLOAD,"error1: INVALID_MODE_RETURNED\r");
        }
    }
    else {
        fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
    }

    if ( *pError ==  ACKNOWLEDGE_RECEIVED) {

        tx_buffer[0] = 145;
        tx_buffer[1] = 1;
        tx_buffer[2] = 126; // 0x7e Acknowledge

        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;

        XCAM_transmit(&tx_buffer[0]); // handshake an Acknowledge

        TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
        fprintf(PAYLOAD,"GOOD: ACKNOWLEDGE_RECEIVED\r");
        pstatus = pstatus_in;

        byte2 = (((uint32_t)*(pstatus+1)) << 8);
        byte1 = (((uint32_t)*(pstatus+2))     );
        payload_flags = byte2 | byte1;
        data_waiting  = ((uint32_t)*(pstatus+5) <<24) |
                        ((uint32_t)*(pstatus+6) <<16) |
                        ((uint32_t)*(pstatus+7) << 8) |
                        ((uint32_t)*(pstatus+8)     );

        xstat.opMode            = *(pstatus+0);
        xstat.payIsBusy         = (payload_flags &  1)/1;
        xstat.opIsFinished      = (payload_flags &  2)/2;
        xstat.dataIsWaiting     = (payload_flags &  8)/8;
        xstat.opErrorExists     = (payload_flags & 16)/16;
        xstat.opModeErrorExists = (payload_flags & 32)/32;

        XCAM_payload_parameter_read (XCAM_OP_ERROR, &XCAM_check,  &XCAM_error);
        xstat.opErrorOutOfMemory   = (XCAM_check==1) ? 1: 0;
        xstat.opErrorAutoExpProb   = (XCAM_check==2) ? 1: 0;
        xstat.opErrorGrabTimeout   = (XCAM_check==3) ? 1: 0;
        xstat.opErrorCISSetupProb  = (XCAM_check==5) ? 1: 0;
        xstat.opErrorCISGrabProb   = (XCAM_check==6) ? 1: 0;
        xstat.opErrorBadParamCombo = (XCAM_check==7) ? 1: 0;

        xstat.data_waiting = data_waiting;

        /*
        fprintf(PAYLOAD,"XCAM_status bytes\r");
        for (ii=0;ii<20;ii++){
            fprintf(PAYLOAD," 0x%02x ", *(pstatus+ii));
        }
        fprintf(PAYLOAD,"\r");
        */
        fprintf(PAYLOAD,"\tXCAM status op_mode           is %d -- 0==wakeUp   1==imaging\r",xstat.opMode);
        fprintf(PAYLOAD,"\tXCAM status payIsBusy         is %d -- 1==True\r",xstat.payIsBusy);
        fprintf(PAYLOAD,"\tXCAM status opIsFinished      is %d -- 1==True\r",xstat.opIsFinished);
        fprintf(PAYLOAD,"\tXCAM status dataIsWaiting     is %d -- 1==True\r",xstat.dataIsWaiting);
        fprintf(PAYLOAD,"\tXCAM status opErrorExists     is %d -- 1==True\r",xstat.opErrorExists);
        fprintf(PAYLOAD,"\tXCAM status opModeErrorExists is %d -- 1==True\r",xstat.opModeErrorExists);
        if (xstat.opErrorExists == 1) {
            if(xstat.opErrorOutOfMemory==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 1 -> insufficient memory\r");
            if(xstat.opErrorAutoExpProb==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 2 -> auto exp failed\r");
            if(xstat.opErrorGrabTimeout==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 3 -> grab timeout\r");
            if(xstat.opErrorCISSetupProb==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 5 -> CIS setup failure\r");
            if(xstat.opErrorCISGrabProb==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 6 -> CIS grab failure\r");
            if(xstat.opErrorBadParamCombo==1)
                fprintf(PAYLOAD,"\tXCAM_op_err of 7 -> Invalid parameter combination\r");
        }
        if ((payload_flags &  1 ) ==  1) {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- payload is busy \r");
        }
        else {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- payload is NOT busy \r");
        }
        if ((payload_flags &  2 ) ==  2) {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- payload operation finished\r");
        }
        else {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- payload operation NOT finished \r");
        }
        if ((payload_flags &  8 ) ==  8) {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- no data packets waiting\r");
        }
        else {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- data packets ARE waiting -> %ld packets\r",
                             data_waiting);
        }
        if ((payload_flags & 16 ) == 16) {  // 16 = 0x10
            fprintf(PAYLOAD,"\tXCAM status op_flags --- operation ERROR\r");
            // read the error code
            XCAM_payload_parameter_read  (XCAM_OP_ERROR, &XCAM_check,  &XCAM_error);
            fprintf(PAYLOAD,"\tXCAM_op_err 0x%x\r",XCAM_check);
            fprintf(PAYLOAD,"\tXCAM_op_err of 1 -> insufficient memory\r");
            fprintf(PAYLOAD,"\tXCAM_op_err of 2 -> auto exp failed\r");
            fprintf(PAYLOAD,"\tXCAM_op_err of 3 -> grab timeout\r");
            fprintf(PAYLOAD,"\tXCAM_op_err of 5 -> CIS setup failure\r");
            fprintf(PAYLOAD,"\tXCAM_op_err of 6 -> CIS grab failure\r");
            fprintf(PAYLOAD,"\tXCAM_op_err of 7 -> Invalid parameter combination\r");
        }
        else {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- operation NO error found\r");
        }
        if ((*(pstatus+2) & 32 ) == 32) {  // 32 = 0x20 invalid mode
            fprintf(PAYLOAD,"\tXCAM status op_flags -- invalid mode\r");
        }
        else {
            fprintf(PAYLOAD,"\tXCAM status op_flags --- NO invalid mode\r");
        }
    }
    else {
        tx_buffer[0] = 145;
        tx_buffer[1] = 9;
        tx_buffer[2] = 1;

        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;

        XCAM_transmit(&tx_buffer[0]);

        fprintf(PAYLOAD,"status error1: rx_buffer[1] %d 0x%x\r",
                rx_buffer[1], rx_buffer[1]);
        fprintf(PAYLOAD,"status error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
            rx_buffer[2],
            rx_buffer[2]);
    }

    return;
}



// pdata is 260 bytes long, the entire packet is desired
void XCAM_payload_data_transfer(uint8_t *ppayload_data, uint8_t *pError, uint32_t *whichOne)
{
    *pError           = BAD_RETURN;

    int ii;
    uint8_t  crc1, crc2;
    HAL_StatusTypeDef SPI_retStat;

    uint8_t *ppay;

    /* Tell XCAM that the data will be xferred via SPI
     * if I2C were used payloadMode = 150   0x96
     */
    uint8_t payloadMode = 151; // SPI 0x97
    tx_buffer[0] = payloadMode;
    tx_buffer[1] = 1;
    tx_buffer[2] = 0;  // data in units of 256B at 1Mbps

    XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

    tx_buffer[3] = crc1;
    tx_buffer[4] = crc2;

    XCAM_transmit(&tx_buffer[0]);

    osDelay(2);

    // The original python code from XCAM had:
    // transmit(commsLink, tx_buffer,'uint8')
    // time.sleep(0.002)
    // ignore_tx = array.array('B',np.zeros((260,1))
    // byteCount, spi_buffer 
    //     = aardvark_py.aa_spi_write(
    //         commsLink['aardvarkHandle'], ignore_tx, spi_buffer)
    //

    uint32_t timeout = 250; // TODO experiment with shorter and longer times
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // chip select active
    osDelay(1);  // 1 seems to work ok
    SPI_retStat = HAL_SPI_TransmitReceive(&hspi1, 
                                          &ignore_tx[0], &spi_buffer[0],
                                          260, timeout);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // chip select inactive


    /* Have to send the handshake back within 2ms, so delay all debug
     * print statements until much further down.
     */

 
    /* expect spi_buffer[0] = 0x97 (151)
              spi_buffer[1] = 0x01 
       if     spi_buffer[1] = 0x09 there is an error
     */

    if (spi_buffer[1] == 9) { // An error was reported
        XCAM_get_crc1andcrc2(&spi_buffer[0],3,&crc1,&crc2);
    }
    if(spi_buffer[1] == 1) {
        XCAM_get_crc1andcrc2(&spi_buffer[0],258,&crc1,&crc2);
        // If the calculated crc's match the crc's in the buffer,
        // the buffer is good
        if((crc1 == spi_buffer[258]) && (crc2 == spi_buffer[259])) {
            if(spi_buffer[0] == payloadMode) {
                *pError = ACKNOWLEDGE_RECEIVED;
            }
        }
    }

    if (*pError == ACKNOWLEDGE_RECEIVED) {

        tx_buffer[0] = payloadMode;
        tx_buffer[1] = 1;
        tx_buffer[2] = 126; // 0x7e Acknowledge

        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;

        XCAM_transmit(&tx_buffer[0]);
    }
    else {

        tx_buffer[0] = payloadMode;
        tx_buffer[1] = 9;
        tx_buffer[2] = 1;

        XCAM_get_crc1andcrc2(&tx_buffer[0],3,&crc1,&crc2);

        tx_buffer[3] = crc1;
        tx_buffer[4] = crc2;

        XCAM_transmit(&tx_buffer[0]);

    }

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);





    /* Now it is safe to print out debug messages */

    //  if( SPI_retStat != HAL_OK )
    //  if( SPI_retStat == HAL_ERROR )
    //  if( SPI_retStat == HAL_TIMEOUT)
    //  if( SPI_retStat == HAL_BUSY)
    //  &&(hi2c3.State == HAL_SPI_STATE_READY)))
    if ( SPI_retStat == HAL_OK) {
        if(*whichOne == 0 ) { // debug info for a specific buffer
            fprintf(PAYLOAD,
                "XCAM HAL_SPI_TransmitReceive return was     HAL_OK\r");
        }
    }
    else {
        fprintf(PAYLOAD,"XCAM HAL_SPI_TransmitReceive return was NOT HAL_OK\r");
    }

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    if (spi_buffer[1] == 9) { // An error was reported
        XCAM_get_crc1andcrc2(&spi_buffer[0],3,&crc1,&crc2);
        if ((crc1 == spi_buffer[3]) && (crc2 == spi_buffer[4])) {
            if(spi_buffer[0] == payloadMode) {
                fprintf(PAYLOAD,"error9: SPI spi_buffer[1] %d 0x%x\r",
                        spi_buffer[1], spi_buffer[1]);
                fprintf(PAYLOAD,"error9: SPI PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                    spi_buffer[2],
                    spi_buffer[2]);
            }
            else {
                fprintf(PAYLOAD,"error9: SPI INVALID_MODE_RETURNED\r");
            }
        }
        else {
            fprintf(PAYLOAD,"error9: SPI CRC_CHECK_FAILED\r");
        }
    }
    else { // error not flagged in this buffer
        XCAM_get_crc1andcrc2(&spi_buffer[0],258,&crc1,&crc2);
        // If the calculated crc's match the crc's in the buffer,
        // the buffer is good
        if((crc1 == spi_buffer[258]) && (crc2 == spi_buffer[259])) {
            if(spi_buffer[0] == payloadMode) {
                if(spi_buffer[1] == 1) {
                    /*
                    fprintf(PAYLOAD,"SPI looks good spi_buffer[0] 0x%x spi_buffer[1] 0x%x\r",
                            spi_buffer[0], spi_buffer[1]);
                    */
                }
                else {
                    fprintf(PAYLOAD,"error: SPI spi_buffer[0] %d 0x%x\r",
                            spi_buffer[0], spi_buffer[0]);
                    fprintf(PAYLOAD,"error: SPI spi_buffer[1] %d 0x%x\r",
                            spi_buffer[1], spi_buffer[1]);
                    fprintf(PAYLOAD,"error: SPI PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                        spi_buffer[2],
                        spi_buffer[2]);
                }
            }
            else {
                fprintf(PAYLOAD,"error: SPI INVALID_MODE_RETURNED\r");
            }
        }
        else {
            if(*whichOne == 0 ) {
            fprintf(PAYLOAD,"error: SPI CRC_CHECK_FAILED\r");
            }
        }
    }

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    if(*whichOne < 5) { // debug info for a specific buffers
        blen = 260;
        blen =   1;
        int jj;
        int kk;
        int stepsz;
        fprintf(PAYLOAD,"\tXCAM_payload_data_transfer SPI spi_buffer[0] [1]\r should be 151 1 or hex 0x97 0x01\r");
        fprintf(PAYLOAD,"\tXCAM_payload_data_transfer SPI spi_buffer is:\r");

        stepsz=26;
        for (ii=0;ii<blen;ii+=stepsz){
            fprintf(PAYLOAD,"%03d     ",ii);
            for(jj=0;jj<stepsz;jj++) {
                kk = ii + jj;
                fprintf(PAYLOAD," 0x%02x ",spi_buffer[kk]);
            }
            fprintf(PAYLOAD,"\r");
        }
    }

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    if((*whichOne == 0 ) || (*whichOne == 1) || (*whichOne%1000 == 0)) {
            fprintf(PAYLOAD,"SPI CRC in spi_buffer: 0x%02x 0x%02x\r",
                spi_buffer[258],spi_buffer[259]);
            fprintf(PAYLOAD,"SPI CRC    calculated: 0x%02x 0x%02x\r",
                        crc1,crc2);
    }

    /* Copy the data regardless whether it is any good
     * Note that you may want to change this and only
     * copy packets that have 0x97 0x01 and good crc's
     */

    // Copy the entire 260 byte packet
    uint8_t *prx;
    prx  = &spi_buffer[0];
    ppay = ppayload_data;
    for(ii=0;ii<260;ii++) {
        *ppay++ = *prx++;
    }
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    if (*pError == ACKNOWLEDGE_RECEIVED) {
        *whichOne += 1;
    }
    
}









void XCAM_zero_tx_rx( uint8_t *ptx , uint8_t *prx)
{
    int ii;
    for (ii=0;ii< 30;ii++) { *ptx++ = 0; }
    for (ii=0;ii<260;ii++) { *prx++ = 0; }
    return;
}
void XCAM_get_crc1andcrc2( uint8_t *pBuf , int length, 
                           uint8_t *pcrc1 , uint8_t *pcrc2 )
{
    uint16_t crc_raw;
    uint8_t crc1 , crc2;
    crc_raw = XCAM_crc16(65535, pBuf , length);
    crc1    = (uint8_t)((crc_raw >> 8) & 255);
    crc2    = (uint8_t)( crc_raw       & 255);
    *pcrc1  = crc1;
    *pcrc2  = crc2;
    return;
}
uint16_t XCAM_crc16(uint16_t seed, uint8_t *pBuffer, int length)
{
    uint16_t crc_lut[256] = {
                        0x0000, 0x1021, 0x2042, 0x3063,
                        0x4084, 0x50a5, 0x60c6, 0x70e7,
                        0x8108, 0x9129, 0xa14a, 0xb16b,
                        0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
                        0x1231, 0x0210, 0x3273, 0x2252,
                        0x52b5, 0x4294, 0x72f7, 0x62d6,
                        0x9339, 0x8318, 0xb37b, 0xa35a,
                        0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
                        0x2462, 0x3443, 0x0420, 0x1401,
                        0x64e6, 0x74c7, 0x44a4, 0x5485,
                        0xa56a, 0xb54b, 0x8528, 0x9509,
                        0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
                        0x3653, 0x2672, 0x1611, 0x0630,
                        0x76d7, 0x66f6, 0x5695, 0x46b4,
                        0xb75b, 0xa77a, 0x9719, 0x8738,
                        0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
                        0x48c4, 0x58e5, 0x6886, 0x78a7,
                        0x0840, 0x1861, 0x2802, 0x3823,
                        0xc9cc, 0xd9ed, 0xe98e, 0xf9af,
                        0x8948, 0x9969, 0xa90a, 0xb92b,
                        0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
                        0x1a71, 0x0a50, 0x3a33, 0x2a12,
                        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e,
                        0x9b79, 0x8b58, 0xbb3b, 0xab1a,
                        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5,
                        0x2c22, 0x3c03, 0x0c60, 0x1c41,
                        0xedae, 0xfd8f, 0xcdec, 0xddcd,
                        0xad2a, 0xbd0b, 0x8d68, 0x9d49,
                        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4,
                        0x3e13, 0x2e32, 0x1e51, 0x0e70,
                        0xff9f, 0xefbe, 0xdfdd, 0xcffc,
                        0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
                        0x9188, 0x81a9, 0xb1ca, 0xa1eb,
                        0xd10c, 0xc12d, 0xf14e, 0xe16f,
                        0x1080, 0x00a1, 0x30c2, 0x20e3,
                        0x5004, 0x4025, 0x7046, 0x6067,
                        0x83b9, 0x9398, 0xa3fb, 0xb3da,
                        0xc33d, 0xd31c, 0xe37f, 0xf35e,
                        0x02b1, 0x1290, 0x22f3, 0x32d2,
                        0x4235, 0x5214, 0x6277, 0x7256,
                        0xb5ea, 0xa5cb, 0x95a8, 0x8589,
                        0xf56e, 0xe54f, 0xd52c, 0xc50d,
                        0x34e2, 0x24c3, 0x14a0, 0x0481,
                        0x7466, 0x6447, 0x5424, 0x4405,
                        0xa7db, 0xb7fa, 0x8799, 0x97b8,
                        0xe75f, 0xf77e, 0xc71d, 0xd73c,
                        0x26d3, 0x36f2, 0x0691, 0x16b0,
                        0x6657, 0x7676, 0x4615, 0x5634,
                        0xd94c, 0xc96d, 0xf90e, 0xe92f,
                        0x99c8, 0x89e9, 0xb98a, 0xa9ab,
                        0x5844, 0x4865, 0x7806, 0x6827,
                        0x18c0, 0x08e1, 0x3882, 0x28a3,
                        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e,
                        0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
                        0x4a75, 0x5a54, 0x6a37, 0x7a16,
                        0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
                        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
                        0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
                        0x7c26, 0x6c07, 0x5c64, 0x4c45,
                        0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
                        0xef1f, 0xff3e, 0xcf5d, 0xdf7c,
                        0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
                        0x6e17, 0x7e36, 0x4e55, 0x5e74,
                        0x2e93, 0x3eb2, 0x0ed1, 0x1ef0 };

    uint16_t crc = 65535;
    uint16_t data, temp, temp_rs, temp_xor, temp_ls;


    //for p in range (length):
    for (int p=0; p<length; p++) {
        // data     = np.array(buffer[p],dtype='uint16')
        // temp_rs  = np.array(crc >> 8,dtype='uint16')
        // temp_xor = np.array(temp_rs ^ data,dtype='uint16')
        // temp     = np.array(temp_xor & 255,dtype='uint16')
        // temp_ls  = np.array(crc << 8,dtype='uint16')
        // crc = np.array(crc_lut[temp] ^ temp_ls,dtype='uint16')
        data     = (uint16_t) *pBuffer++;
        temp_rs  = crc >> 8;
        temp_xor = temp_rs  ^ data;
        temp     = temp_xor & 255;
        temp_ls  = crc << 8;
        crc      = crc_lut[temp] ^ temp_ls;
    }

    return crc;
}
HAL_StatusTypeDef XCAM_transmit_receive(uint8_t *ptx_buffer,uint8_t *prx_buffer) {
    HAL_StatusTypeDef I2C_retStat;
    I2C_retStat = XCAM_transmit(ptx_buffer);
    if (I2C_retStat != HAL_OK) {
        fprintf(PAYLOAD,"Bad i2c response from XCAM_transmit\r");
        return(I2C_retStat);
    }
    osDelay(3); // documentation says 2ms
    I2C_retStat = XCAM_receive (prx_buffer);
    if (I2C_retStat != HAL_OK) {
        fprintf(PAYLOAD,"Bad i2c response from XCAM_receive\r");
    }
    return(I2C_retStat);
}


HAL_StatusTypeDef XCAM_transmit(uint8_t *ptx_buffer) {
    HAL_StatusTypeDef I2C_retStat;
    uint16_t XCAM_i2cAddress7bit = XCAM_i2cAddress<<1;
    uint16_t nbytes         = 30;
    uint32_t timeout        = 10;
    uint8_t i;
    //HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, 
    //                                          uint16_t DevAddress, 
    //                                          uint8_t *pData, 
    //                                          uint16_t Size, 
    //                                          uint32_t Timeout);

    for (i=0;i<10;i++)
    {
        if (hi2c3.State != HAL_I2C_STATE_READY)
        {
            fprintf(PAYLOAD,"hi2c3 not ready\r");
        }
        /*
        else {
            fprintf(PAYLOAD,"hi2c3  is ready\r");
        }
        */
        I2C_retStat = HAL_I2C_Master_Transmit(&hi2c3,
                                              XCAM_i2cAddress7bit,
                                              ptx_buffer, nbytes, timeout);
        if (I2C_retStat == HAL_OK)
            break;
        if (I2C_retStat == HAL_ERROR ) fprintf(PAYLOAD,"HAL ERROR \r");
        if (I2C_retStat == HAL_TIMEOUT) fprintf(PAYLOAD,"TIMEOUT \r");
        if (I2C_retStat == HAL_BUSY) fprintf(PAYLOAD,"BUSY \r");
        fprintf(PAYLOAD,"Failed to XCAM Transmit, ERROR %d \r",I2C_retStat);
        osDelay(1000);

        fprintf(PAYLOAD,"\tXCAM_transmit I2C_Reset\r");
        I2C_Reset(&hi2c3);
        osDelay(5);
    }
    // reset the I2C interface
    if(  (  I2C_retStat == HAL_ERROR )
      || (  I2C_retStat == HAL_TIMEOUT)
      || (( I2C_retStat == HAL_BUSY)&&(hi2c3.State == HAL_I2C_STATE_READY)))
    {
        fprintf(PAYLOAD,"\tXCAM_transmit I2C_Reset\r");
        I2C_Reset(&hi2c3);
        osDelay(5);
    }

    return(I2C_retStat);
}

HAL_StatusTypeDef XCAM_receive(uint8_t *prx_buffer) {
    HAL_StatusTypeDef I2C_retStat;
    uint16_t XCAM_i2cAddress7bit = XCAM_i2cAddress<<1;
    uint16_t nbytes         = 260;
    uint32_t timeout        = 50;
    // HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, 
    //                                          uint16_t DevAddress, 
    //                                          uint8_t *pData, 
    //                                          uint16_t Size, 
    //                                          uint32_t Timeout);
    I2C_retStat = HAL_I2C_Master_Receive(&hi2c3,
                                         XCAM_i2cAddress7bit,
                                         prx_buffer, nbytes, timeout);
    if ( I2C_retStat == HAL_OK) {
        //fprintf(PAYLOAD,"\tXCAM_receive return was HAL_OK\r");
    }
    else {
        fprintf(PAYLOAD,"\tXCAM_receive return was NOT HAL_OK\r");
        // reset the I2C interface
        if(  (  I2C_retStat == HAL_ERROR )
          || (  I2C_retStat == HAL_TIMEOUT)
          || (( I2C_retStat == HAL_BUSY)&&(hi2c3.State == HAL_I2C_STATE_READY)))
        {
            fprintf(PAYLOAD,"\tXCAM_transmit I2C_Reset\r");
            I2C_Reset(&hi2c3);
            osDelay(5);
            // retry
            I2C_retStat = HAL_I2C_Master_Receive(&hi2c3,
                                         XCAM_i2cAddress7bit,
                                         prx_buffer, nbytes, timeout);
            if ( I2C_retStat != HAL_OK) {
                fprintf(PAYLOAD,"\t2nd XCAM_receive return was NOT HAL_OK\r");
            }
        }
    }
    return(I2C_retStat);
}


void XCAM_WaitSeconds(int numSeconds, int printit) {
    int ii,jj;
    if(printit==1) fprintf(PAYLOAD,"Waiting %d seconds...",numSeconds);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    for (ii=0;ii<numSeconds;ii++) {
        for (jj=0;jj<10;jj++) {
            osDelay(100); // in ms
            TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
        }
        if(printit==1) fprintf(PAYLOAD,"--%d--",ii);
    }
    if(printit==1) fprintf(PAYLOAD,"\r");
}

// 20201013 end


/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * EPS code alice@ucar.edu 2020 12 30
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
// This code queries the EPS battery voltage and current.
// It writes the information out to a file on the OBC as
// well as to the PAYLOAD terminal emulator. --Alice
void EPS_check(int printToFile, int printToPayload ) {
    long  battV, battC;
    long  voltZ;
    long  statusLUP3, statusLUP5;
    float battVconv = 2.346;
    float battCconv = 3.05 ;
    //float voltZconv = 2.4414063;
    char  timeStr[25];
    FIL  fidB;
    //UINT bw;
    FRESULT fresult;
    FILINFO  file_info;
    char filenm[] = "0:/BATT.txt";
 
    EPS_read( 1,&battV);  //  1 is battery voltage
    EPS_read( 2,&battC);  //  2 is battery current
    EPS_read(11,&voltZ);  // 11 is panel Z voltage
    EPS_read(16,&statusLUP3);
    EPS_read(17,&statusLUP5);
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format);  // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
    sprintf(timeStr,"%04d-%02d-%02d %02d:%02d:%02d",
        sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);

    sprintf(battery_text,
        "%s EPS Batt: voltage(mV): %ld %.2f  \tcurrent(mA): %ld %.2f\r",
        timeStr,
        battV, battVconv*(float)battV,
        battC, battCconv*(float)battC);

    
    if(printToFile==1) {
        fresult = f_open(&fidB, filenm,
                         FA_WRITE | FA_READ | FA_OPEN_ALWAYS);
                      // FA_WRITE|FA_READ|FA_OPEN_ALWAYS|FA_OPEN_EXISTING);//,FA_OPEN_APPEND);
        if( fresult == FR_OK ) {
            fprintf(PAYLOAD,"****************BATT.txt SUCCESS!!!!!!!\r");
            fresult = f_stat (filenm, &file_info);
            if( fresult == FR_OK ) {
                fprintf(PAYLOAD,"****************BATT.txt f_stat SUCCESS!!!!!!!\r");
                fresult = f_lseek(&fidB, file_info.fsize);   //Go to the end of the file
                if( fresult == FR_OK ) {
                    fprintf(PAYLOAD,"****************BATT.txt f_lseek SUCCESS!!!!!!!\r");
                    fprintf((FILE *)&fidB,battery_text);
                    //f_write(&fidB, battery_text, sizeof(battery_text) , &bw);
                }
            }
            f_close(&fidB);
        }
        else {
            fprintf(PAYLOAD,"****************BATT.txt could not be opened!\r");
            fprintf(PAYLOAD,"****************ERROR(%u)\r", (uint16_t)fresult);
        }
    }
    if(printToPayload==1) {
        /*
        fprintf(PAYLOAD,battery_text);
        fprintf(PAYLOAD,
            "EPS Z panel: voltage(mV): %ld %.2f\r",
            voltZ, voltZconv*(float)voltZ);
        */
        fprintf(PAYLOAD,
            "EPS status LUP3 LUP5: %ld %ld \r", statusLUP3, statusLUP5 );
    }
}





// This routine will reads from I2C1 which is the EPS. --Alice
void EPS_read(uint16_t Cmd, long *pEpsValue) {
    HAL_StatusTypeDef I2C_retStat;
    uint8_t timeout = 0;
    uint8_t temp_reg_buff[2];
    uint16_t eps_reg;

    *pEpsValue = -9999;  // initialize to a bad value

    /* Read to Communication interface I2C */
    MX_I2C1_Init(); //Enable I2C1 interface
    do {
        if( timeout >= 1 ) {
            // if there is any error reset the I2C interface
            if((  I2C_retStat == HAL_ERROR )
             ||(  I2C_retStat == HAL_TIMEOUT)
             ||(( I2C_retStat == HAL_BUSY)&&(hi2c1.State == HAL_I2C_STATE_READY))
             )
            {
                I2C_Reset(&hi2c1);
            }
            osDelay(5);

            if( timeout >= 10 ) { // Stop trying after certain times
                break;
            }
        }

        //Read the register from the EPS
        //HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, 
        //                 uint16_t DevAddress, 
        //                 uint16_t MemAddress, 
        //                 uint16_t MemAddSize, 
        //                 uint8_t *pData, 
        //                 uint16_t Size, 
        //                 uint32_t Timeout);
        I2C_retStat = HAL_I2C_Mem_Read(
                          &hi2c1,               // I2C_HandleTypeDef
                          EPS_I2C_ADDRESS<<1,   // DevAddress
                          Cmd,                  // MemAddress
                          sizeof(uint8_t),      // MemAddSize
                          temp_reg_buff,        // *pData
                          sizeof(uint16_t),     // Size
                          10);                  // Timeout
        timeout ++; //count one more try
    }while( I2C_retStat != HAL_OK);

    HAL_I2C_DeInit(&hi2c1); //Disable I2C1 interface

    if (HAL_OK == I2C_retStat) {
        //the register is read successfully
        eps_reg = (temp_reg_buff[0] << 8) + temp_reg_buff[1];
        *pEpsValue = (long)eps_reg;

        //Print out the register value
        //fprintf(PAYLOAD,
        //    "EPS OK+0x%04X   %04ld \t", eps_reg,(long int)eps_reg);
    }
    else{
        //Reading has failed. Possibly the EPS is missing
        fprintf(PAYLOAD,"EPS ERR - executing");
    }

}


// This routine will writes to I2C1 which is the EPS. --Alice
void EPS_write(uint16_t memAddress, uint8_t Value) {
    HAL_StatusTypeDef I2C_retStat;
    uint8_t timeout = 0;

    uint8_t *pData, transmData[2];
    pData = &transmData[0];


    /* Read to Communication interface I2C */
    MX_I2C1_Init(); //Enable I2C1 interface
    do {
        if( timeout >= 1 ) {
            // if there is any error reset the I2C interface
            if((  I2C_retStat == HAL_ERROR )
             ||(  I2C_retStat == HAL_TIMEOUT)
             ||(( I2C_retStat == HAL_BUSY)&&(hi2c1.State == HAL_I2C_STATE_READY))
             )
            {
                I2C_Reset(&hi2c1);
            }
            osDelay(5);

            if( timeout >= 10 ) { // Stop trying after certain times
                break;
            }
        }

        // HAL_StatusTypeDef 
        //     HAL_I2C_Mem_Write(
        //     I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
        //     uint16_t MemAddress,     uint16_t MemAddSize,
        //     uint8_t *pData,          uint16_t Size,         uint32_t Timeout);
        /*
        transmData[0] = Value;
        I2C_retStat = HAL_I2C_Mem_Write(&hi2c1, EPS_I2C_ADDRESS<<1,
                      memAddress,      sizeof(uint16_t), 
                      &transmData[0],  sizeof(uint8_t), 10);
        */

        //HAL_StatusTypeDef HAL_I2C_Master_Transmit(
        //    I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
        //    uint8_t *pData,          uint16_t Size, uint32_t Timeout);
        /**/
        transmData[0] = (uint8_t)memAddress;
        transmData[1] = Value;
        I2C_retStat = HAL_I2C_Master_Transmit(&hi2c1, EPS_I2C_ADDRESS<<1, 
            pData, 2, 10);
        /**/

        timeout ++; //count one more try
    }while( I2C_retStat != HAL_OK);

    HAL_I2C_DeInit(&hi2c1); //Disable I2C1 interface

    if (HAL_OK == I2C_retStat) {
        //the register is read successfully
        //fprintf(PAYLOAD, "EPS OK");
    }
    else{
        //Reading has failed. Possibly the EPS is missing
        fprintf(PAYLOAD,"EPS ERR - executing");
    }

}








/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
