/*
 * HR4000.c
 *
 *  Created on: Dec 29, 2020
 *      Author: damonb
 */

#include "defs.h"
#include "HR4000.h"
#define vdebug true

// "In binary mode alldata, except where noted, passes as 16-bit unsigned integers (WORDs)
// with the MSB followed by the LSB."

struct sHR4000 HR_InitStruct(void)
{
  struct sHR4000 HR4000;
  HR4000.UART = &huart6;
  HR4000.IntegrationTime_ms = 100;
  HR4000.PreviousIntegrationTime_ms = 65000;
  HR4000.Summing = 1;
  HR4000.Smoothing = 0;
  HR4000.Checksum = false;
  return HR4000;
}

void HR_ClearBuffer(UART_HandleTypeDef *huart)
{
  uint8_t result = 1;
  uint8_t buffer[20];
  if (vdebug) printf("Clearing UART buffer....\n");
  while (result)
    result = !(HAL_UART_Receive(huart, buffer, 20, 3));
}

uint8_t HR_SetBinaryMode(UART_HandleTypeDef *huart)
{
  uint8_t resp[2];
  uint8_t command[2] = { 0 };
  command[0] = 'b';
  command[1] = 'B';
  if (vdebug) printf("Setting binary mode.");

  HAL_StatusTypeDef result;
  result = HAL_UART_Transmit(huart, command, 2, 10);
  if (result != 0)
    return 40 + result;
  result = HAL_UART_Receive(huart, resp, 2, 100);
  if (result != 0)
  {
    if (result == HAL_TIMEOUT)
    {
      if (vdebug) printf(" - TIMEOUT.\n");
      return 80 + result;
    }
  }
  if ((resp[1] == 2) || (resp[1] == 6))
  {
    if (vdebug) printf(" - ACK.\n");
    return 0;
  }
  if ((resp[1] == 3) || (resp[1] == 21))
  {
    if (vdebug) printf(" - NACK.\n");
    return 1;
  }
  return 1; // shouldn't reach this
}

uint8_t HR_SendCommand(UART_HandleTypeDef *huart, uint8_t *command, size_t length, uint16_t wait_time)
{
  uint8_t resp = 0;
  HAL_StatusTypeDef result;
  if (length < 1)
    return 9;
  result = HAL_UART_Transmit(huart, command, length, 10);
  if (result != 0)
    return 40 + result;
  result = HAL_UART_Receive(huart, &resp, 1, wait_time);
  if (result != 0)
  {
    if (result == HAL_TIMEOUT)
    {
      if (vdebug) printf(" - TIMEOUT.\n");
      return 80 + result;
    }
  }
  if ((resp == 2) || (resp == 6))
  {
    if (vdebug) printf(" - ACK.\n");
    return 0;
  }
  if ((resp == 3) || (resp == 21))
  {
    if (vdebug) printf(" - NACK.\n");
    return 1;
  }
  return 1; // shouldn't reach this
}

uint8_t HR_SetIntegrationTime(UART_HandleTypeDef *huart, uint16_t time_ms)
{
  if (time_ms > 65000)
    return 1;
  uint8_t command[3] = { 0 };

  command[0] = 'I';
  command[1] = (time_ms >> 8) & 0xff;
  command[2] = time_ms & 0xff;
  if (vdebug) printf("Setting integration time to %u.", time_ms);
  return HR_SendCommand(huart, command, 3, 65000);
}

uint8_t HR_SetSumming(UART_HandleTypeDef *huart, uint8_t count)
{
  if ((count < 1) | (count > 4))
    return 1;
  uint8_t command[3] = { 0 };
  command[0] = 'A';
  command[1] = 0;
  command[2] = count;
  if (vdebug) printf("Setting scan summation to %u.", count);
  return HR_SendCommand(huart, command, 3, 10);
}

uint8_t HR_SetSmoothing(UART_HandleTypeDef *huart, uint8_t count)
{
  if (count > 15)
    return 1;
  uint8_t command[3] = { 0 };
  command[0] = 'B';
  command[1] = 0;
  command[2] = count;
  if (vdebug) printf("Setting smoothing to %u.", count);
  return HR_SendCommand(huart, command, 3, 10);
}

uint8_t HR_ClearMemory(UART_HandleTypeDef *huart)
{
  uint8_t command[3] = { 0 };
  command[0] = 'L';
  // datasheet says L followed by either a 0 or 1, which do the same thing
  // however, device sends ACK immediately following L, so we send one byte
  if (vdebug) printf("Clearing HR4000 memory.");
  return HR_SendCommand(huart, command, 1, 10);
}

uint8_t HR_SetTriggerMode(UART_HandleTypeDef *huart, uint8_t mode)
{
  if (mode > 3)
    return 1;
  uint8_t command[3] = { 0 };
  command[0] = 'T';
  command[1] = 0;
  command[2] = mode;
  if (vdebug) printf("Setting trigger mode to %u.", mode);
  return HR_SendCommand(huart, command, 3, 10);
}

uint8_t HR_SetChecksumMode(UART_HandleTypeDef *huart, bool state)
{
  uint8_t command[3] = { 0 };
  command[0] = 'k';
  command[1] = 0;
  command[2] = (state == true); // probably only need = state;
  return HR_SendCommand(huart, command, 3, 10);
}

uint8_t HR_GetSpectra(struct sHR4000 *SR, struct sspectra *Spectra)
{
  uint16_t i;
  uint8_t command[1] = { 0 };
  uint8_t buffer[8000] = { 0 };
  HAL_StatusTypeDef result;

  for (i=0;i<3840;i++)
  {
    Spectra->Data[i]=0;
    Spectra->ExtraBytes[i]=0;
  }
  Spectra->ExtraByteCount = 0;


  HR_ClearBuffer(SR->UART);
  if (HR_SetBinaryMode(SR->UART))
    return 1;
  if (HR_SetTriggerMode(SR->UART, 1))
    return 1;
  if (SR->IntegrationTime_ms != SR->PreviousIntegrationTime_ms)
  {
    if (HR_SetIntegrationTime(SR->UART, SR->IntegrationTime_ms))
      return 1;
    SR->PreviousIntegrationTime_ms = SR->IntegrationTime_ms;
    if (HR_SetTriggerMode(SR->UART, 0))
      return 1;
    command[0] = 'S';
    if (vdebug) printf("Waiting for old spectra.");
    if (HR_SendCommand(SR->UART, command, 1, 10))
      return 1;
    // we probably need to keep track of previous summing, hopefully we just don't need it
    result = HAL_UART_Receive(SR->UART, buffer, 14, 65000);
    if (result != 0)
    {
      return 1;
    }
    while (!(HAL_UART_Receive(SR->UART, buffer, 1, 80)));
  }

  if (HR_SetSmoothing(SR->UART, SR->Smoothing))
    return 1;
  if (HR_SetSumming(SR->UART, SR->Summing))
    return 1;
//	if (HR_SetChecksumMode(SR->UART, SR->Checksum))
//			return 1;
//  HR_ClearMemory(SR->UART); // I don't think this does anything.

//  if (HR_SetTriggerMode(SR->UART, 0))
//    return 1;

  command[0] = 'S';
  printf("Requesting spectra.");
  if (HR_SendCommand(SR->UART, command, 1, 10))
    return 1;

  result = HAL_UART_Receive(SR->UART, buffer, 14, 9000 + SR->IntegrationTime_ms * SR->Summing);
  if (result != 0)
  {
    if (result == HAL_TIMEOUT)
      printf("TIMEOUT.\n");
    return 1;
  }

  for (i = 0; i < 14; i++)
    Spectra->DataHeader[i] = buffer[i];

  /*
   WORD 0xFFFF – start of spectrum
   WORD Spectral Data Size Flag (0 → Data is WORD’s, 1 → Data is DWORD’s)
   WORD scan number ALWAYS 0
   WORD Number of scans accumulated together
   DWORD integration time in microseconds (LSW followed by MSW)
   WORD pixel mode
   WORDs if pixel mode not 0, indicates parameters passed to the Pixel Mode command (P)
   (D)WORDs spectral data – see Data Size Flag for variable size
   WORD 0xFFFD – end of spectrum
   */

  if (((buffer[0] << 8) | buffer[1]) != 0xffff)
  {
    printf("Invalid data start.\n");
    return 1;
  }
  else
    printf("Valid data start.\n");

  if (((buffer[3] << 8) | buffer[4]) == 0)
    Spectra->DataSize = 16;
  else if (((buffer[3] << 8) | buffer[4]) == 1)
    Spectra->DataSize = 32;
  else
  {
    printf("Invalid data size.\n");
    return 1;
  }

  Spectra->ScanCount = (uint8_t) ((buffer[6] << 8) | buffer[7]);

  // datasheet claims format is LSW(MSB LSB) MSW(MSB LSB) but format is really MSW(MSB LSB) LSW(MSB LSB)
  Spectra->IntegrationTime_us = (buffer[8] << 24) | (buffer[9] << 16) | (buffer[10] << 8) | buffer[11];

  if (HAL_UART_Receive(SR->UART, buffer, 3840 * Spectra->DataSize / 8 + 2, 800))
    return 1;

  if (Spectra->DataSize == 16)
    for (i = 0; i < 3840; i++)
      Spectra->Data[i] = (buffer[2 * i] << 8) | buffer[2 * i + 1];
  else
    for (i = 0; i < 3840; i++)
      Spectra->Data[i] = (buffer[4 * i] << 24) | (buffer[4 * i + 1] << 16) | (buffer[4 * i + 2] << 8)
          | buffer[4 * i + 3];

  printf("Spectra received, scan count %u, integration time %lu us, data size %u.\n", Spectra->ScanCount,
      Spectra->IntegrationTime_us, Spectra->DataSize);

  i = 0;
  // extra data is returned, typically all the same value, datasheet doesn't explain why
  while ((!(HAL_UART_Receive(SR->UART, buffer, 1, 80))) && (i < 2000))
  {
    Spectra->ExtraBytes[i] = buffer[0];
    i = i + 1;
  }
  printf("Received %u extra bytes for an unknown reason.\n", i);
  Spectra->ExtraByteCount = i;

  return 0;
}

void HD_AnalyzeSpectra(struct sspectra *spectra)
{
  uint32_t dsum = 0;
  uint16_t i;
  uint32_t dmax = 0;
  uint32_t dmin = 0xffffffff;

  for (i=0;i<3840;i++)
  {
    dsum += spectra->Data[i];
    if (spectra->Data[i] < dmin)
      dmin = spectra->Data[i];
    if (spectra->Data[i] > dmax)
      dmax = spectra->Data[i];
  }
  printf("Spectra min %lu, max %lu, avg %lu.\n", dmin, dmax, dsum/3840);
}
