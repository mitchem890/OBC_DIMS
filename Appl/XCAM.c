/*
Damon's Reference XCAM Implementation
2021-08-24
*/
#include "XCAM.h"
#include "TaskMonitor.h"


#define XCAM_ADDRESS (0x66)
#define XCAM_DEBUG (1)
#define XCAM_HALOK (0)
#define XCAM_RAWSTATUS (0)


void XCAM_Init(XCAM_device XCAM_device){
    //  The start-up sequence for C3D is as follows:
    //1) Power on the 3V3 line to initialise the I2C buffer
    //2) Power on the 5V line to boot the FPGA
    //3) Send the payload a ‘payload initialise command’ to complete the start-up sequence. This will boot the remaining peripheral devices.
    XCAM_Toggle_Power();
    XCAM_device.xret = BAD_RETURN;
    if (XCAM_SendInitCommand(XCAM_device)){return 1;}
    //4) Leave for five seconds to allow the secondary CPU to boot
    XCAM_WaitSeconds(5, true);
    // Need to send a new init command.
    if (XCAM_SendInitCommand(XCAM_device)){return 2;}
    return 0;
}


uint8_t XCAM_SendInitCommand(XCAM_device){
  return XCAM_SendInitOrUpdate(XCAM_device, true, false);
}


uint8_t XCAM_SendInitOrUpdate(XCAM_device XCAM_device, bool init, bool imagingmode)
{
  uint8_t txbuf[25] = {0};
  uint8_t rxbuf[5] = {0};
  unsigned long ukube_time;


  //Setup the Buffers we are going to transmit
  if (init)
  {
    if (XCAM_DEBUG)
      fprintf(PAYLOAD, "Sending Init Command");
    XCAM_device.tx_buffer[0] = 0x90;
  }
  else
  {
    if (XCAM_DEBUG)
      fprintf(PAYLOAD, "Sending Update Command");
    XCAM_device.tx_buffer[0] = 0x92;
  }
  txbuf[1] = 1;
  if (imagingmode)
  {
    if (XCAM_DEBUG)
      fprintf(PAYLOAD, " and setting imaging mode.\r");
    XCAM_device.tx_buffer[2] = 1;
  }
  else
  {
    if (XCAM_DEBUG)
      fprintf(PAYLOAD, " and setting standby mode.\r");
    XCAM_device.tx_buffer[2] = 0;
  }


  // Get the Current time and assign them to the buffers
  XCAM_Get_UKube_DateTime(&ukube_time);

  XCAM_device.tx_buffer[5] = (uint8_t)((ukube_time >> 24) & 0xff);
  XCAM_device.tx_buffer[6] = (uint8_t)((ukube_time >> 16) & 0xff);
  XCAM_device.tx_buffer[7] = (uint8_t)((ukube_time >>  8) & 0xff);
  XCAM_device.tx_buffer[8] = (uint8_t)((ukube_time      ) & 0xff);

  //Get the CRC for the message
  XCAM_SetCRC(XCAM_device.tx_buffer, 25);

  //Attempt to communicate with the Device
  if (XCAM_multiattempt_transmit(XCAM_device.tx_buffer))
    return 1;
  osDelay(3);
  if (XCAM_receive(XCAM_device.rx_buffer, nbytes = 5, ack = false))
    return 2;

  XCAM_PrintACKOrResponse(XCAM_device.rx_buffer, 5);
  return 0;
}






//Working through
void XCAM_Exposure(XCAM_device XCAM_device) {
    fprintf(PAYLOAD,"\rInside XCAM_Exposure\r");
    uint16_t XCAM_health;
    uint8_t  XCAM_error;
    uint16_t XCAM_check;
    uint32_t byte1, byte2, byte3, byte4, data_waiting;
    uint32_t data_count, bytes_read, payload_data_position;
    uint32_t bytes_written;
    char filenm[80];


    //ToDo make this into its own Function
    //Not Needed????
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

    XCAM.exposureIsDone = 0;

    FIL  fid;
    UINT btw , bw;
    FRESULT fresult;
    bytes_written = 0;
    //Working Through
    XCAM_payload_status_update(&XCAM_status[0], &XCAM_error);
    if(XCAM_error != AOK) {
        fprintf(PAYLOAD,"XCAM_Exposure can't read status -- returning\r");
    }
    if((XCAM.xstat.dataIsWaiting == 1) || (xstat.data_waiting > 0)) {
        fprintf(PAYLOAD,"XCAM_Exposure data is already waiting -- returning\r");
    }


    //Update the Payload parameters
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


//Get the UKube Time format and Store in the address given
void XCAM_Get_UKube_DateTime(unsigned long *ukube_time){
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc,&sTime,calendar_format); // must be before GetDate
    HAL_RTC_GetDate(&hrtc,&sDate,calendar_format);
    fprintf(PAYLOAD,"Setting XCAM time to %02d-%02d %02d:%02d:%02d\r",
            sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);

    ukube_time =   (long)sTime.Seconds
                 +((long)sTime.Minutes*60L)
                 +((long)sTime.Hours*3600L)
                 +((long)sDate.Date*24L*3600L)
                 +((long)sDate.Month*30L*24L*3600L);
    return;

}

// This is a Type 2 command
//Needs to be simplified

void XCAM_payload_status_update(XCAM_device XCAM, uint8_t *pstatus, uint8_t *pError)
{

    //Set up
    uint8_t  XCAM_error;//Same as pError
    uint16_t XCAM_check;
    *XCAM.xret = BAD_RETURN;
    uint32_t byte1, byte2, data_waiting, payload_flags;
    uint8_t *pstatus_in;
    pstatus_in = pstatus;

    int ii;
    uint8_t  crc1, crc2;

    fprintf(PAYLOAD,"XCAM_payload_status_update ");


    //Zero out all the Buffers
    XCAM_zero_tx_rx ( &XCAM.tx_buffer[0] , &XCAM.rx_buffer[0] );

    XCAM.tx_buffer[0] = 145; // 0x91
    XCAM.tx_buffer[1] = 1;

    //Get the CRC for the Command
    XCAM_get_crc1andcrc2(&XCAM.tx_buffer[0],2,&crc1,&crc2);
    XCAM.tx_buffer[2] = crc1;
    XCAM.tx_buffer[3] = crc2;


    //Send and request the Payload status
    XCAM.Halstat = XCAM_transmit_receive( &XCAM.tx_buffer[0] , &XCAM.rx_buffer[0]);
    if (XCAM.Halstat != HAL_OK) return;


    //Get the CRC for the recieved info
    XCAM_get_crc1andcrc2(&XCAM.rx_buffer[0],20,&crc1,&crc2);


   //Check if the Recieved info matched the calculated CRC
    if((crc1 == XCAM.rx_buffer[20]) && (crc2 == XCAM.rx_buffer[21])){
        if( XCAM.rx_buffer[0] == 145 ) {
            if (rx_buffer[1] == 1 ) {
                *XCAM.xret = ACKNOWLEDGE_RECEIVED;
                // copy the receive buffer to pstatus
                // status = rx_buffer[2:19]
                uint8_t *prx;
                prx = &rx_buffer[2];
                //Set the Status buffer to the Same as the RX buffer
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

       //If things went well
    if ( *XCAM.xret ==  ACKNOWLEDGE_RECEIVED) {

        XCAM.tx_buffer[0] = 145;
        XCAM.tx_buffer[1] = 1;
        XCAM.tx_buffer[2] = 126; // 0x7e Acknowledge

        XCAM_get_crc1andcrc2(&XCAM.tx_buffer[0],3,&crc1,&crc2);
        XCAM.tx_buffer[3] = crc1;
        XCAM.tx_buffer[4] = crc2;

        //Send Ack to XCAM to say things were recieved
        XCAM_transmit(&XCAM.tx_buffer[0]); // handshake an Acknowledge

        //Tell the TaskManager we are still working on things
        TaskMonitor_IamAlive(TASK_MONITOR_DEFAULT);
        fprintf(PAYLOAD,"GOOD: ACKNOWLEDGE_RECEIVED\r");
        pstatus = pstatus_in;

        XCAM_Parse_Data(XCAM_Device, );
        XCAM_Pasre_Flags();
        XCAM_payload_parameter_read (XCAM_OP_ERROR, &XCAM_check,  &XCAM_error);
        XCAM.xstat.opErrorOutOfMemory   = (XCAM_check==1) ? 1: 0;
        XCAM.xstat.opErrorAutoExpProb   = (XCAM_check==2) ? 1: 0;
        XCAM.xstat.opErrorGrabTimeout   = (XCAM_check==3) ? 1: 0;
        XCAM.xstat.opErrorCISSetupProb  = (XCAM_check==5) ? 1: 0;
        XCAM.xstat.opErrorCISGrabProb   = (XCAM_check==6) ? 1: 0;
        XCAM.xstat.opErrorBadParamCombo = (XCAM_check==7) ? 1: 0;

        XCAM.xstat.data_waiting = data_waiting;

        XCAM_Display_Status();


    }
    //If we didnt recieve an Acknowledge
    else {

        XCAM.tx_buffer[0] = 145;
        XCAM.tx_buffer[1] = 9;
        XCAM.tx_buffer[2] = 1;

        XCAM_get_crc1andcrc2(&XCAM.tx_buffer[0],3,&crc1,&crc2);

        XCAM.tx_buffer[3] = crc1;
        XCAM.tx_buffer[4] = crc2;

        XCAM_transmit(&XCAM.tx_buffer[0]);

        fprintf(PAYLOAD,"status error1: rx_buffer[1] %d 0x%x\r",
                XCAM.rx_buffer[1], XCAM.rx_buffer[1]);
        fprintf(PAYLOAD,"status error1: PAYLOAD_ERROR_CONDITION %d 0x%x\r",
                XCAM.rx_buffer[2],
                XCAM.rx_buffer[2]);
    }

    return;
}

//Parse Payload Status
void XCAM_Parse_Payload_Status(XCAM_device XCAM,uint8_t pstatus,){
    byte2 = (((uint32_t)*(pstatus+1)) << 8);
    byte1 = (((uint32_t)*(pstatus+2))     );
    payload_flags = byte2 | byte1;
    data_waiting  = ((uint32_t)*(pstatus+5) <<24) |
                    ((uint32_t)*(pstatus+6) <<16) |
                    ((uint32_t)*(pstatus+7) << 8) |
                    ((uint32_t)*(pstatus+8)     );

    XCAM.xstat.opMode            = *(pstatus+0);
    XCAM_Set_Flags(XCAM, payload_flags);
    XCAM.xstat.opMode            = *(pstatus+0);

}



void XCAM_Set_Flags(XCAM_device XCAM, payload_flags){

    XCAM.xstat.payIsBusy         = (payload_flags &  1)/1;
    XCAM.xstat.opIsFinished      = (payload_flags &  2)/2;
    XCAM.xstat.dataIsWaiting     = (payload_flags &  8)/8;
    XCAM.xstat.opErrorExists     = (payload_flags & 16)/16;
    XCAM.xstat.opModeErrorExists = (payload_flags & 32)/32;
}



void XCAM_Display_Status(){
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
}


//Do I really Need this
HAL_StatusTypeDef XCAM_transmit_receive(uint8_t *ptx_buffer, uint8_t *prx_buffer) {
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


//Should be a general Util
void XCAM_zero_tx_rx( XCAM_Device XCAM)
{
    int ii;
    for (ii=0;ii< 30;ii++) { *XCAM.tx_buffers++ = 0; }
    for (ii=0;ii<260;ii++) { *XCAM.rx_buffers++ = 0; }
    return;
}

/*
 *
 * All of these Should be turned into a general Util
 *
 *
 */
// Should Be a general Util


uint8_t XCAM_receive(XCAM_device XCAM, size_t nbytes = 30, bool ack = false)
{
  XCAM.Halstat = HAL_ERROR;
  uint8_t buf[5] = {0};

  if (nbytes > 0)
  {
    XCAM.Halstat = HAL_I2C_Master_Receive(&hi2c3, XCAM_ADDRESS << 1,
                                 XCAM.rx_buffer, nbytes, 100);
    if ((XCAM.Halstat == HAL_OK) && XCAM_HALOK)
      fprintf(PAYLOAD,"\tXCAM_receive return was HAL_OK\r");

    if (XCAM.Halstat != HAL_OK){

      fprintf(PAYLOAD,"\tXCAM_receive return was NOT HAL_OK\r");
      I2C_Reset(&hi2c3);
      osDelay(5);
      return XCAM.Halstat;
    }

    // there's a bug in the camera firmware where a command 0x95 returns a 0x91 as the first byte
    if ((nbytes == 260) && (XCAM.rx_buffer[0] = 0x91))
        XCAM.rx_buffer[0] = 0x95;
    if (XCAM_ValidateCRC(XCAM.rx_buffer, nbytes) == false)
      fprintf(PAYLOAD, "WARNING: response failed CRC check\r");
  }

  if (ack) // if we need to send an acknowledgment packet
  {
    buf[0] = XCAM.rx_buffer[0];
    buf[1] = 1;
    buf[2] = 0x7E;

    XCAM_SetCRC(buf, 5);
    XCAM.Halstat = HAL_I2C_Master_Transmit(&hi2c3, XCAM_ADDRESS << 1,
                                  buf, 5, 100);
    if ((XCAM.Halstat == HAL_OK) && XCAM_HALOK)
      fprintf(PAYLOAD,"\tACK return was HAL_OK\r");
    if (XCAM.Halstat != HAL_OK)
    {
      fprintf(PAYLOAD,"\tACK return was NOT HAL_OK\r");
      I2C_Reset(&hi2c3);
      osDelay(5);
      return 2;
    }
  }
  return 0;
}


//Print the Acknowledgement or the Rx_buffer
void XCAM_PrintACKOrResponse(uint8_t *buffer , size_t len){

    uint16_t i;
    if ((len == 5) && (buffer[2] == 0x7e)){
        fprintf(PAYLOAD, "Response: ACK\r");
    }
    else{

        fprintf(PAYLOAD, "Response: 0x");
        for (i=0; i<len; i++){
            fprintf(PAYLOAD, "%02x ", XCAM.rx_buffer[i]);
        }
        fprintf(PAYLOAD, "\r");
  }
}


//Attempt to Send tx i2c transmission multiple times. If it continually fails reset the i2C state
HAL_StatusTypeDef XCAM_multiattempt_transmit(XCAM_device XCAM, size_t nbytes = 30, int attempts = 10){


    for(i=0;i<attempts;i++){
        if (hi2c3.State != HAL_I2C_STATE_READY){

                fprintf(PAYLOAD,"hi2c3 not ready\r");

        }
        //pass to XCAM_Tansmit
        XCAM.Halstat = XCAM_transmit(XCAM, nbytes);
        //If Transmission went through end
        if (XCAM.Halstat == HAL_OK)
            break;

        //Print Issue With Transmission
        //TODO Remove before Final Product
        fprintf(PAYLOAD,"Failed to XCAM Transmit, ERROR %d \r",XCAM.Halstat);
        osDelay(1000);

        fprintf(PAYLOAD,"\tXCAM_transmit I2C_Reset\r");
        I2C_Reset(&hi2c3);
        osDelay(5);
    }
    //Reset the I2C State
    if((  XCAM.Halstat != HAL_OK )
       &&(hi2c3.State == HAL_I2C_STATE_READY)){

        fprintf(PAYLOAD,"\tXCAM_transmit I2C_Reset\r");
        I2C_Reset(&hi2c3);
        osDelay(5);
    }
    return XCAM.Halstat;
}

//Transmit Data in XCAM_devices TX buffer
HAL_StatusTypeDef XCAM_transmit(XCAM_device XCAM, uint8_t *buffer, size_t nbytes = 30)
{
  XCAM.Halstat = HAL_ERROR;
  XCAM.Halstat = HAL_I2C_Master_Transmit(&hi2c3, XCAM_ADDRESS << 1,
                                XCAM.tx_buffer, nbytes, 10);
  if ((XCAM.Halstat == HAL_OK) && XCAM_HALOK)
    fprintf(PAYLOAD,"\tXCAM_transmit return was HAL_OK\r");
  if (XCAM.Halstat != HAL_OK)
  {
    fprintf(PAYLOAD,"\tXCAM_transmit return was NOT HAL_OK\r");
    return XCAM.Halstat;
  }
  return 0;
}



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

void XCAM_Toggle_Power(){
    fprintf(PAYLOAD,"\r\tTurning off LUP in AppTasks\r");
    EPS_write(6,1);  //  --> turn off LUP 5v
    XCAM_WaitSeconds(1,0);
    EPS_write(5,1);  //  --> turn off LUP 3.3v
    // wait for everything to settle
    XCAM_WaitSeconds(30,1);
    EPS_check(0,1);

    fprintf(PAYLOAD,"\r\tTurning on  LUP in AppTasks\r");
    EPS_write(5,0);  //  --> turn  on LUP 3.3v
    XCAM_WaitSeconds(5,1);
    EPS_write(6,0);  //  --> turn  on LUP 5v
    XCAM_WaitSeconds(5,1);
    EPS_check(0,1);

}
