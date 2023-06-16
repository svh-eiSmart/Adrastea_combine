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

/* Standard includes. */
#include <string.h>
#include <assert.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* Middleware includes. */
#include "altcom.h"
#include "serial.h"
#include "mini_console.h"

/* App includes */
#include "apitest_main.h"

/* Power management includes */
#include "DRV_PM.h"
#include "pwr_mngr.h"
#include "sleep_notify.h"

/* Platform includes */
#include "newlibPort.h"

int32_t sleep_callback(sleep_notify_state state, PWR_MNGR_PwrMode mode, void *arg) {
  printf("\r\n%s\r\n", (state == SUSPENDING) ? "\tSUSPENDING" : "\tRESUMING");
  printf("\r\nPower Mode:%lu\r\n", (uint32_t)mode);
  assert(mode == PWR_MNGR_MODE_STANDBY || mode == PWR_MNGR_MODE_STOP);

  return SLEEP_NOTIFY_SUCCESS;
}

static int32_t altcom_config_init();

static void init_task(void *arg) {
  mini_console();
  apitest_init();
  altcom_config_init();
  pwr_mngr_enable_sleep(0);

  /* Register the sleep notify callback function */
  int32_t _;
  if (sleep_notify_register_callback(sleep_callback, &_, NULL) != 0) {
    return;
  }

  vTaskDelete(NULL);
}

int main(void) {
  DRV_PM_Initialize();

  osThreadAttr_t attr = {0};

  attr.name = "init";
  attr.stack_size = 2048;
  attr.priority = osPriorityRealtime;
  osThreadNew(init_task, NULL, &attr);

  return 0;
}
/*-----------------------------------------------------------*/

static int32_t altcom_config_init() {
  altcom_init_t initCfg;
  memset((void *)&initCfg, 0x0, sizeof(initCfg));
  /* Configure the debugging message level */
  initCfg.dbgLevel = ALTCOM_DBG_NORMAL;

  /* Provide application log lock interface */
  loglock_if_t loglock_if = {apitest_log_lock, apitest_log_unlock};
  initCfg.loglock_if = &loglock_if;

  /* Configure the buffer pool */
  blockset_t blkset[] = {{16, 16}, {32, 12}, {128, 5}, {256, 4}, {512, 10}, {2064, 4}, {5120, 10}};
  initCfg.bufMgmtCfg.blkCfg.blksetCfg = blkset;
  initCfg.bufMgmtCfg.blkCfg.blksetNum = sizeof(blkset) / sizeof(blkset[0]);

  /* Configure the HAL */
  initCfg.halCfg.halType = ALTCOM_HAL_INT_UART;
  initCfg.halCfg.virtPortId = 0;

  return altcom_initialize(&initCfg);
}
