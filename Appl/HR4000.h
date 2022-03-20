/*
 * HR4000.h
 *
 *  Created on: Dec 29, 2020
 *      Author: damonb
 */

#ifndef INC_HR4000_H_
#define INC_HR4000_H_

#include "defs.h"

struct sHR4000
{
	UART_HandleTypeDef *UART;
  uint16_t IntegrationTime_ms;
  uint16_t PreviousIntegrationTime_ms;
	uint8_t Summing;
	uint8_t Smoothing;
	bool Checksum;
};

struct sHR4000 HR_InitStruct(void);

struct sspectra
{
	uint8_t DataSize;
	uint8_t ScanCount;
	uint32_t IntegrationTime_us;
	uint8_t PixelMode;
	uint32_t Data[3840];
	uint32_t ExtraBytes[3840];
	uint16_t ExtraByteCount;
	uint8_t DataHeader[14];
};

void HR_ClearBuffer(UART_HandleTypeDef *huart);
uint8_t HR_SetBinaryMode(UART_HandleTypeDef *huart);
uint8_t HR_SendCommand(UART_HandleTypeDef *huart, uint8_t *command, size_t length, uint16_t wait_time);
uint8_t HR_SetIntegrationTime(UART_HandleTypeDef *huart, uint16_t time_ms);
uint8_t HR_SetSumming(UART_HandleTypeDef *huart, uint8_t count);
uint8_t HR_SetSmoothing(UART_HandleTypeDef *huart, uint8_t count);
uint8_t HR_ClearMemory(UART_HandleTypeDef *huart);
uint8_t HR_SetChecksumMode(UART_HandleTypeDef *huart, bool state);
uint8_t HR_GetSpectra(struct sHR4000* SR, struct sspectra* Spectra);
uint8_t HR_SetTriggerMode(UART_HandleTypeDef *huart, uint8_t mode);
void HD_AnalyzeSpectra(struct sspectra *spectra);

#endif /* INC_HR4000_H_ */
