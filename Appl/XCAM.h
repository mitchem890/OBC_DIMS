#ifndef XCAM_H
#define XCAM_H

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INCLUDES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include "main.h"
#include "fatfs.h"

//Return Values from XCAM Command
//Values are all ad hoc
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
#define RX_BUFFER_LEN 260
#define TX_BUFFER_LEN 30

#define XCAM_CRC_SEED 65535
uint16_t XCAM_inttime  =  200;

//Different Statuses of XCAM
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

typedef struct {
    XCAM_STATUS status;
    uint8_t  status_buffer[RX_BUFFER_LEN];
    bool initialized;
    bool exposureIsDone;
    uint8_t tx_buffer[TX_BUFFER_LEN];
    uint8_t rx_buffer[RX_BUFFER_LEN];
    uint8_t error;
    uint16_t check;
} XCAM;

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



#endif    /* APPTASKS_H */