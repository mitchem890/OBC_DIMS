/*!
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @file X_Band_Trnsm.h
* @brief Header of X_Band_Trnsm.c
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* @author            Vassil Milev
* @version           1.0.0
* @date              2019.08.15
*
* @copyright         (C) Copyright Endurosat
*
*                    Contents and presentations are protected world-wide.
*                    Any kind of using, copying etc. is prohibited without prior permission.
*                    All rights - incl. industrial property rights - are reserved.
*
* @history
* @revision{         1.0.0  , 2019.08.15, author Vassil Milev, Initial revision }
* @revision{         1.0.1  , 2019.12.02, author Vassil Milev, Added Read and Write commands for all Attenuations }
* @endhistory
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#ifndef X_BAND_TRNSM_H
#define X_BAND_TRNSM_H

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* INCLUDES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include "main.h"

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL DEFINES
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define X_BAND_TRNSM_STRCT_SIZE         (14)     /* size of one packet without the data and the CRC */
#define X_BAND_TRNSM_TX_BUFF_SIZE       (1536)
#define X_BAND_TRNSM_RX_BUFF_SIZE       (1536)
#define X_BAND_ID_LENGT               (4)      /* Length of the S-Band identifier */
#define X_BAND_ID_RANGE_MIN           (0x1000) /* Length of the S-Band identifier */
#define X_BAND_ID_RANGE_MAX           (0x1FFF) /* Length of the S-Band identifier */
/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL TYPES DECLARATIONS
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
typedef struct{
    uint32_t Header;
    uint16_t ModuleID;
    uint16_t Length;
    uint16_t Responce;
    uint16_t CMD;
    uint16_t Type;
}__packed X_BAND_TRNSM_Pack_struct;

/*---------------------------------------------------------------------------------------
Message 0x0049 - Tranciver Chip AD9364 All Tranciver parameters.
-----------------------------------------------------------------------------------------
 Description: Message to XTR to write data to the AD9364
 Total payload: 12 bytes
 Structure of the payload: ALL TRANCIVER PARAMS
    Byte 0   |  1 byte    |  Symbol_Rate
    Byte 1   |  1 byte    |  TX_Power
    Byte 2   |  1 byte    |  Modulation_Code
    Byte 3   |  1 byte    |  Roll_Off
    Byte 4   |  1 byte    |  Pilot_Signal_On_Off
    Byte 5   |  1 byte    |  FEC_Frame_Size
    Byte 6   |  2 byte    |  Pre_Trans_Staff_Delay
    Byte 8   |  4 byte    |  Central_Frequency
*/
typedef struct{
    uint8_t  SymbolRate;
    uint8_t  TxPower;
    uint8_t  MODCOD;
    uint8_t  RollOff;
    uint8_t  PilotSygnal;
    uint8_t  FEC_Frame_Size;
    uint16_t PreTxStaffingDelay;
    float    CentralFreq;
}__packed X_BAND_TRNSM_GET_AllParams_struct;

/*---------------------------------------------------------------------------------------
Message 0x004E - Module Status Mixed Report.
-----------------------------------------------------------------------------------------
 Description: Message to Master Device Reporting Status of XTR
 Total payload: 12 bytes
 Structure of the payload: ALL TRANCIVER PARAMS
    Byte 0   |  1 byte    |  Sysyem_State
    Byte 1   |  1 byte    |  Status_Flags
    Byte 2   |  2 byte    |  Reserved
    Byte 4   |  4 byte    |  CPU_Temperature
    Byte 8   |  4 byte    |  FW_Version
*/

typedef struct{
    uint8_t  SystemState;
    uint8_t  StatusFlag;
    int16_t  PA_Temp;
    uint16_t TxPower_m;
}__packed X_BAND_TRNSM_GET_SympleReport_struct;

typedef struct{
    uint8_t  SystemState;
    uint8_t  StatusFlag;
    int16_t  PA_Temp;
    uint16_t TxPower_m;
    uint8_t  PredistortionStat;
    uint8_t  PredistortionErr;
    uint8_t  PredistortionWarning;
    uint8_t  PredistortionFreqRange;
    uint8_t  PredistortionAdaptMode;
    uint8_t  PredistortionAdaptState;
    uint8_t  PredistortionOutputStat;
    uint8_t  PredistortionNomrFact;
    uint8_t  PredistortionRFIN_AGC;
    uint8_t  PredistortionRFRB_AGC;
    uint16_t PredistortionUnnormCoeff;
    int16_t  PredistortionInternTemp;
    float    PredisMinFreqScan;
    float    PredisMaxFreqScan;
    float    PredisSignalBandwidth;
    float    PredisCentralFreq;
    float    CPU_Temperature;
    uint32_t FW_Version;
}__packed X_BAND_TRNSM_GET_FullReport_struct;

typedef struct{
    float Attenuation27dBmF8025_8100Mhz;
    float Attenuation28dBmF8025_8100Mhz;
    float Attenuation29dBmF8025_8100Mhz;
    float Attenuation30dBmF8025_8100Mhz;
    float Attenuation31dBmF8025_8100Mhz;
    float Attenuation32dBmF8025_8100Mhz;
    float Attenuation33dBmF8025_8100Mhz;

    float Attenuation27dBmF8100_8200Mhz;
    float Attenuation28dBmF8100_8200Mhz;
    float Attenuation29dBmF8100_8200Mhz;
    float Attenuation30dBmF8100_8200Mhz;
    float Attenuation31dBmF8100_8200Mhz;
    float Attenuation32dBmF8100_8200Mhz;
    float Attenuation33dBmF8100_8200Mhz;

    float Attenuation27dBmF8200_8300Mhz;
    float Attenuation28dBmF8200_8300Mhz;
    float Attenuation29dBmF8200_8300Mhz;
    float Attenuation30dBmF8200_8300Mhz;
    float Attenuation31dBmF8200_8300Mhz;
    float Attenuation32dBmF8200_8300Mhz;
    float Attenuation33dBmF8200_8300Mhz;

    float Attenuation27dBmF8300_8400Mhz;
    float Attenuation28dBmF8300_8400Mhz;
    float Attenuation29dBmF8300_8400Mhz;
    float Attenuation30dBmF8300_8400Mhz;
    float Attenuation31dBmF8300_8400Mhz;
    float Attenuation32dBmF8300_8400Mhz;
    float Attenuation33dBmF8300_8400Mhz;
}__packed X_BAND_TRNSM_GET_AttenuationParams_struct;


typedef struct{
    uint8_t  DirNextAvailable;
    uint16_t NumberFiles;
    uint8_t  * FileList;
}__packed X_BAND_TRNSM_FileList_struct;

typedef struct{
    uint32_t  Size;          /* size of the file */
    uint8_t  NameLength;    /* length of the name of the file */
    uint8_t  FileName[31];  /* String with the name of the file */
}X_BAND_TRNSM_FileInfo_struct;

typedef struct{
    uint16_t  Reserved;
    uint16_t  Size;
    int32_t   FileHandler;
    uint32_t  PacketNumber;
    uint8_t   Data[X_BAND_TRNSM_TX_BUFF_SIZE];
}__packed X_BAND_TRNSM_WriteFile_struct;

typedef struct{
    uint8_t  Reserved;
    uint8_t  Status;
    uint16_t  Length;
    uint32_t  PacketNumber;
    uint8_t   Data[X_BAND_TRNSM_TX_BUFF_SIZE];
}__packed X_BAND_TRNSM_ReadFile_struct;

typedef enum{
#ifdef ENABLE_SX_BAND_TESTBOARD
    X_BAND_TRNSM_CMD_PD_RF_OUT_ENB_DIS,
    X_BAND_TRNSM_CMD_PD_CLEAR_WARNINGS,
    X_BAND_TRNSM_CMD_PD_CALIBRATION_A,
    X_BAND_TRNSM_CMD_PD_REWRITE_CONFIG,
    X_BAND_TRNSM_CMD_PD_RESET,
    X_BAND_TRNSM_CMD_PD_ADAPTATION_ENB_DIS,
#endif /* ENABLE_SX_BAND_TESTBOARD */
    X_BAND_TRNSM_CMD_GET,               /* Get status */
    X_BAND_TRNSM_CMD_SET,               /* Set status */
    X_BAND_TRNSM_CMD_DIR_F,             /* Dir list of files in the SD Card */
    X_BAND_TRNSM_CMD_DIR_NEXT_F,        /* Get next page from the list of file after cmd DIR */
    X_BAND_TRNSM_CMD_DELL_F,            /* Delete a file */
    X_BAND_TRNSM_CMD_DELL_ALL_F,        /* Delete all files */
    X_BAND_TRNSM_CMD_CREATE_F,          /* Create a file */
    X_BAND_TRNSM_CMD_WRITE_F,           /* Write data to a file */
    X_BAND_TRNSM_CMD_OPEN_F,            /* Open a file */
    X_BAND_TRNSM_CMD_READ_F,            /* Read data from a file */
    X_BAND_TRNSM_CMD_SEND_F,            /* Send a file by the S-Band RF transmitter */
    X_BAND_TRNSM_CMD_TX_MODE,           /* Enter Transmit mode (High power consumption) */
    X_BAND_TRNSM_CMD_LOAD_MODE,         /* Exit Transmit mode (Low power consumption) */
    X_BAND_TRNSM_CMD_UPDATE_FW,         /* Update the firmware from a file */
    X_BAND_TRNSM_CMD_SAFE_SHUTDOWN,     /* Terminate all pending operations and prepare for shut down (Shut down must be done 1 second after that command) */
    X_BAND_TRNSM_CMD_GET_RES,           /* Pool the result from the last command */
    X_BAND_TRNSM_CMD_NUMBER             /* Get the number of all command */
}X_BAND_TRNSM_CMD_enum;

/* Values of all commands */
typedef enum{
#ifdef ENABLE_SX_BAND_TESTBOARD
    X_BAND_TRNSM_CMD_VAL_PD_RF_OUT_ENB_DIS     = 0x0070,
    X_BAND_TRNSM_CMD_VAL_PD_CLEAR_WARNINGS     = 0x0071,
    X_BAND_TRNSM_CMD_VAL_PD_CALIBRATION_A      = 0x0072,
    X_BAND_TRNSM_CMD_VAL_PD_REWRITE_CONFIG     = 0x0073,
    X_BAND_TRNSM_CMD_VAL_PD_RESET              = 0x0074,
    X_BAND_TRNSM_CMD_VAL_PD_ADAPTATION_ENB_DIS = 0x0075,
#endif /* ENABLE_SX_BAND_TESTBOARD */
    X_BAND_TRNSM_CMD_VAL_GET           = 0x0100,
    X_BAND_TRNSM_CMD_VAL_SET           = 0x0101,
    X_BAND_TRNSM_CMD_VAL_DIR_F         = 0x0102,
    X_BAND_TRNSM_CMD_VAL_DIR_NEXT_F    = 0x0103,
    X_BAND_TRNSM_CMD_VAL_DELL_F        = 0x0104,
    X_BAND_TRNSM_CMD_VAL_DELL_ALL_F    = 0x0105,
    X_BAND_TRNSM_CMD_VAL_CREATE_F      = 0x0106,
    X_BAND_TRNSM_CMD_VAL_WRITE_F       = 0x0107,
    X_BAND_TRNSM_CMD_VAL_OPEN_F        = 0x0108,
    X_BAND_TRNSM_CMD_VAL_READ_F        = 0x0109,
    X_BAND_TRNSM_CMD_VAL_SEND_F        = 0x010A,
    X_BAND_TRNSM_CMD_VAL_TX_MODE       = 0x0110,
    X_BAND_TRNSM_CMD_VAL_LOAD_MODE     = 0x0111,
    X_BAND_TRNSM_CMD_VAL_UPDATE_FW     = 0x0112,
    X_BAND_TRNSM_CMD_VAL_SAFE_SHUTDOWN = 0x0113,
    X_BAND_TRNSM_CMD_VAL_GET_RESULT    = 0x0114,
    X_BAND_TRNSM_CMD_VAL_NUMBER
}X_BAND_TRNSM_CMD_VAL_enum;


/* Available types for CMD "Get Result" ( 0x0101 ) */
typedef enum{
#ifdef ENABLE_SX_BAND_TESTBOARD
    X_BAND_TRNSM_RET_RES_PD_RF_OUT_ENB_DIS           = X_BAND_TRNSM_CMD_VAL_PD_RF_OUT_ENB_DIS,
    X_BAND_TRNSM_RET_RES_PD_CLEAR_WARNINGS           = X_BAND_TRNSM_CMD_VAL_PD_CLEAR_WARNINGS,
    X_BAND_TRNSM_RET_RES_PD_CALIBRATION_A            = X_BAND_TRNSM_CMD_VAL_PD_CALIBRATION_A,
    X_BAND_TRNSM_RET_RES_PD_REWRITE_CONFIG           = X_BAND_TRNSM_CMD_VAL_PD_REWRITE_CONFIG,
    X_BAND_TRNSM_RET_RES_PD_RESET                    = X_BAND_TRNSM_CMD_VAL_PD_RESET,
    X_BAND_TRNSM_RET_RES_PD_ADAPTATION_ENB_DIS       = X_BAND_TRNSM_CMD_VAL_PD_ADAPTATION_ENB_DIS,
#endif /* ENABLE_SX_BAND_TESTBOARD */
    X_BAND_TRNSM_RET_RES_GET           = X_BAND_TRNSM_CMD_VAL_GET        ,
    X_BAND_TRNSM_RET_RES_SET           = X_BAND_TRNSM_CMD_VAL_SET        ,
    X_BAND_TRNSM_RET_RES_DIR_F         = X_BAND_TRNSM_CMD_VAL_DIR_F      ,
    X_BAND_TRNSM_RET_RES_DIR_NEXT_F    = X_BAND_TRNSM_CMD_VAL_DIR_NEXT_F ,
    X_BAND_TRNSM_RET_RES_DELL_F        = X_BAND_TRNSM_CMD_VAL_DELL_F     ,
    X_BAND_TRNSM_RET_RES_DELL_ALL_F    = X_BAND_TRNSM_CMD_VAL_DELL_ALL_F ,
    X_BAND_TRNSM_RET_RES_CREATE_F      = X_BAND_TRNSM_CMD_VAL_CREATE_F   ,
    X_BAND_TRNSM_RET_RES_WRITE_F       = X_BAND_TRNSM_CMD_VAL_WRITE_F    ,
    X_BAND_TRNSM_RET_RES_OPEN_F        = X_BAND_TRNSM_CMD_VAL_OPEN_F     ,
    X_BAND_TRNSM_RET_RES_READ_F        = X_BAND_TRNSM_CMD_VAL_READ_F     ,
    X_BAND_TRNSM_RET_RES_SEND_F        = X_BAND_TRNSM_CMD_VAL_SEND_F     ,
    X_BAND_TRNSM_RET_RES_TX_MODE       = X_BAND_TRNSM_CMD_VAL_TX_MODE    ,
    X_BAND_TRNSM_RET_RES_LOAD_MODE     = X_BAND_TRNSM_CMD_VAL_LOAD_MODE  ,
    X_BAND_TRNSM_RET_RES_UPDATE_FW     = X_BAND_TRNSM_CMD_VAL_UPDATE_FW  ,
    X_BAND_TRNSM_RET_RES_NUMBER
}X_BAND_TRNSM_RetRes_enum;

/* Types for all commands that don't have type */
#define X_BAND_TRNSM_NULL_TYPE          ((uint16_t)(0x0000))

/* Available types for CMD "GET" ( 0x0100 ) */
typedef enum{
    X_BAND_TRNSM_GET_TYPE_SYMBOL_RATE     = 0x0040,
    X_BAND_TRNSM_GET_TYPE_TX_POWER        = 0x0041,
    X_BAND_TRNSM_GET_TYPE_CENTRAL_FREQ    = 0x0042,
    X_BAND_TRNSM_GET_TYPE_MODCOD          = 0x0043,
    X_BAND_TRNSM_GET_TYPE_ROLL_OFF        = 0x0044,
    X_BAND_TRNSM_GET_TYPE_PILOT_SIGNAL    = 0x0045,
    X_BAND_TRNSM_GET_TYPE_FEC_FRAME_SIZE  = 0x0046,
    X_BAND_TRNSM_GET_TYPE_PRETRASIT_DELAY = 0x0047,
    X_BAND_TRNSM_GET_TYPE_ALL_CHANGE_MODE = 0x0048,
    X_BAND_TRNSM_GET_TYPE_SIMPLE_REPORT   = 0x0049,
    X_BAND_TRNSM_GET_TYPE_FULL_REPORT     = 0x004A, //X-Band only
    X_BAND_TRNSM_GET_TYPE_ATTENUATION_PAR = 0x004C,
    X_BAND_TRNSM_GET_TYPE_NUMBER
}X_BAND_TRNSM_GET_types_enum;

/* Available types for CMD "SET" ( 0x0101 ) */
typedef enum{
    X_BAND_TRNSM_SET_TYPE_SYMBOL_RATE     = 0x0040,
    X_BAND_TRNSM_SET_TYPE_TX_POWER        = 0x0041,
    X_BAND_TRNSM_SET_TYPE_CENTRAL_FREQ    = 0x0042,
    X_BAND_TRNSM_SET_TYPE_MODCOD          = 0x0043,
    X_BAND_TRNSM_SET_TYPE_ROLL_OFF        = 0x0044,
    X_BAND_TRNSM_SET_TYPE_PILOT_SIGNAL    = 0x0045,
    X_BAND_TRNSM_SET_TYPE_FEC_FRAME_SIZE  = 0x0046,
    X_BAND_TRNSM_SET_TYPE_PRETRASIT_DELAY = 0x0047,
    X_BAND_TRNSM_SET_TYPE_ALL_CHANGE_MODE = 0x0048,
    X_BAND_TRNSM_SET_TYPE_BAUDRATE        = 0x004B,
    X_BAND_TRNSM_SET_TYPE_NUMBER
}X_BAND_TRNSM_SET_types_enum;

/* Available types for CMD "Send File" ( 0x010A ) */
typedef enum{
    X_BAND_TRNSM_SEND_F_TYPE_WITHOUT_ISSUES = 0x0050,
    X_BAND_TRNSM_SEND_F_TYPE_WITH_PREDST    = 0x0051,
    X_BAND_TRNSM_SEND_F_TYPE_WITH_RF_TRACT  = 0x0052,
    X_BAND_TRNSM_SEND_F_TYPE_PREDIS_AND_RF  = 0x0053,   /* Recommended */
    X_BAND_TRNSM_SEND_F_TYPE_NUMBER
}X_BAND_TRNSM_SendFile_types_enum;

/* Return statuses from sending command to S-Band transmitter */
typedef enum{
    X_BAND_TRNSM_STAT_GET_RESULT  = 0x00,
    X_BAND_TRNSM_STAT_ACK         = 0x05,
    X_BAND_TRNSM_STAT_NACK        = 0x06,
    X_BAND_TRNSM_STAT_BUSY        = 0x07,
    X_BAND_TRNSM_STAT_NCE         = 0x08,
    X_BAND_TRNSM_STAT_STACK_FULL  = 0x09,
    X_BAND_TRNSM_STAT_CTNA        = 0x0A,
    X_BAND_TRNSM_STAT_WRONG_PARAM = 0xFE,
    X_BAND_TRNSM_STAT_COMM_ERR    = 0xFF,
    X_BAND_TRNSM_STAT_NUMBER
}X_BAND_TRNSM_Responce_enum;


/* Return statuses from CMD Delete and Delete All */
typedef enum{
    X_BAND_TRNSM_DELL_OK         = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_DELL_NOT_FOUND  = 0x01,    /* The file is not found */
    X_BAND_TRNSM_DELL_CARD_ERR   = 0x03,    /* SD card Error */
    X_BAND_TRNSM_DELL_PARAMS_ERR = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_DELL_COMM_ERR   = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_DELL_NUMBER
}X_BAND_TRNSM_DellStatus_enum;

/* Return statuses from CMD Create a file */
typedef enum{
    X_BAND_TRNSM_CREATE_OK         = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_CREATE_ERR        = 0x01,    /* The file cannot be created - maybe it is already existing (should be deleted first) */
    X_BAND_TRNSM_CREATE_CARD_ERR   = 0x03,    /* SD card Error */
    X_BAND_TRNSM_CREATE_PARAMS_ERR = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_CREATE_COMM_ERR   = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_CREATE_NUMBER
}X_BAND_TRNSM_CrateStatus_enum;

/* Return statuses from CMD Open a file */
typedef enum{
    X_BAND_TRNSM_OPEN_OK         = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_OPEN_ERR        = 0x01,    /* The file cannot be open - it may not exist */
    X_BAND_TRNSM_OPEN_CARD_ERR   = 0x03,    /* SD card Error */
    X_BAND_TRNSM_OPEN_PARAMS_ERR = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_OPEN_COMM_ERR   = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_OPEN_NUMBER
}X_BAND_TRNSM_OpenStatus_enum;

/* Return statuses from CMD Send a file */
typedef enum{
    X_BAND_TRNSM_SEND_OK              = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_SEND_ERR             = 0x01,    /* The file cannot be Send - maybe it is missing */
    X_BAND_TRNSM_SEND_CARD_ERR        = 0x03,    /* SD card Error */
    X_BAND_TRNSM_SEND_FILE_COMM_ERR   = 0x04,    /* RF error */
    X_BAND_TRNSM_SEND_FILE_NOT_READY  = 0x06,    /* The file is not ready to be transmitted */
    X_BAND_TRNSM_SEND_PARAMS_ERR      = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_SEND_COMM_ERR        = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_SEND_NUMBER
}X_BAND_TRNSM_SendStatus_enum;

/* Return statuses from all commands about changing the mode of the S-Band Transceiver */
typedef enum{
    X_BAND_TRNSM_CHANG_MODE_OK              = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_CHANG_MODE_SYS_ERR         = 0x01,    /* System error */
    X_BAND_TRNSM_CHANG_MODE_BUSY            = 0x03,    /* SD card Error */
    X_BAND_TRNSM_CHANG_MODE_PARAMS_ERR      = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_CHANG_MODE_COMM_ERR        = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_CHANG_MODE_NUMBER
}X_BAND_TRNSM_ChangeMode_enum;

/* Return statuses from all CMD Firmware update */
typedef enum{
    X_BAND_TRNSM_FW_UPDATE_OK              = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_FW_UPDATE_FW_FILE_ERR     = 0x01,    /* The file is corrupted */
    X_BAND_TRNSM_FW_UPDATE_OLD_SAME_FW     = 0x02,    /* Earlier or same FW version */
    X_BAND_TRNSM_FW_UPDATE_SD_CARD_ERR     = 0x03,    /* SD Card Error */
    X_BAND_TRNSM_FW_UPDATE_UPDATE_FAILD    = 0xFC,    /* Updated started, but finished with a Error */
    X_BAND_TRNSM_FW_UPDATE_PARAMS_ERR      = 0xFE,    /* Parameter Error */
    X_BAND_TRNSM_FW_UPDATE_COMM_ERR        = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_FW_UPDATE_NUMBER
}X_BAND_TRNSM_FW_update_enum;

/* Return statuses from CMD Set and Get parameters */
typedef enum{
    X_BAND_TRNSM_PARAMS_OK         = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_PARAMS_PARAMS_ERR = 0x03,    /* Parameter Error */
    X_BAND_TRNSM_PARAMS_COMM_ERR   = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_PARAMS_NUMBER
}X_BAND_TRNSM_SetGetParams_enum;

/* Return statuses from CMD Dir and Dir Next*/
typedef enum{
    X_BAND_TRNSM_DIR_OK         = 0x00,    /* Delete complete successful */
    X_BAND_TRNSM_DIR_NO_FILES   = 0x01,    /* The files cannot be found */
    X_BAND_TRNSM_DIR_CARD_ERR   = 0x03,    /* SD card Error */
    X_BAND_TRNSM_DIR_COMM_ERR   = 0xFF,    /* Communication Error with S-Band */
    X_BAND_TRNSM_DIR_NUMBER
}X_BAND_TRNSM_Dir_enum;


typedef struct{
    X_BAND_TRNSM_CMD_VAL_enum   CMD;
    uint16_t                    tx_data_max_size;
    uint16_t                    rx_data_max_size;
}__packed X_BAND_TRNSM_CMD_INFO_TxInfo_struct;


/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL VARIABLES DECLARATIONS
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
extern uint16_t X_Band_Index;
extern uint8_t X_BAND_TRNSM_X_Band_Data[];

/*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EXTERNAL ROUTINES DECLARATIONS 
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void X_BAND_TRNSM_Init(void);
void X_BAND_TRNSM_Task(void const * argument);
uint32_t X_BAND_TRNSM_GetBaudrate(void);

X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_SymbolRate(uint16_t Identifier, uint8_t * pSymbolRate);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_TxPower(uint16_t Identifier, uint8_t * pTxPower);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_Modcod(uint16_t Identifier, uint8_t * pModcod);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_RollOff(uint16_t Identifier, uint8_t * pRollOff);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_PilotSygnal(uint16_t Identifier, uint8_t * pPilotSygnal);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_FEC_FrameSize(uint16_t Identifier, uint8_t * pFrameSize);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_PreTransDelay(uint16_t Identifier, uint16_t * pPreTransDelay);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_Param_CentralFreq(uint16_t Identifier, float * pCentralFreq);

X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_SymbolRate(uint16_t Identifier, uint8_t * pSymbolRate);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_TxPower(uint16_t Identifier, uint8_t * pTxPower);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_Modcod(uint16_t Identifier, uint8_t * pModcod);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_RollOff(uint16_t Identifier, uint8_t * pRollOff);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_PilotSygnal(uint16_t Identifier, uint8_t * pPilotSygnal);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_FEC_FrameSize(uint16_t Identifier, uint8_t * pFrameSize);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_PreTransDelay(uint16_t Identifier, uint16_t * pPreTransDelay);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_CentralFreq(uint16_t Identifier, float * pCentralFreq);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_Param_Baudrate(uint16_t Identifier, uint8_t pBaudrate);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_AttenuationParms(uint16_t Identifier, X_BAND_TRNSM_GET_AttenuationParams_struct * Attenuation);

X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_AllParameters(uint16_t Identifier, X_BAND_TRNSM_GET_AllParams_struct * Params);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_SET_AllParameters(uint16_t Identifier, X_BAND_TRNSM_GET_AllParams_struct * Params);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_SimpleReport(uint16_t Identifier, X_BAND_TRNSM_GET_SympleReport_struct * SysStatus);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_FullReport(uint16_t Identifier, X_BAND_TRNSM_GET_FullReport_struct * SysStatus);
X_BAND_TRNSM_SetGetParams_enum X_BAND_TRNSM_CMD_GET_AttenuationParams(uint16_t Identifier, X_BAND_TRNSM_GET_AttenuationParams_struct * SysStatus);
X_BAND_TRNSM_Dir_enum X_BAND_TRNSM_CMD_Dir(uint16_t Identifier, X_BAND_TRNSM_FileList_struct * DirData);
X_BAND_TRNSM_Dir_enum X_BAND_TRNSM_CMD_DirNext(uint16_t Identifier, X_BAND_TRNSM_FileList_struct * Params);
uint8_t X_BAND_TRNSM_FileNameParcer(uint16_t Identifier, uint8_t * FileListBuffer, X_BAND_TRNSM_FileInfo_struct * File);
X_BAND_TRNSM_DellStatus_enum X_BAND_TRNSM_DellFile(uint16_t Identifier, uint8_t * FileName);
X_BAND_TRNSM_DellStatus_enum X_BAND_TRNSM_DellAllFile(uint16_t Identifier);
X_BAND_TRNSM_CrateStatus_enum X_BAND_TRNSM_CreateFile(uint16_t Identifier, uint8_t * FileName, uint32_t FileSize, int32_t * FileHandler);
X_BAND_TRNSM_OpenStatus_enum X_BAND_TRNSM_OpenFile(uint16_t Identifier, uint8_t * FileName, uint32_t * FileHandler, uint32_t * FileSize);
uint8_t X_BAND_TRNSM_UploadFileToSBandTrnsm(uint16_t Identifier, uint8_t * FileName);
uint8_t X_BAND_TRNSM_DownloadFileFromSBandTrnsm(uint16_t Identifier, uint8_t * FileName);
X_BAND_TRNSM_SendStatus_enum X_BAND_TRNSM_SendFile(uint16_t Identifier, uint8_t * FileName);
X_BAND_TRNSM_ChangeMode_enum X_BAND_TRNSM_StartTxMode(uint16_t Identifier);
X_BAND_TRNSM_ChangeMode_enum X_BAND_TRNSM_StartLoadMode(uint16_t Identifier);
X_BAND_TRNSM_FW_update_enum X_BAND_TRNSM_FW_Update(uint16_t Identifier, uint8_t * FileName );
X_BAND_TRNSM_ChangeMode_enum X_BAND_TRNSM_ShutDownMode(uint16_t Identifier);
void X_BAND_TRNSM_ESTTC_StartCmd(uint16_t Identifier, uint8_t Cmd, uint8_t Type, uint8_t size, uint8_t * pData);

/* Low level functions */
X_BAND_TRNSM_Responce_enum X_BAND_TRNSM_SendCMD(uint16_t Identifier, X_BAND_TRNSM_CMD_enum CMD, uint16_t CMD_Type, uint8_t * TxData, uint16_t TxDataLenght);
X_BAND_TRNSM_Responce_enum X_BAND_TRNSM_GetResult(uint16_t Identifier, X_BAND_TRNSM_CMD_enum CMD, X_BAND_TRNSM_RetRes_enum CMD_Type, uint8_t * RxData, uint16_t * RxDataLenght);
/* End of Low level functions */

#endif    /* X_BAND_TRNSM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
