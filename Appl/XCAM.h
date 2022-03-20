/*
Mitch's XCAM Implementation
2022-01-18

*/

#pragma once
#include "stm32f4xx_hal.h"
#include "MCU_Init.h"
#include <stdbool.h>
#include "CRC.h"

// These defines below are used for XCAM errors
// The actual values are ad hoc
//An enum for the latest return from an XCAM i2c call will be stroed within the XCAM device Struct
typedef enum XCAM_Return{
    AOK = 1,
    COMMAND_ACCEPTED = 1,
    PARAMETER_UPLOADED = 1,
    ACKNOWLEDGE_RECEIVED = 1,
    PAYLOAD_ERROR_CONDITION = 5,
    INVALID_MODE_RETURNED = 6,
    CRC_CHECK_FAILED = 7,
    BAD_RETURN = 250,
    UNSPECIFIED_ERROR = 255
}XCAM_Return;

// Enum for the Registers of the XCAM
typedef enum XCAM_Registers{
    XCAM_SENSOR = 0x00,   // use 1, WFI at HAO
    XCAM_INTTIME =0x01,
    XCAM_GRAB = 0x02,
    XCAM_AUTOEXP = 0x03,
    XCAM_HEALTH = 0x04,
    XCAM_COMPRESS = 0x07,
    XCAM_THUMB = 0x08,
    XCAM_WINDOW = 0x0B ,
    XCAM_TIMEOUT = 0x12,
    XCAM_INTTIMEFRAC = 0x13,
    XCAM_OP_ERROR = 0x15
}XCAM_Registers;


//The Current Status of these XCAM parameters will be updated within the
typedef struct XCAM_Status{
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
} XCAM_Status;



//This Should be Primarily what is passed between functions at higher-level
typedef struct XCAM_device{

    bool is_initialized = false;
    uint16_t XCAM_i2cAddress     = 0x66;

    //Statuses
    XCAM_Status xstat;
    HAL_StatusTypeDef Halstat;
    XCAM_Return xret;


    //XCAM Reg
    XCAM_Registers xreg;


    int blen;

    //Buffers
    uint8_t rx_buffer[260];
    uint8_t tx_buffer[30];
    uint16_t XCAM_sensor   =  1; // 1 == WFI
    uint16_t XCAM_autoexp  =  0; // 1 == use auto exposure
    uint16_t XCAM_timeout  = 30; // the time C3D goes back to standby
                                 // after a grab command
    uint16_t XCAM_inttime  =  200;
    uint16_t XCAM_inttimefrac = 0;  // just an adhoc value for now
    uint16_t XCAM_window   =  0;
    uint16_t XCAM_compress =  0; // = 0 no compression, 1=compression
    uint16_t XCAM_thumb    =  0; // = 0 no thumbnail  , 1=thumbnail
    uint16_t XCAM_grab     =  1; // = 0 no grab, 1=grab an image
    uint8_t  XCAM_mode     =  1; // 1=imaging mode

    bool      exposureIsDone = 1;

}XCAM_device;


//Example
void XCAM_Example(XCAM_device XCAM);

//Should be exposed to outside implementations
//Public
uint8_t XCAM_Init(XCAM_device XCAM);
void XCAM_TakeExposure(XCAM_device XCAM);
void XCAM_AdjustExposureTime(XCAM_device XCAM);

//Should not be used by outside implementations
//Private
static uint8_t XCAM_GetStatus(uint8_t *status);
static uint16_t XCAM_AnalyzeStatus(uint8_t *status);
static uint8_t XCAM_GetImageSPI(uint8_t *buffer);
static uint8_t XCAM_GetImageI2C(uint8_t *buffer);
static uint8_t XCAM_SetParameter(uint8_t ID, uint16_t value);
static uint8_t XCAM_SendInitCommand(void);
static uint8_t XCAM_EnableImagingMode(void);
static uint8_t XCAM_SendInitOrUpdate(bool init, bool imagingmode);
static void XCAM_PrintACKOrResponse(uint8_t *buffer, size_t len);


//low-level Should not be used by outside implementations
static void XCAM_SetCRC(uint8_t* data, size_t len);
static bool XCAM_ValidateCRC(uint8_t* data, size_t len);
static uint16_t XCAM_crc16(uint16_t seed, uint8_t *pBuffer, int length);
static uint8_t XCAM_transmit(uint8_t *buffer, size_t len);
static uint8_t XCAM_receive(uint8_t *buffer, size_t len, bool ack);
static void XCAM_WaitSeconds(uint16_t numSeconds, bool printit);
static void XCAM_zero_tx_rx(tx_buffer, rx_buffer);




void XCAM_Init(XCAM_device XCAM);
void XCAM_Exposure(XCAM_device XCAM);
void XCAM_AdjustExposureTime(XCAM_device XCAM);


void XCAM_payload_parameter_read   (uint8_t   parameter_id, uint16_t *pparameter_value, uint8_t *perror);
void XCAM_payload_parameter_update (uint8_t  parameter_id,  uint16_t parameter_value,   uint8_t *perror);
void XCAM_payload_operations_update(uint8_t operation_mode,   uint8_t *perror);
void XCAM_payload_status_update    (uint8_t *pstatus,         uint8_t *perror);
void XCAM_payload_data_transfer    (uint8_t *ppayload_data,   uint8_t *perror,  uint32_t *whichOne);
void XCAM_zero_tx_rx     ( uint8_t *ptx, uint8_t *prx);
void XCAM_get_crc1andcrc2( uint8_t *pBuf,  int length, uint8_t *pcrc1, uint8_t *pcrc2 );
uint16_t XCAM_crc16      ( uint16_t seed, uint8_t *pBuffer, int length);


void XCAM_WaitSeconds(int numSeconds, int printit);



// done with alice

