/*  ---------------------------------------------------------------------------

    (c) copyright 2022 Altair Semiconductor, Ltd. All rights reserved.

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

/* Middleware includes. */
#include "altcom.h"
#include "mini_console.h"

static int32_t altcom_config_init() {
  altcom_init_t initCfg = {0};

  /* Configure the debugging message level */
  initCfg.dbgLevel = ALTCOM_DBG_NONE;

  /* Configure the buffer pool */
  blockset_t blkset[] = {{16, 16}, {32, 12}, {128, 5}, {256, 4}, {512, 2}, {2064, 1}, {5120, 2}};
  initCfg.bufMgmtCfg.blkCfg.blksetCfg = blkset;
  initCfg.bufMgmtCfg.blkCfg.blksetNum = sizeof(blkset) / sizeof(blkset[0]);

  /* Configure the HAL */
  initCfg.halCfg.halType = ALTCOM_HAL_INT_UART;
  initCfg.halCfg.virtPortId = 0;

  return altcom_initialize(&initCfg);
}


static void init_task(void* arg) {
  mini_console();
  altcom_config_init();

  osThreadExit();
}

int main(void) {
  osThreadAttr_t attr = {0};

  attr.name = "init";
  attr.stack_size = 3072;
  attr.priority = osPriorityRealtime;
  osThreadNew(init_task, NULL, &attr);

  return 0;
}
