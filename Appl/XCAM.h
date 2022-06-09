/*
Mitch's XCAM Implementation
2022-01-18

*/

#pragma once
#include "stm32f4xx_hal.h"
#include stdint.h
#include "MCU_Init.h"
#include <stdbool.h>
#include "CRC.h"

void XCAM_Set_Time_in_TX();
void XCAM_Init(uint8_t *pError);
bool XCAM_Set_And_Check(uint16_t XCAM_register, uint16_t set_value, uint8_t *pError);
bool XCAM_Prepare_For_Exposure(uint8_t *pError);
bool XCAM_Begin_Frame_Grab(uint8_t *pError);
uint32_t XCAM_wait_for_data(int sec);
void XCAM_Write_To_File(uint32_t data_waiting);
void XCAM_Exposure(void);
void XCAM_AdjustExposureTime();

bool XCAM_Set_And_Check(uint16_t XCAM_register, uint16_t set_value, uint8_t *pError);
void XCAM_payload_parameter_read(uint8_t   parameter_id, 
                                 uint16_t *pparameter_value,
                                 uint8_t  *pError);
void XCAM_payload_parameter_update(uint8_t  parameter_id,
                                    uint16_t parameter_value,
                                    uint8_t  *pError);
void XCAM_payload_operations_update(uint8_t operation_mode, uint8_t  *pError);
void XCAM_payload_status_update(uint8_t *pstatus, uint8_t *pError);
void XCAM_payload_data_transfer(uint8_t *ppayload_data, uint8_t *pError, uint32_t *whichOne);
void XCAM_zero_tx_rx( uint8_t *ptx , uint8_t *prx);
void XCAM_get_crc1andcrc2( uint8_t *pBuf , int length, 
                           uint8_t *pcrc1 , uint8_t *pcrc2 );

uint16_t XCAM_crc16(uint16_t seed, uint8_t *pBuffer, int length);
HAL_StatusTypeDef XCAM_transmit_receive(uint8_t *ptx_buffer,uint8_t *prx_buffer);
HAL_StatusTypeDef XCAM_transmit(uint8_t *ptx_buffer);
HAL_StatusTypeDef XCAM_receive(uint8_t *prx_buffer);
void XCAM_WaitSeconds(int numSeconds);
