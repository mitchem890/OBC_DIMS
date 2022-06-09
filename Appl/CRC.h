#pragma once
#include "stm32f4xx_hal.h"
#include "MCU_Init.h"
#include <stdbool.h>

bool XCAM_ValidateCRC(uint8_t* data, size_t len);
uint16_t XCAM_crc16(uint16_t seed, uint8_t *pBuffer, int length);
void XCAM_SetCRC(uint8_t* data, size_t len);
