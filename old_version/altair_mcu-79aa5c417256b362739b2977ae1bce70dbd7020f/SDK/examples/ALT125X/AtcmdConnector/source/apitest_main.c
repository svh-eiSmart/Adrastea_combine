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
#include "atcmd/altcom_atcmd.h"

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
/****************************************************************************
 * Private Data Type
 ****************************************************************************/
struct apitest_command_s {
  int argc;
  char *argv[APITEST_MAX_NUMBER_OF_PARAM];
};

struct data_s {
  char *data;
  int16_t len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

osMessageQueueId_t atq = NULL;
bool atmode_start = false;
bool echo_enable = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_retval(bool val, char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

static void atUnsolCallback(const char *urcStr, void *userPriv) {
  if (atmode_start) {
    APITEST_DBG_PRINT("%s", urcStr);
  } else {
    APITEST_DBG_PRINT("Incoming URC ==start==\n%s\n==end==\nuserpriv: %p\n", urcStr, userPriv);
  }
}

static void apitest_task(void *arg) {
  int cmp_res;
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
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "atcmd", strlen("atcmd"));
      if (cmp_res == 0) {
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "atmode", strlen("atmode"));
        if (cmp_res == 0) {
          if (command->argc == 3) {
            struct data_s *cmd = NULL;
            char *resBuf = NULL;
            osStatus_t atRet;

            resBuf = malloc(ATCMD_MAX_CMD_LEN);
            if (NULL == resBuf) {
              APITEST_DBG_PRINT("malloc fail\n");
              break;
            }

            atmode_start = true;
            APITEST_DBG_PRINT("atmode start, echo mode: %d, use \"atcmd\" to send AT Command!\n",
                              echo_enable);
            do {
              atRet = osMessageQueueGet(atq, (void *)(&cmd), 0, osWaitForever);
              if (atRet != osOK) {
                APITEST_DBG_PRINT("osMessageQueueGet atq fail[%d]\n", atRet);
                free(resBuf);
                break;
              }

              if (NULL == cmd) {
                free(resBuf);
                break;
              }

              if (0 == cmd->len) {
                free(cmd->data);
                free(cmd);
                APITEST_DBG_PRINT("\r\nERROR\r\n");
                continue;
              }
              if (4 == cmd->len && !strncasecmp("ATE", cmd->data, strlen("ATE"))) {
                echo_enable = atoi(&cmd->data[3]) == 0 ? 0 : 1;
                free(cmd->data);
                free(cmd);
                APITEST_DBG_PRINT("\r\nOK\r\n");
                continue;
              }

              resBuf[0] = '\0';
              atRet = altcom_atcmdSendCmd(cmd->data, ATCMDCFG_MAX_CMDRES_LEN, resBuf);
              if (0 <= atRet) {
                APITEST_DBG_PRINT("%s", resBuf);
              } else {
                APITEST_DBG_PRINT("\r\nERROR\r\n");
              }

              free(cmd->data);
              free(cmd);
            } while (true);
            APITEST_DBG_PRINT("atmode stop!\n");
            atmode_start = false;
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "reg", strlen("reg"));
        if (cmp_res == 0) {
          if (command->argc == 4) {
            print_retval(ATCMD_SUCCESS == altcom_atcmdRegUnsol(command->argv[APITEST_CMD_PARAM_2],
                                                               atUnsolCallback, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(altcom_atcmdRegUnsol));
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "unreg", strlen("unreg"));
        if (cmp_res == 0) {
          if (command->argc == 4) {
            print_retval(ATCMD_SUCCESS ==
                             altcom_atcmdRegUnsol(command->argv[APITEST_CMD_PARAM_2], NULL, NULL),
                         APITEST_GETFUNCNAME(altcom_atcmdRegUnsol));
          }
        }
      }
    }

    if (command) {
      APITEST_FREE_CMD(command, command->argc);
      command = NULL;
    }
  }

  osThreadExit();
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

    atq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(char *), NULL);
    if (!atq) {
      APITEST_DBG_PRINT("atq init failed\n");
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

  if (atq) {
    osMessageQueueDelete(atq);
    atq = NULL;
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
