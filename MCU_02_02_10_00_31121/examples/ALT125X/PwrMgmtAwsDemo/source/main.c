/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

#include "DRV_PM.h"

/* Power management includes */
#include "pwr_mngr.h"

/* Middleware includes. */
#include "kvpfs_api.h"
#include "mini_console.h"

/* Application includes */
#include "appMainLoop.h"

void appMainLoopInitCaller_task(void *arg) {
  appMainLoopInit(NULL);
  osThreadExit();
}
/*-----------------------------------------------------------*/

void appMainLoopInitCaller(void) {
  osThreadAttr_t attr = {0};

  attr.name = "appMainLoopInit";
  attr.stack_size = 2048;
  attr.priority = osPriorityRealtime;
  osThreadNew(appMainLoopInitCaller_task, NULL, &attr);
}
/*-----------------------------------------------------------*/

int main(void) {
  DRV_PM_Initialize();

  DRV_GPIO_Initialize();
  DRV_GPIO_PowerControl(GPIO_POWER_FULL);

  kvpfs_init();

  mini_console();
  appMainLoopInitCaller();

  return 0;
}
