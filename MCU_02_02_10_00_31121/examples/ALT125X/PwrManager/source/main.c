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

#include "assert.h"

/* Middleware includes. */
#include "serial.h"
#include "mini_console.h"

/* Driver includes */
#include "kvpfs_api.h"
#include "pwr_mngr.h"
#include "DRV_PM.h"

serial_handle sHandle_in = NULL;

static void prvMapAtCmdTask(void *pvParameters) {
  char c;

  sHandle_in = serial_open(ACTIVE_UARTI0);
  assert(sHandle_in);

  while (1) {
    serial_read(sHandle_in, &c, 1);
    console_write(&c, 1);
  }
}

int switch2cli(void) {
  int ret, ret_val = 0;

  ret = set_env("PWR_CONF_MAPON", "off");
  if (ret != 0) {
    printf("fail to set key PWR_CONF_MAPON to OFF.err:%d\r\n", ret);
    ret_val = (-1);
  } else {
    ret = save_env();
    if (ret != 0) {
      printf("fail to save to flash- ret %d \r\n", ret);
      ret_val = (-2);
    }
  }

  return ret_val;
}

int start_map(void) {
  char c;

  osThreadAttr_t attr = {0};

  attr.name = "mapOnConso";
  attr.stack_size = 4096;
  attr.priority = osPriorityRealtime7;

  osThreadId_t tid = osThreadNew(prvMapAtCmdTask, NULL, &attr);
  if (!tid) {
    printf("osThreadNew prvMapAtCmdTask failed\n");
    return -1;
  }

  printf("In MAP channel. (Press Ctrl+D to switch to CLI channel in next active cycle)\r\n");
  do {
    console_read(&c, 1);

    if (c != 4) {
      serial_write(sHandle_in, &c, 1);
    }
  } while (c != 4);

  serial_close(sHandle_in);
  if (switch2cli() == 0) {
    /* enable sleep */
    if (pwr_mngr_enable_sleep(1) != PWR_MNGR_OK) {
      printf("Failed to enable\r\n");
    }

    printf("will back to CLI in next active cycle...\r\n");
  }

  sHandle_in = NULL;
  osThreadTerminate(tid);

  return 0;
}

static void pwr_demo_task(void *arg) {
  unsigned int duration, inact;
  char *v_mode = NULL, *v_dur = NULL, *v_inact = NULL, *v_en_slp = NULL, *v_map_en = NULL;
  PWR_MNGR_PwrMode set_pwr_mode;
  bool enable_map_start = false;

  if (DRV_PM_GetDevBootType() == DRV_PM_DEV_WARM_BOOT) {
    /* load configuration when warm mode */
    if (get_env("PWR_CONF_SLP_MODE", &v_mode) == 0) {
      if (!strcmp(v_mode, "stop")) {
        set_pwr_mode = PWR_MNGR_MODE_STOP;
      } else if (!strcmp(v_mode, "standby")) {
        set_pwr_mode = PWR_MNGR_MODE_STANDBY;
      } else if (!strcmp(v_mode, "shutdown")) {
        set_pwr_mode = PWR_MNGR_MODE_SHUTDOWN;
      } else {
        set_pwr_mode = PWR_MNGR_MODE_SLEEP;
      }

      if (get_env("PWR_CONF_SLP_DUR", &v_dur) == 0) {
        duration = atoi(v_dur);
        if (pwr_mngr_conf_set_mode(set_pwr_mode, duration) != PWR_MNGR_OK) {
          printf("-> Failed to configure sleep mode [%d] duration [%d].\r\n", set_pwr_mode,
                 duration);
        };
      }

      free(v_dur);
    }

    free(v_mode);

    if (get_env("PWR_CONF_INACTT", &v_inact) == 0) {
      inact = atoi(v_inact);
      if (pwr_mngr_conf_set_cli_inact_time(inact) != PWR_MNGR_OK) {
        printf("-> Failed to configure CLI inactivity time [%d] sec.\r\n", inact);
      }
    }

    free(v_inact);

    if (get_env("PWR_CONF_SLP_EN", &v_en_slp) == 0) {
      if ((strcmp(v_en_slp, "enable") == 0)) {
        if (pwr_mngr_conf_get_sleep() == 0) {
          if (pwr_mngr_enable_sleep(1) != PWR_MNGR_OK) {
            printf("Failed to enable\r\n");
          }
        }
      } else if ((strcmp(v_en_slp, "disable") == 0)) {
        if (pwr_mngr_conf_get_sleep() == 1) {
          if (pwr_mngr_enable_sleep(0) != PWR_MNGR_OK) {
            printf("Failed to disable\r\n");
          }
        }
      }
    }

    free(v_en_slp);

    if (get_env("PWR_CONF_MAPON", &v_map_en) == 0) {
      if ((strcmp(v_map_en, "on") == 0)) {
        enable_map_start = true;
      }
    }

    free(v_map_en);

    if (enable_map_start == true) {
      osDelay(100); /* For printf available, wait for schedular ready. */
      mini_console_terminate();
      start_map();
    }
  }

  osThreadExit();
}

void pwr_demo_init(void) {
  osThreadAttr_t attr = {0};

  attr.name = "pwr_demo";
  attr.stack_size = 512 * 2;
  attr.priority = osPriorityRealtime6;

  osThreadId_t tid = osThreadNew(pwr_demo_task, NULL, &attr);

  if (!tid) {
    printf("osThreadNew failed\n");
    for (;;)
      ;
  }
}

int main(void) {
  DRV_PM_Initialize();

  DRV_GPIO_Initialize();
  DRV_GPIO_PowerControl(GPIO_POWER_FULL);

  kvpfs_init();
  mini_console();

  pwr_demo_init(); /* Demo for power testing: restore specific power settings when MCU warm boot. */

  return 0;
}
