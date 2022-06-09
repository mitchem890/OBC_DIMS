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
/*TODO-Style System is working but why do we use both Delay(int ms) and XCAM_WaitSeconds(Throughout Code) -- Should think about making things more homogeneuos
 *
 *
 *
 *
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

      D_XCAM_Example();

      //----------------------------sensors init end
      TaskMonitor_TaskInitialized(TASK_MONITOR_DEFAULT);   /* The task is initialized and is ready */

      // D_XCAM_Example();   // Damon's nice code

      /* XCAM */
    

}