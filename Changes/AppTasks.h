/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @file AppTasks.h
* @brief Header of AppTasks.c
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @author            Vasil MIlev
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
#ifndef APPTASKS_H
#define APPTASKS_H

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INCLUDES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include "main.h"
#include "fatfs.h"

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL DEFINES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define ENABLE_S_BAND_TRANSMITTER
#define ENABLE_X_BAND_TRANSMITTER
// OBC ADCS System task
// #define ENABLE_OBC_ADCS
// OBC board as X/S-Band transmitter testing medium
// #define ENABLE_SX_BAND_TESTBOARD
#ifdef ENABLE_SX_BAND_TESTBOARD
    // Disable OBC ESTTC (command length safety check) and other modules safety mechanisms for the sake of testing the X/S-Band
    #define SX_BAND_TESTBOARD_OBC_SAFETY_OFF
#endif /* ENABLE_SX_BAND_TESTBOARD */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL TYPES DECLARATIONS
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* No External types declarations */

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL VARIABLES DECLARATIONS
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
extern uint8_t  up_sec, up_min, up_hrs;
extern uint32_t up_day;
extern uint16_t UptimePeriod;

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL ROUTINES DECLARATIONS 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void StartDefaultTask(void const * argument);
void Process_Sensors(void const * argument);
void ServicesTask(void const * argument);
void S_X_BAND_Task(void const * argument);


// 20201013
// alice mods below
typedef struct {
    uint8_t  opMode;    // 0==wakeup/standby  1==imaging   (Table 12)
    uint8_t  payIsBusy;             // (Table 18)
    uint8_t  opIsFinished;          // (Table 18)
    uint8_t  dataIsWaiting;         // (Table 18)
    uint8_t  opErrorExists;         // (Table 18)
    uint8_t  opModeErrorExists;     // (Table 18)
    uint8_t  opErrorOutOfMemory;    // (Table 20)
    uint8_t  opErrorAutoExpProb;    // (Table 20)
    uint8_t  opErrorGrabTimeout;    // (Table 20)
    uint8_t  opErrorCISSetupProb;   // (Table 20)
    uint8_t  opErrorCISGrabProb;    // (Table 20)
    uint8_t  opErrorBadParamCombo;  // (Table 20)
    uint32_t data_waiting;
} XCAM_STATUS;
void XCAM_Init(uint8_t *perror);
void XCAM_Exposure(void);
void XCAM_AdjustExposureTime();
void XCAM_payload_parameter_read   (uint8_t   parameter_id, uint16_t *pparameter_value, uint8_t *perror);
void XCAM_payload_parameter_update (uint8_t  parameter_id,  uint16_t parameter_value,   uint8_t *perror);
void XCAM_payload_operations_update(uint8_t operation_mode,   uint8_t *perror);
void XCAM_payload_status_update    (uint8_t *pstatus,         uint8_t *perror);
void XCAM_payload_data_transfer    (uint8_t *ppayload_data,   uint8_t *perror,  uint32_t *whichOne);
void XCAM_zero_tx_rx     ( uint8_t *ptx, uint8_t *prx);
void XCAM_get_crc1andcrc2( uint8_t *pBuf,  int length, uint8_t *pcrc1, uint8_t *pcrc2 );
uint16_t XCAM_crc16      ( uint16_t seed, uint8_t *pBuffer, int length);

HAL_StatusTypeDef XCAM_transmit(uint8_t *ptx_buffer);
HAL_StatusTypeDef XCAM_receive (uint8_t *prx_buffer);
HAL_StatusTypeDef XCAM_transmit_receive (uint8_t *ptx_buffer, uint8_t *prx_buffer);

void XCAM_WaitSeconds(int numSeconds, int printit);
// 20201013 end

void EPS_check(int printToFile, int printToPAYLOAD);
void EPS_read (uint16_t Cmd,  long *pEpsValue);
void EPS_write(uint16_t Addr, uint8_t Value);



// done with alice

#endif    /* APPTASKS_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
