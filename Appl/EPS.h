
#include "main.h"
#include "fatfs.h"



void EPS_check(int printToFile, int printToPAYLOAD);
void EPS_read (uint16_t Cmd,  long *pEpsValue);
void EPS_write(uint16_t Addr, uint8_t Value);