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
#include <stdlib.h>
#include <string.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* DRV includes */
#include "DRV_GPTIMER.h"

static void gptimer_test1_interrupt_handler(uint32_t user_param) {
  DRV_GPTIMER_DisableInterrupt(0);

  printf("gptimer interrupt 0 test handler (%ld)\n\r", user_param);
  printf("gptimer value is %ld\n\r", DRV_GPTIMER_GetValue());
}

static int32_t test2_int_counter;

static void gptimer_test2_interrupt_handler(uint32_t user_param) {
  test2_int_counter--;
  if (test2_int_counter > 0) {
    DRV_GPTIMER_SetInterruptOffset(1, user_param);
  } else {
    DRV_GPTIMER_DisableInterrupt(1);
  }

  printf("gptimer interrupt 1 test handler %ld\n\r", test2_int_counter);
  printf("gptimer value is %ld\n\r", DRV_GPTIMER_GetValue());
}

void print_gptimer_usage(void) {
  printf("Usage:\n\r");
  printf("gptimer [start|stop|read|off|val|req|freq] <args>\n\r");
  printf("  gptimer start <value>      - Start GP timer with intial value (default is 0)\n\r");
  printf("  gptimer read               - Read GP timer\n\r");
  printf("  gptimer stop               - Stop GP timer\n\r");
  printf("  gptimer off offset         - Set GP timer interrupt 0 with offset\n\r");
  printf("  gptimer val value          - Set GP timer interrupt 0 at value\n\r");
  printf(
      "  gptimer req offset <count> - Set GP timer requiring interrupt 1 with offset for count "
      "times (default count 3)\n\r");
  printf("  gptimer freq frequency     - Set GP timer frequency\n\r");
}

int32_t do_gptimer(char *s) {
  char command[20] = {0}, par2[20] = {0}, par3[20] = {0};
  int32_t argc;
  int32_t ret_val = 0;

  argc = sscanf(s, "%s %s %s", command, par2, par3) + 1;

  if (argc <= 1) {
    print_gptimer_usage();
  } else {
    if (argc > 1) {
      if (strcmp("read", command) == 0) {
        printf("Timer = %ld\n\r", DRV_GPTIMER_GetValue());
      } else if (strcmp("start", command) == 0) {
        uint32_t value;
        if (argc == 3) {
          value = strtol(par2, NULL, 10);
        } else {
          value = 0;
        }
        printf("Start timer with value %ld\n\r", value);
        DRV_GPTIMER_Activate(value);
      } else if (strcmp("stop", command) == 0) {
        DRV_GPTIMER_Disable();
        DRV_GPTIMER_DisableInterrupt(0);
        DRV_GPTIMER_DisableInterrupt(1);
        printf("Timer stopped\n\r");
      } else if (strcmp("off", command) == 0) {
        if (argc == 3) {
          int32_t gptimer = 0;
          uint32_t value = DRV_GPTIMER_GetValue();
          uint32_t offset = strtol(par2, NULL, 10);
          DRV_GPTIMER_RegisterInterrupt(gptimer, gptimer_test1_interrupt_handler, offset);
          if (DRV_GPTIMER_SetInterruptOffset(gptimer, offset) == 0) {
            printf("Register interrupt %ld at offset %ld. Current timer is %ld\n\r", gptimer,
                   offset, value);
          } else {
            printf("Could not register interrupt %ld with offset %ld\n\r", gptimer, offset);
          }
        } else {
          printf("Wrong number of parameters\n\r");
        }
      } else if (strcmp("val", command) == 0) {
        if (argc == 3) {
          int32_t gptimer = 0;
          uint32_t value = DRV_GPTIMER_GetValue();
          uint32_t val2 = strtol(par2, NULL, 10);
          DRV_GPTIMER_RegisterInterrupt(gptimer, gptimer_test1_interrupt_handler, val2);
          if (DRV_GPTIMER_SetInterruptValue(gptimer, val2) == 0) {
            printf("Register interrupt %ld at value %ld. Current timer is %ld\n\r", gptimer, val2,
                   value);
          } else {
            printf("Could not register interrupt %ld at value %ld\n\r", gptimer, val2);
          }
        } else {
          printf("Wrong number of parameters\n\r");
        }
      } else if (strcmp("req", command) == 0) {
        if ((argc == 3) || (argc == 4)) {
          int32_t gptimer = 1;
          uint32_t offset = strtol(par2, NULL, 10);
          uint32_t value = DRV_GPTIMER_GetValue();
          DRV_GPTIMER_RegisterInterrupt(gptimer, gptimer_test2_interrupt_handler, offset);
          if (argc == 4) {
            test2_int_counter = strtol(par3, NULL, 10);
          } else {
            test2_int_counter = 3;
          }
          if (DRV_GPTIMER_SetInterruptOffset(gptimer, offset) == 0) {
            printf("Register repitative interrupt %ld with offset %ld. Current timer is %ld\n\r",
                   gptimer, offset, value);
          } else {
            printf("Could not register interrupt %ld with offset %ld\n\r", gptimer, offset);
          }
        } else {
          printf("Wrong number of parameters\n\r");
        }
      } else if (strcmp("freq", command) == 0) {
        if (argc == 3) {
          int32_t freq = DRV_GPTIMER_SetFrequency(strtol(par2, NULL, 10));
          if (freq > 0) {
            printf("Frequency set to %ld\n\r", freq);
          } else {
            printf("Could not set frequency\n\r");
          }
        } else {
          printf("Wrong number of parameters\n\r");
        }
      } else {
        print_gptimer_usage();
      }
    }
  }

  return ret_val;
}
