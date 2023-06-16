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

#include "cmsis_os2.h"

#include "serial.h"
#include "mini_console.h"
#include "assert.h"

static serial_handle sHandle_i = NULL;

static void prvAtCmdTask(void *pvParameters) {
  char c;

  while (1) {
    serial_read(sHandle_i, &c, 1);
    console_write(&c, 1);
  }
}

int do_map(char *s) {
  char c;
  sHandle_i = serial_open(ACTIVE_UARTI0);
  assert(sHandle_i);

  osThreadAttr_t attr = {0};
  attr.name = "Console";
  attr.stack_size = 2048;
  attr.priority = osPriorityNormal;

  osThreadId_t AtCmdTaskHandle = osThreadNew(prvAtCmdTask, NULL, &attr);

  printf("Open MAP CLI (Press Ctrl+D to exit)\r\n");
  do {
    console_read(&c, 1);

    if (c != 4) {
      serial_write(sHandle_i, &c, 1);
    }
  } while (c != 4);
  printf("MAP CLI Closed.\r\n");
  serial_close(sHandle_i);
  sHandle_i = NULL;

  osThreadTerminate(AtCmdTaskHandle);

  return 0;
}
