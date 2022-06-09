/*
Damon's Reference XCAM Implementation
2021-08-24
*/

#include "stm32f4xx_hal.h"
#include "MCU_Init.h"
#include <stdbool.h>

void D_XCAM_Example(void);
uint8_t D_XCAM_Init(void);
uint8_t D_XCAM_GetStatus(uint8_t *status);
uint16_t D_XCAM_AnalyzeStatus(uint8_t *status);
uint8_t D_XCAM_GetImageSPI(uint8_t *buffer);
uint8_t D_XCAM_GetImageI2C(uint8_t *buffer);
uint8_t D_XCAM_SetParameter(uint8_t ID, uint16_t value);
void D_XCAM_SetCRC(uint8_t* data, size_t len);
bool D_XCAM_ValidateCRC(uint8_t* data, size_t len);
uint16_t D_XCAM_crc16(uint16_t seed, uint8_t *pBuffer, int length);
uint8_t D_XCAM_transmit(uint8_t *buffer, size_t len);
uint8_t D_XCAM_receive(uint8_t *buffer, size_t len, bool ack);
void D_XCAM_WaitSeconds(uint16_t numSeconds, bool printit);

uint8_t D_XCAM_SendInitCommand(void);
uint8_t D_XCAM_EnableImagingMode(void);
uint8_t D_XCAM_SendInitOrUpdate(bool init, bool imagingmode);
void D_XCAM_PrintACKOrResponse(uint8_t *buffer, size_t len);
