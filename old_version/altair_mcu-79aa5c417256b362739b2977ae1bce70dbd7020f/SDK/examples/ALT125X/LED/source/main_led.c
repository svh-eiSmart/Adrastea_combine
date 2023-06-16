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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "DRV_LED.h"

int do_led(char* s) {
#define MAX_PARAM (6)

  char* pch;
  int argc = 0;
  char* argv[MAX_PARAM];  // two parameters at most
  int ret = 1;
  DRV_LED_Param led = {0};
  DRV_LED_Dim_Param dim = {0};

  pch = strtok(s, " ");
  while (pch != NULL) {
    if (argc == MAX_PARAM) return ret;
    argv[argc++] = pch;
    pch = strtok(NULL, " ");
  }

  if (!strcmp(argv[0], "led")) {
    printf("config led\n");
    led.timing.polarity = 0;
    led.timing.offset = atoi(argv[1]);  // 0;
    led.on_duration = atoi(argv[2]);    // 0x400;
    led.off_duration = atoi(argv[3]);   // 0x400;
    // DRV_LED_Set_Led_Mode(LED_CTRL_2 | LED_CTRL_4, &led);
    DRV_LED_Set_Led_Mode(atoi(argv[4]), &led);
  } else if (!strcmp(argv[0], "dim")) {
    printf("config dim\n");
    dim.timing.polarity = 0;
    dim.timing.offset = atoi(argv[1]);  // 0;
    dim.dim_sel = atoi(argv[2]);        // eDIM0;
    dim.bright_level = atoi(argv[3]);   // 20
    dim.repeat_times = atoi(argv[4]);   // 3
    dim.brightest_hold = 0;
    dim.darkest_hold = 0;
    // DRV_LED_Set_Dim_Mode(LED_CTRL_5, &dim);
    DRV_LED_Set_Dim_Mode(atoi(argv[5]), &dim);
  } else if (!strcmp(argv[0], "start")) {
    printf("start\n");
    // DRV_LED_Start(LED_CTRL_4|LED_CTRL_2|LED_CTRL_5);
    DRV_LED_Start(atoi(argv[1]));
  } else if (!strcmp(argv[0], "stop")) {
    DRV_LED_Stop(atoi(argv[1]));
  }

  return 0;
}

int do_hello(char* s) {
  printf("Hello %s\n\r", s);

  if (strcmp(s, "world")) {
    return -1;
  } else {
    return 0;
  }
}
