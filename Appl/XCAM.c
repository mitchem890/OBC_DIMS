// 20201013
/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * XCAM code alice@ucar.edu 2020 09 17
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

//Initialize the XCAM Device
#include "XCAM.h"
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

uint16_t XCAM_i2cAddress     = 0x66;

void XCAM_Init(XCAM *pxcam) {
 
    HAL_StatusTypeDef HAL_val;

    uint16_t crc_raw;
    uint8_t  crc1, crc2;

    pxcam->error = BAD_RETURN;

    XCAM_zero_tx_rx (pxcam);
    XCAM_Set_Buffer_Time(pxcam);
    xcam->tx_buffer[0] = 144; // 0x90
    xcam->tx_buffer[1] = 1;
    
    XCAM_get_crc1andcrc2( pxcam->tx_buffer, 23, &crc1 , &crc2 );
    pxcam->tx_buffer[23] = crc1;
    pxcam->tx_buffer[24] = crc2;
    
    HAL_val = XCAM_transmit_receive( pxcam );
    if (HAL_val != HAL_OK) return;

    fprintf(PAYLOAD,
            "\tactual  rx_buffer[0]=%d rx_buffer[1]=%d rx_buffer[2]=%d \r",
                pxcam->rx_buffer[0],pxcam->rx_buffer[1], pxcam->rx_buffer[2]);
    fprintf(PAYLOAD,
            "\tdesired rx_buffer[0]=144 rx_buffer[1]=1 rx_buffer[2]=126\r");


    if(pxcam->rx_buffer[0]==151) {
        XCAM_payload_status_update(pxcam);
    }
    //fprintf(PAYLOAD,"\tdesired that crc values = rx_buffer[3] and rx_buffer[4] \r");
    if ((pxcam->rx_buffer[1] == 1) && (pxcam->rx_buffer[0] == 144)) pxcam->error = AOK;

}


//Set the position 6-9 of the TX_Buffer to be the Current time on the Device
void XCAM_Set_Buffer_Time(XCAM *pxcam){
    unsigned long  ukube_time;
    uint8_t  time1, time2, time3, time4;
    
    
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format); // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
    fprintf(PAYLOAD,"\tXCAM_Init time is %02d-%02d %02d:%02d:%02d\r",
            sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
    
    //Parse the Time
    ukube_time =   (long)sTime.Seconds
                 +((long)sTime.Minutes*60L)
                 +((long)sTime.Hours*3600L)
                 +((long)sDate.Date*24L*3600L)
                 +((long)sDate.Month*30L*24L*3600L);


    time1 = (uint8_t)((ukube_time      ) & 255);
    time2 = (uint8_t)((ukube_time >>  8) & 255);
    time3 = (uint8_t)((ukube_time >> 16) & 255);
    time4 = (uint8_t)((ukube_time >> 24) & 255);


    pxcam->tx_buffer[5] = time4;
    pxcam->tx_buffer[6] = time3;
    pxcam->tx_buffer[7] = time2;
    pxcam->tx_buffer[8] = time1;
    return;
}


//Set And Check parameters for the XCAM
void XCAM_Set_and_Check(XCAM *pxcam, uint8_t   parameter_id, 
                                 uint16_t *pparameter_value){

    XCAM_payload_parameter_update(parameter_id,  pparameter_value, &(pxcam->error));
    osDelay(2);
    XCAM_payload_parameter_read  (parameter_id, &XCAM_check,  &(pxcam->error));
    fprintf(PAYLOAD,"XCAM %d = %d ? \r",*pparameter_value, XCAM_check);
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

}
//Take an XCAm exposure
void XCAM_Exposure(XCAM *pxcam) {
    fprintf(PAYLOAD,"\rInside XCAM_Exposure\r");
    uint16_t XCAM_health;
    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    uint32_t byte1, byte2, byte3, byte4, data_waiting;
    uint32_t data_count, bytes_read, payload_data_position;
    uint32_t bytes_written;
    char filenm[80];
    
    sprintf(filenm,"0:/XCAM.raw");

    pxcam->exposureIsDone = 0;

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


void XCAM_AdjustExposureTime(XCAM *pxcam){
    uint16_t XCAM_check;
    
    XCAM_payload_parameter_update(pxcam, XCAM_INTTIME,  XCAM_inttime);
    osDelay(100);
    XCAM_payload_parameter_read (pxcam, XCAM_INTTIME, &XCAM_check);
    fprintf(PAYLOAD,"XCAM_inttime %d = %d ? \r",XCAM_inttime,XCAM_check);
}

// Type 2 command
void XCAM_payload_parameter_read(XCAM *pxcam, uint8_t   parameter_id, 
                                 uint16_t *pparameter_value)
{
    HAL_StatusTypeDef HAL_val;
    *pparameter_value = 0;
    pxcam->error = BAD_RETURN;
    //*pError           = BAD_RETURN;

    uint8_t crc1, crc2;

    XCAM_zero_tx_rx ( pxcam);

    pxcam->tx_buffer[0] = 148;  // 0x94
    pxcam->tx_buffer[1] = 1;
    pxcam->tx_buffer[2] = parameter_id;

    XCAM_get_crc1andcrc2(&(pxcam->tx_buffer[0]), 3, &crc1 , &crc2);

    pxcam->tx_buffer[3] = crc1;
    pxcam->tx_buffer[4] = crc2;

    HAL_val = XCAM_transmit_receive( pxcam );
    if (HAL_val != HAL_OK) return;

    fprintf(PAYLOAD,"XCAM_payload_parameter_read ");
    if (pxcam->rx_buffer[1] == 9) {

        XCAM_get_crc1andcrc2(&(pxcam->rx_buffer[0]),3,&crc1,&crc2);

        if((crc1 == pxcam->rx_buffer[3]) && (crc2 == pxcam->rx_buffer[4])){
            if( pxcam->rx_buffer[0] == 148 ) {
                fprintf(PAYLOAD,"error9: rx_buffer[1] %d 0x%x\r",
                        pxcam->rx_buffer[1], pxcam->rx_buffer[1]);
                fprintf(PAYLOAD,"error9: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                    pxcam->rx_buffer[2],
                    pxcam->rx_buffer[2]);
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
        XCAM_get_crc1andcrc2(&pxcam->rx_buffer[0],5,&crc1,&crc2);

        if((crc1 == pxcam->rx_buffer[5]) && (crc2 == pxcam->rx_buffer[6])){
            if(pxcam->rx_buffer[0] == 148) {
                if(pxcam->rx_buffer[1] == 1) {
                    pxcam->error = ACKNOWLEDGE_RECEIVED;
                }
                else{
                    fprintf(PAYLOAD,"error: rx_buffer[1] %d 0x%x\r",
                            pxcam->rx_buffer[1], pxcam->rx_buffer[1]);
                    fprintf(PAYLOAD,"error: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                        pxcam->rx_buffer[2],
                        pxcam->rx_buffer[2]);
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

    if ( pxcam->error ==  ACKNOWLEDGE_RECEIVED) {
        uint16_t msbyte = (pxcam->rx_buffer[3] << 8) & 65535;
        uint16_t lsbyte =  pxcam->rx_buffer[4]       & 65535;
        *pparameter_value = msbyte | lsbyte;

        pxcam->tx_buffer[0] = 148;
        pxcam->tx_buffer[1] = 1;
        pxcam->tx_buffer[2] = 126; // 0x7e Acknowledge

        XCAM_get_crc1andcrc2(&pxcam->tx_buffer[0],3,&crc1,&crc2);

        pxcam->tx_buffer[3] = crc1;
        pxcam->tx_buffer[4] = crc2;

        XCAM_transmit(&(pxcam->tx_buffer[0])); // handshake an Acknowledge

        fprintf(PAYLOAD,"good return: ACKNOWLEDGE_RECEIVED\r");
    }

    return;
}

//Update the Parameter in this 
void XCAM_payload_parameter_update(XCAM *pxcam, uint8_t  parameter_id,
                                   uint16_t parameter_value){
    HAL_StatusTypeDef HAL_val;
    pxcam->error = BAD_RETURN;

    uint8_t  crc1, crc2;
    uint8_t  lsbyte, msbyte;

    XCAM_zero_tx_rx ( pxcam );

    pxcam->tx_buffer[0] = 147; // 0x93
    pxcam->tx_buffer[1] = 1;
    pxcam->tx_buffer[2] = parameter_id;

    msbyte = (uint8_t)((parameter_value >> 8) & 255);
    lsbyte = (uint8_t)( parameter_value       & 255);

    pxcam->tx_buffer[3] = msbyte;
    pxcam->tx_buffer[4] = lsbyte;

    XCAM_get_crc1andcrc2(&(pxcam->tx_buffer[0]),5,&crc1,&crc2);

    pxcam->tx_buffer[5] = crc1;
    pxcam->tx_buffer[6] = crc2;

    HAL_val = XCAM_transmit_receive(pxcam);
    if (HAL_val != HAL_OK) return;

    XCAM_get_crc1andcrc2(&(pxcam->rx_buffer[0]),3,&crc1,&crc2);

    fprintf(PAYLOAD,"XCAM_payload_parameter_update ");
    if((crc1 == pxcam->rx_buffer[3]) && (crc2 == pxcam->rx_buffer[4])){
        if( pxcam->rx_buffer[0] == 147 ) {
            if (pxcam->rx_buffer[1] == 1 ) {
                pxcam->error = PARAMETER_UPLOADED;
                fprintf(PAYLOAD,"GOOD: PARAMETER_UPLOADED\r");
            }
            else {
                fprintf(PAYLOAD,"error1: rx_buffer[1] %d 0x%x\r",
                        pxcam->rx_buffer[1], pxcam->rx_buffer[1]);
                fprintf(PAYLOAD,"error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                    pxcam->rx_buffer[2],
                    pxcam->rx_buffer[2]);
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
void XCAM_payload_operations_update(XCAM *pxcam, uint8_t operation_mode)
{
    pxcam->error = BAD_RETURN;


    HAL_StatusTypeDef HAL_val;
    unsigned long  ukube_time;
    uint8_t  time1, time2, time3, time4;
    uint8_t  crc1, crc2;

    XCAM_zero_tx_rx ( pxcam );
    //Set the Time 
    XCAM_Set_Buffer_Time(pxcam);
    tx_buffer[0] = 146; // 0x92
    tx_buffer[1] = 1;
    tx_buffer[2] = operation_mode;
    

    XCAM_get_crc1andcrc2(&tx_buffer[0],23,&crc1,&crc2);
    tx_buffer[23] = crc1;
    tx_buffer[24] = crc2;
    
    
    HAL_val = XCAM_transmit_receive( pxcam);
    if (HAL_val != HAL_OK) return;

    XCAM_get_crc1andcrc2(&(pxcam->rx_buffer[0]),3,&crc1,&crc2);

    fprintf(PAYLOAD,"XCAM_payload_operations_update ");
    if((crc1 == pxcam->rx_buffer[3]) && (crc2 == pxcam->rx_buffer[4])){
        if( pxcam->rx_buffer[0] == 146 ) {
            if (pxcam->rx_buffer[1] == 1 ) {
                pxcam->error = COMMAND_ACCEPTED;
                fprintf(PAYLOAD,"GOOD: COMMAND_ACCEPTED\r");
            }else {
                fprintf(PAYLOAD,"error1: rx_buffer[1] %d 0x%x\r",
                        pxcam->rx_buffer[1], pxcam->rx_buffer[1]);
                fprintf(PAYLOAD,"error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                        pxcam->rx_buffer[2],
                        pxcam->rx_buffer[2]);
            }
        }else {
             fprintf(PAYLOAD,"error1: INVALID_MODE_RETURNED\r");
        }
    }else {
        fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
    }

    return;
}


// This is a Type 2 command
void XCAM_payload_status_update(XCAM *pxcam)
{
    HAL_StatusTypeDef HAL_val;
    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    pxcam->error = BAD_RETURN;
    uint32_t byte1, byte2, data_waiting, payload_flags;
    uint8_t *pstatus_in;
    pstatus_in = &(pxcam->status_buffer);

    int ii;
    uint8_t  crc1, crc2;

    fprintf(PAYLOAD,"XCAM_payload_status_update ");

    XCAM_zero_tx_rx ( pxcam );
    pxcam->tx_buffer[0] = 145; // 0x91
    pxcam->tx_buffer[1] = 1;
    XCAM_get_crc1andcrc2(&(pxcam->tx_buffer),2,&crc1,&crc2);
    pxcam->tx_buffer[2] = crc1;
    pxcam->tx_buffer[3] = crc2;
    HAL_val = XCAM_transmit_receive( pxcam );
    if (HAL_val != HAL_OK) return;


    XCAM_get_crc1andcrc2(&(pxcam->rx_buffer),20,&crc1,&crc2);
    if((crc1 == pxcam->rx_buffer[20]) && (crc2 == pxcam->rx_buffer[21])){
        if( pxcam->rx_buffer[0] == 145 ) {
            if (pxcam->rx_buffer[1] == 1 ) {
                pxcam->error = ACKNOWLEDGE_RECEIVED;
                uint8_t *prx;
                //Point to the Begginging of the Status information
                prx = &(pxcam->rx_buffer[2]);
                //Copy Over buffer
                //ToDO --Check this
                memcpy(pxcam->status_buffer,prx,20);
                //for (ii=2;ii<20;ii++) { pxcam->status_buffer[ii] = *prx++; }
            }
        }else{
             fprintf(PAYLOAD,"error1: INVALID_MODE_RETURNED\r");
        }
    }else {
        fprintf(PAYLOAD,"error: CRC_CHECK_FAILED\r");
    }
    if (pxcam->error !=  ACKNOWLEDGE_RECEIVED){
        //There was an issue in the handshake
        //Lets get out out of there
        pxcam->tx_buffer[0] = 145;
        pxcam->tx_buffer[1] = 9;
        pxcam->tx_buffer[2] = 1;

        XCAM_get_crc1andcrc2(&(pxcam->tx_buffer[0]),3,&crc1,&crc2);
        pxcam->tx_buffer[3] = crc1;
        pxcam->tx_buffer[4] = crc2;

        XCAM_transmit(pxcam);

        fprintf(PAYLOAD,"status error1: rx_buffer[1] %d 0x%x\r",
                pxcam->rx_buffer[1], pxcam->rx_buffer[1]);
        fprintf(PAYLOAD,"status error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
            pxcam->rx_buffer[2],
            pxcam->rx_buffer[2]);
        return;
    }
    //Send an Acknowledgement
    pxcam->tx_buffer[0] = 145;
    pxcam->tx_buffer[1] = 1;
    pxcam->tx_buffer[2] = 126; // 0x7e Acknowledge
    XCAM_get_crc1andcrc2(&(pxcam->tx_buffer[0]),3,&crc1,&crc2);
    pxcam->tx_buffer[3] = crc1;
    pxcam->tx_buffer[4] = crc2;
    XCAM_transmit(pxcam); // handshake an Acknowledge

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    fprintf(PAYLOAD,"GOOD: ACKNOWLEDGE_RECEIVED\r");
    
    byte2 = (((uint32_t)*(pxcam->status_buffer+1)) << 8);
    byte1 = (((uint32_t)*(pxcam->status_buffer+2))     );
    payload_flags = byte2 | byte1;
    data_waiting  = ((uint32_t)*(pxcam->status_buffer+5) <<24) |
                    ((uint32_t)*(pxcam->status_buffer+6) <<16) |
                    ((uint32_t)*(pxcam->status_buffer+7) << 8) |
                    ((uint32_t)*(pxcam->status_buffer+8)     );

    pxcam->status.opMode            = *(pxcam->status_buffer+0);
    pxcam->status.payIsBusy         = (payload_flags &  1)/1;
    pxcam->status.opIsFinished      = (payload_flags &  2)/2;
    pxcam->status.dataIsWaiting     = (payload_flags &  8)/8;
    pxcam->status.opErrorExists     = (payload_flags & 16)/16;
    pxcam->status.opModeErrorExists = (payload_flags & 32)/32;

    XCAM_payload_parameter_read (pxcam, XCAM_OP_ERROR, &XCAM_check);
    pxcam->status.opErrorOutOfMemory   = (XCAM_check==1) ? 1: 0;
    pxcam->status.opErrorAutoExpProb   = (XCAM_check==2) ? 1: 0;
    pxcam->status.opErrorGrabTimeout   = (XCAM_check==3) ? 1: 0;
    pxcam->status.opErrorCISSetupProb  = (XCAM_check==5) ? 1: 0;
    pxcam->status.opErrorCISGrabProb   = (XCAM_check==6) ? 1: 0;
    pxcam->status.opErrorBadParamCombo = (XCAM_check==7) ? 1: 0;

    pxcam->status.data_waiting = data_waiting;

    XCAM_Print_Status(pxcam);
    
    return;
}

//Print the XCAM status
void XCAM_Print_Status(XCAM *pxcam){
    fprintf(PAYLOAD,"\tXCAM status op_mode           is %d -- 0==wakeUp   1==imaging\r",pxcam->status.opMode);
    fprintf(PAYLOAD,"\tXCAM status payIsBusy         is %d -- 1==True\r",pxcam->status.payIsBusy);
    fprintf(PAYLOAD,"\tXCAM status opIsFinished      is %d -- 1==True\r",pxcam->status.opIsFinished);
    fprintf(PAYLOAD,"\tXCAM status dataIsWaiting     is %d -- 1==True\r",pxcam->status.dataIsWaiting);
    fprintf(PAYLOAD,"\tXCAM status opErrorExists     is %d -- 1==True\r",pxcam->status.opErrorExists);
    fprintf(PAYLOAD,"\tXCAM status opModeErrorExists is %d -- 1==True\r",pxcam->status.opModeErrorExists);
    if (pxcam->status.opErrorExists == 1) {
        if(pxcam->status.opErrorOutOfMemory==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 1 -> insufficient memory\r");
        if(pxcam->status.opErrorAutoExpProb==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 2 -> auto exp failed\r");
        if(pxcam->status.opErrorGrabTimeout==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 3 -> grab timeout\r");
        if(pxcam->status.opErrorCISSetupProb==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 5 -> CIS setup failure\r");
        if(pxcam->status.opErrorCISGrabProb==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 6 -> CIS grab failure\r");
        if(pxcam->status.opErrorBadParamCombo==1)
            fprintf(PAYLOAD,"\tXCAM_op_err of 7 -> Invalid parameter combination\r");
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

//Zero out the Buffer of the XCAM device
void XCAM_zero_tx_rx(XCAM *xcam)
{
    int ii;
    
    for (ii=0;ii< RX_BUFFER_LEN;ii++) { xcam->rx_buffer[ii] = 0; }
    for (ii=0;ii< TX_BUFFER_LEN;ii++) { xcam->tx_buffer[ii] = 0; }
    return;
}


void XCAM_get_crc1andcrc2( uint8_t *pBuffer, int length, 
                           uint8_t *pcrc1 , uint8_t *pcrc2 )
{
    uint16_t crc_raw;
    uint8_t crc1 , crc2;
    crc_raw = XCAM_crc16(pBuffer, length);
    crc1    = (uint8_t)((crc_raw >> 8) & 255);
    crc2    = (uint8_t)( crc_raw       & 255);
    *pcrc1  = crc1;
    *pcrc2  = crc2;
    return;
}


uint16_t XCAM_crc16(uint8_t *pBuffer, int length)
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

        data     = (uint16_t) *pBuffer++;
        temp_rs  = crc >> 8;
        temp_xor = temp_rs  ^ data;
        temp     = temp_xor & 255;
        temp_ls  = crc << 8;
        crc      = crc_lut[temp] ^ temp_ls;
    }

    return crc;
}

//Send a Command And Expect a return From the Device
HAL_StatusTypeDef XCAM_transmit_receive(XCAM *pxcam) {
    HAL_StatusTypeDef I2C_retStat;
    I2C_retStat = XCAM_transmit(pxcam);
    if (I2C_retStat != HAL_OK) {
        fprintf(PAYLOAD,"Bad i2c response from XCAM_transmit\r");
        return(I2C_retStat);
    }
    osDelay(3); // documentation says 3ms
    I2C_retStat = XCAM_receive (pxcam);
    if (I2C_retStat != HAL_OK) {
        fprintf(PAYLOAD,"Bad i2c response from XCAM_receive\r");
    }
    return(I2C_retStat);
}

//Send a Command
//Tries to send 10 times with a delay built in
HAL_StatusTypeDef XCAM_transmit(XCAM *pxcam) {
    HAL_StatusTypeDef I2C_retStat;
    uint16_t XCAM_i2cAddress7bit = XCAM_i2cAddress<<1;
    uint32_t timeout        = 10;
    uint8_t i;
    uint8_t tries = 10;

    //Attempt to Transmit a number of times if the HAL_I@C is busy wait for a bit
    for (i=0;i<tries;i++)
    {
        if (hi2c3.State != HAL_I2C_STATE_READY)
        {
            fprintf(PAYLOAD,"hi2c3 not ready\r");
        }

        I2C_retStat = HAL_I2C_Master_Transmit(&hi2c3,
                                              XCAM_i2cAddress7bit,
                                              &(pxcam->tx_buffer), TX_BUFFER_LEN, timeout);
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



    // if there was an issue reset the I2C interface
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

//Recieve a Command
HAL_StatusTypeDef XCAM_receive(XCAM *pxcam) {
    HAL_StatusTypeDef I2C_retStat;
    uint16_t XCAM_i2cAddress7bit = XCAM_i2cAddress<<1;
    uint32_t timeout        = 50;
    
    
    I2C_retStat = HAL_I2C_Master_Receive(&hi2c3,
                                         XCAM_i2cAddress7bit,
                                         &(pxcam->rx_buffer), RX_BUFFER_LEN, timeout);
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
                                         &(pxcam->rx_buffer), RX_BUFFER_LEN, timeout);
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