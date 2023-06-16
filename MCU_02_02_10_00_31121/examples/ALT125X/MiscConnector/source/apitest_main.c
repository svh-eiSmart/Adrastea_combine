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

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "misc/altcom_misc.h"

/* Middleware includes. */
#include "sfplogger.h"

/*Applib includes */
#include "timex.h"

/* App includes */
#include "apitest_main.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APITEST_RESULT_OK (0)
#define APITEST_RESULT_ERROR (1)

#define APITEST_CMD_ARG (1)
#define APITEST_CMD_PARAM_1 (2)
#define APITEST_CMD_PARAM_2 (3)
#define APITEST_CMD_PARAM_3 (4)
#define APITEST_CMD_PARAM_4 (5)
#define APITEST_CMD_PARAM_5 (6)
#define APITEST_CMD_PARAM_6 (7)
#define APITEST_CMD_PARAM_7 (8)
#define APITEST_CMD_PARAM_8 (9)

#define APITEST_MAX_NUMBER_OF_PARAM (12)
#define APITEST_MAX_API_MQUEUE (16)

#define APITEST_TASK_STACKSIZE (2048)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_INVALID_ARG ("INVALID")
#define APITEST_NULL_ARG ("NULL")
#define APITEST_UNDEF ("UNDEF")

#define APITEST_GETFUNCNAME(func) (#func)

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
    printf(__VA_ARGS__);       \
    UNLOCK();                  \
  } while (0)

#define APITEST_FREE_CMD(cmd, argc)    \
  do {                                 \
    int32_t idx;                       \
    for (idx = 0; idx < argc; idx++) { \
      free(cmd->argv[idx]);            \
      cmd->argv[idx] = NULL;           \
    }                                  \
                                       \
    free(cmd);                         \
  } while (0);
#if defined(__GNUC__)
#define CTIME_R(time, buf) ctime_r(time, buf)
#elif defined(__ICCARM__)
#define CTIME_R(time, buf) !ctime_s(buf, sizeof(buf), time)
#else
#error time function not supported in this toolchain
#endif

/****************************************************************************
 * Private Data Type
 ****************************************************************************/

struct apitest_command_s {
  int argc;
  char *argv[APITEST_MAX_NUMBER_OF_PARAM];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_retval(bool val, FAR char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

static void apitest_task(FAR void *arg) {
  osStatus_t ret;
  struct apitest_command_s *command;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until send commands */

    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      /* altcom_GetACfg & altcom_SetACfg */
      if (!strncmp(command->argv[APITEST_CMD_ARG], "cfg", strlen("cfg"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "get", strlen("get"))) {
          if (4 == command->argc) {
            Misc_Error_e result;
            char *cfgName;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              cfgName = NULL;
            } else {
              cfgName = command->argv[APITEST_CMD_PARAM_2];
            }

            char cfgValue[MISC_MAX_CFGVALUE_LEN];

            print_retval(MISC_SUCCESS == (result = altcom_GetACfg(cfgName, cfgValue)),
                         APITEST_GETFUNCNAME(altcom_GetACfg));

            if (MISC_SUCCESS == result) {
              APITEST_DBG_PRINT("\"%s\" = \"%s\"\n", cfgName, cfgValue);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }

        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "set", strlen("set"))) {
          if (5 == command->argc) {
            char *cfgName, *cfgValue;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              cfgName = NULL;
            } else {
              cfgName = command->argv[APITEST_CMD_PARAM_2];
            }

            if (!strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              cfgValue = NULL;
            } else {
              cfgValue = command->argv[APITEST_CMD_PARAM_3];
            }

            print_retval(MISC_SUCCESS == altcom_SetACfg(cfgName, cfgValue),
                         APITEST_GETFUNCNAME(altcom_SetACfg));
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      /* altcom_GetTime */
      if (!strncmp(command->argv[APITEST_CMD_ARG], "time", strlen("time"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "get", strlen("get"))) {
          if (3 == command->argc) {
            time_t map_time;
            char timeStr[32];

            APITEST_DBG_PRINT("sizeof(time_t): %d\n", sizeof(time_t));
            print_retval(MISC_SUCCESS == altcom_GetTime(&map_time),
                         APITEST_GETFUNCNAME(altcom_GetTime));

            if (CTIME_R(&map_time, timeStr)) {
              APITEST_DBG_PRINT("MAP Time: %s", timeStr);
            } else {
              APITEST_DBG_PRINT("Something wrong in map_time or timeStr\n");
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      /* altcom_get_resetinfo */
      if (!strncmp(command->argv[APITEST_CMD_ARG], "gtrstinfo", strlen("gtrstinfo"))) {
        if (2 == command->argc) {
          Misc_Error_e result;
          ResetInfo_t info;

          print_retval(MISC_SUCCESS == (result = altcom_get_resetinfo(&info)),
                       APITEST_GETFUNCNAME(altcom_get_resetinfo));
          if (result == MISC_SUCCESS) {
            APITEST_DBG_PRINT(
                "==Rest Information==\nType: %lu\nCause: %lu\nCPU: %lu\nFailure Type: %lu\n",
                (uint32_t)info.type, (uint32_t)info.cause, (uint32_t)info.cpu,
                (uint32_t)info.failure_type);
            APITEST_DBG_PRINT(
                "[CAUTION]\"CPU\" and \"Failure Type\" only valid on the specific \"Cause\"!\n");
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "slog", strlen("slog"))) {
        if ((command->argv[APITEST_CMD_PARAM_1] != NULL) &&
            ((strlen(command->argv[APITEST_CMD_PARAM_1]) > 0) &&
             (strlen(command->argv[APITEST_CMD_PARAM_1]) < 80))) {
          if (3 == command->argc) {
            sfp_log_string("%s", command->argv[APITEST_CMD_PARAM_1]);

          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d \n\r", command->argc);
            goto errout_with_param;
          }
        }
      }
    }
  errout_with_param:
    if (command) {
      APITEST_FREE_CMD(command, command->argc);
      command = NULL;
    }
  }

  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void apitest_log_lock(void) {
  if (app_log_mtx) {
    osMutexAcquire(app_log_mtx, osWaitForever);
  }
}

void apitest_log_unlock(void) {
  if (app_log_mtx) {
    osMutexRelease(app_log_mtx);
  }
}

int32_t apitest_init(void) {
  if (!app_init) {
    if (sfp_log_init()) {
      goto errout;
    } else {
      sfp_log_string("%s", "### Hello World ###");
    }

    app_log_mtx = osMutexNew(NULL);
    if (!app_log_mtx) {
      APITEST_DBG_PRINT("app_log_mtx init failed\n");
      goto errout;
    }

    cmdq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(struct apitest_command_s *), NULL);
    if (!cmdq) {
      APITEST_DBG_PRINT("cmdq init failed\n");
      goto errout;
    }
  } else {
    APITEST_DBG_PRINT("App already initialized\n");
  }

  app_init = true;
  return 0;

errout:
  if (app_log_mtx) {
    osMutexDelete(app_log_mtx);
    app_log_mtx = NULL;
  }

  if (cmdq) {
    osMessageQueueDelete(cmdq);
    cmdq = NULL;
  }

  return -1;
}

int32_t apitest_main(int32_t argc, char *argv[]) {
  struct apitest_command_s *command;
  int32_t itr = 0;

  if (!app_init) {
    APITEST_DBG_PRINT("App not yet initialized\n");
    return -1;
  }

  if (2 <= argc) {
    if (APITEST_MAX_NUMBER_OF_PARAM < argc) {
      APITEST_DBG_PRINT("too many arguments\n");
      return -1;
    }

    if ((command = malloc(sizeof(struct apitest_command_s))) == NULL) {
      APITEST_DBG_PRINT("malloc fail\n");
      return -1;
    }

    memset(command, 0, sizeof(struct apitest_command_s));
    command->argc = argc;

    for (itr = 0; itr < argc; itr++) {
      if ((command->argv[itr] = malloc(strlen(argv[itr]) + 1)) == NULL) {
        APITEST_FREE_CMD(command, itr);
        return -1;
      }

      memset(command->argv[itr], '\0', (strlen(argv[itr]) + 1));
      strncpy((command->argv[itr]), argv[itr], strlen(argv[itr]));
    }

    if (!taskrunning) {
      osThreadAttr_t attr = {0};

      attr.name = "apitest";
      attr.stack_size = APITEST_TASK_STACKSIZE;
      attr.priority = APITEST_TASK_PRI;

      osThreadId_t tid = osThreadNew(apitest_task, NULL, &attr);

      if (!tid) {
        APITEST_DBG_PRINT("osThreadNew failed\n");
        APITEST_FREE_CMD(command, command->argc);
        return -1;
      }
    }

    if (osOK != osMessageQueuePut(cmdq, (const void *)&command, 0, osWaitForever)) {
      APITEST_DBG_PRINT("osMessageQueuePut to apitest_task Failed!!\n");
      APITEST_FREE_CMD(command, command->argc);
    }
  }

  return 0;
}
