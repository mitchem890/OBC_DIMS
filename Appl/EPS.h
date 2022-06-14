#include "stm32f4xx_hal.h"
#include "MCU_Init.h"
#include <stdbool.h>
#include "AppTasks.h"
#include "fatfs.h"
#include "ESTTC.h"
#include "TaskMonitor.h"
#include "User_types.h"
#include "DAT_Inputs.h"
#include "version.h"
#include "S_Band_Trnsm.h"
#include "X_Band_Trnsm.h"
#include "stm32f4xx_hal_i2c.h"  // alice added XCAM
#include "MCU_Init.h"           // alice added XCAM
#include  <Svc_RTC.h>

#include "D_XCAM.h"


void EPS_check(int printToFile, int printToPayload );
void EPS_read(uint16_t Cmd, long *pEpsValue);
void EPS_write(uint16_t memAddress, uint8_t Value);