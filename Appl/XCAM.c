//--------------------
//Mitch's Refactoring of XCAM Code 3/20/22
//----------------------------------------------------------------

#include "Appl/XCAM.h"

// 20201013
/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * XCAM code alice@ucar.edu 2020 09 17
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/*
 * XCAM_Set_Time_in_TX
 * Will set Current time in the first 9 bytes of the TX buffer
 *
 */
void XCAM_Set_Time_in_TX(){

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
    if(DEBUG){
    fprintf(PAYLOAD,"\tXCAM_Init time is %02d-%02d %02d:%02d:%02d\r",
            sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
    }


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


}



/*
 * XCAM_Init Will send Date Time to the XCAM and Check to make sure its responding
 */

void XCAM_Init(uint8_t *pError) {
    //int ii;
    HAL_StatusTypeDef HAL_val;
    XCAM_Set_Time_in_TX();

//+++++Set the CRC Will probably need to be popped out into its own Function. This is a bit Confusing
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

    }
    if ((rx_buffer[1] == 1) && (rx_buffer[0] == 144)) *pError = AOK;

}


/*
 * Sets the Given Register to the Set Value then Checks that what is in that register is what we set. Currently Does not return True for False
 * Return: Bool
 *  True if the set_value is the same as the Check
 */
bool XCAM_Set_And_Check(uint16_t XCAM_register, uint16_t set_value, uint8_t *pError){

    uint16_t XCAM_check;
    XCAM_payload_parameter_update(XCAM_register,  set_value, pError);
    osDelay(2);
    XCAM_payload_parameter_read(XCAM_register, &XCAM_check,  pError);

    if(DEBUG){
        fprintf(PAYLOAD,"XCAM_inttime %d = %d ? \r",set_value,XCAM_check);
    }
    //TODO Do we need a check that things went through Correctly

    //TODO -- Do we need this delay?
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    return (XCAM_Check == set_value);

}

/*
 * Sets the XCAM registers to prepare for an exposure
 *  TODO-ENG If this fails we should Kill the XCAM_Exposure() Func
 */
bool XCAM_Prepare_For_Exposure(uint8_t *pError){

    uint16_t XCAM_health;
    //+++++++++++ Read XCam Health ++++++++++++++++++++
    // XCAM_HEALTH
    // #define XCAM_HEALTH        (4)
    XCAM_payload_parameter_read(XCAM_HEALTH,  &XCAM_health,  pError);
    if(DEBUG){ fprintf(PAYLOAD,"XCAM_health %d 0x%x\r",XCAM_health,XCAM_health);}
    osDelay(100);
    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);


    //++++++++++Update the Registers to prepare for an exposure
    // XCAM_SENSOR
    XCAM_Set_And_Check(XCAM_Register = XCAM_SENSOR ,  set_value = XCAM_sensor, pError = pError);

    // XCAM_TIMEOUT
    XCAM_Set_And_Check(XCAM_Register = XCAM_TIMEOUT ,  set_value = XCAM_timeout, pError = pError);


    //Set And Check The XCAM Window
    XCAM_Set_And_Check(XCAM_Register = XCAM_WINDOW ,  set_value = XCAM_window, pError = pError);


    // XCAM_INTTIME  integration time
    XCAM_Set_And_Check(XCAM_register = XCAM_INTTIME, set_value = XCAM_inttime, pError = pError);


    // XCAM_INITTIMEFRAC fractional part of the integration time
    XCAM_Set_And_Check(XCAM_register = XCAM_INTTIMEFRAC, set_value = XCAM_inttimefrac, pError = pError);


    //Set And Check Auto Exposure
    XCAM_Set_And_Check(XCAM_register = XCAM_AUTOEXP, set_value = XCAM_autoexp, pError = pError);

    //Set and Check Compresion
    XCAM_Set_And_Check(XCAM_register = XCAM_COMPRESS, set_value = XCAM_compress, pError = pError);

    //Set and Check XCAM Thumb
    XCAM_Set_And_Check(XCAM_register = XCAM_THUMB, set_value = XCAM_thumb, pError = pError);

    //Change to Operation Mode
    XCAM_payload_operations_update(XCAM_mode, &XCAM_error);

    //Make Sure Everything Went Smoothly
    if (XCAM_error != AOK ) {
        fprintf(PAYLOAD,"XCAM_payload_operations_update not AOK --> %d 0x%x\r",
                XCAM_error,XCAM_error);
        return False;
    }
    return True;
}


/*
 * initiate A frame Grab to the XCAM
 * Sometimes this take 2 Goes
 * Return: Bool
 * Indicate if it completed Sucessfully (True) or if there was an issue (False)
 *
 */
bool XCAM_Begin_Frame_Grab(uint8_t *pError){


    // XCAM_GRAB
    // #define XCAM_GRAB  (2)
    fprintf(PAYLOAD,"xcam Initiating Grab command\r");

    set_frame_grab_worked = XCAM_Set_And_Check(XCAM_register = XCAM_GRAB, set_value = XCAM_grab, pError = pError);

    // Usually need to initiate the grab command twice.
    if (!set_frame_grab_worked) {
        fprintf(PAYLOAD,"xcam Initiating Grab command try2 \r");
        set_frame_grab_worked = XCAM_Set_And_Check(XCAM_register = XCAM_GRAB, set_value = XCAM_grab, pError = pError);
    }
    if (!set_frame_grab_worked) return False;

    XCAM_payload_status_update(&XCAM_status[0], pError);
    // Wait 5 seconds for the grab execute.
    // TODO it might be possible to make this shorter.
    XCAM_WaitSeconds(5);
    return True;

}

/*----------------------------------------------------------------
 Wait for the Xcam to Take an Exposure and to have the data waiting to be recieveced

 */
uint32_t XCAM_wait_for_data(int sec){

    uint32_t byte1, byte2, byte3, byte4, data_waiting;
    fprintf(PAYLOAD,"\r\rStarting the waiting for SPI data ready status\r");
    int p = 0;
    while (1) {
        XCAM_WaitSeconds(1);
        fprintf(PAYLOAD,"Waiting for SPI data ready status -- %d\r",p);
        p++;
        XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
        if ( (p==Sec)                   // timeout
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
    data_waiting = (byte4 | byte3) | (byte2 | byte1);

    if(DEBUG){
        fprintf(PAYLOAD,"byte4 byte3 byte2 byte1: %ld %ld %ld %ld\r",
            byte4,byte3,byte2,byte1);
    
        fprintf(PAYLOAD,"Number of payload data packets to transfer: %ld\r",
            data_waiting);
    }
    return data_waiting;
}


void XCAM_Write_To_File(uint32_t data_waiting){
    FIL  fid;
    uint16_t btw , bw; //What is this?

    FRESULT fresult = NULL;  // just some value
    char filenm[80];
    uint32_t bytes_written = 0;
    //Todo make this a Global Variable or Setting?
    sprintf(filenm,"0:/XCAM.raw");

    if (writeToFile== 1) {
            fresult = f_open(&fid, filenm,
                         FA_WRITE|FA_READ|FA_OPEN_ALWAYS|FA_OPEN_EXISTING);
        
            //If the File didnt open Correctly
            if( fresult != FR_OK ) { fprintf(PAYLOAD,"**************** %s could not be opened!\r",filenm);}
            else{
             
                fprintf(PAYLOAD,"\t**************** %s open success\r", filenm);
                uint32_t nn=0;
             
                while ( nn <= data_waiting) { // Note! WARNING TODO
                                            // This could turn into an infinte loop
                                            // if the "nn" returned by 
                                            // XCAM_payload_data_transfer are
                                            // not incrementing because the
                                            // data packet is not good.

                    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
                    
                    if((nn % 1000) == 0 ) fprintf(PAYLOAD,"data count so far %ld\r",nn);

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
                fprintf(PAYLOAD,"\t**************** %s closed %ld bytes written\r",
                    filenm,bytes_written);
                f_close(&fid);
            }
        }
}


//This Should be Mostly Good to go but needs to be checked
void XCAM_Exposure(void) {
    fprintf(PAYLOAD,"\rInside XCAM_Exposure\r");

    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    uint32_t data_waiting;
    uint32_t bytes_read;
    


    //Mitch: Dont think this is used
//    RTC_TimeTypeDef sTime;
//    RTC_DateTypeDef sDate;
//    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format); // must be before GetDate
//    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);

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
    
    
    exposureIsDone = 0;



    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);

    if(XCAM_error != AOK) {fprintf(PAYLOAD,"XCAM_Exposure can't read status -- returning\r");}

    if((xstat.dataIsWaiting == 1) || (xstat.data_waiting > 0)) {fprintf(PAYLOAD,"XCAM_Exposure data is already waiting -- returning\r");}


    //Prepare for the Exposure if this returns false Kill the Exposure
    if(!XCAM_Prepare_For_Exposure(&XCAM_error)){return;}


    //Begin the Frame Grab
    if(!XCAM_Begin_Frame_Grab(&XCAM_error)){return;}


    // Start waiting for XCAM to indicate that there is data
    // on the SPI interface.
    //What is going on here?? 
    //Make a timer, wait for 60 Sec add to timer check the status when 60 Second is up parse the Data in the XCAM Status

    //TODO-Eng This secton of code is written a bit odd but is functioning
    //Think it should be as follows:
    //Check Current exposure time.
    //Wait for a minumimum amount of time == To the Exposure time + expected delay
    //Poll XCAM Status after timer is up
    data_waiting = get_data_waiting(sec = 60);
   

    //There is a lot of Stuff happening here as well
    //Write data Waiting To file

    if ( data_waiting != 0 ) {

        // double check for an error
        if (XCAM_status[2] == 146) {
            XCAM_payload_parameter_read  (XCAM_OP_ERROR, &XCAM_check,  &XCAM_error);
            fprintf(PAYLOAD,"XCAM_op_err 0x%x\r",XCAM_check);
        }

        XCAM_WaitSeconds(1);
        

        //Open The File
        XCAM_Write_To_File(data_waiting = data_waiting);
        
        
    }

    //TODO-Question Why is this happening Twice
    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);



    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);

    exposureIsDone = 1;

} // XCAM_Exposure end



void XCAM_AdjustExposureTime()
{
    uint8_t  XCAM_error;
    XCAM_Set_And_Check(XCAM_register = XCAM_INTTIME, set_value= XCAM_inttime, pError = &XCAM_error);
}



// Type 2 command
//Holy Toledo
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
    //If things messed up jump out
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
//This is Chunky! Could probably be slimmed down quite a bit
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


void XCAM_WaitSeconds(int numSeconds) {
    int ii,jj;
    if(DEBUG>1) fprintf(PAYLOAD,"Waiting %d seconds...",numSeconds);

    TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
    for (ii=0;ii<numSeconds;ii++) {
        for (jj=0;jj<10;jj++) {
            osDelay(100); // in ms
            TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
        }

        if(DEBUG > 1) fprintf(PAYLOAD,"--%d--",ii);
    }
    if(DEBUG > 1) fprintf(PAYLOAD,"\r");
}

// 20201013 end
