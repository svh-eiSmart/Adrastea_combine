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
#include <time.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "lwm2m/altcom_lwm2m.h"

/* App includes */
#include "apitest_main.h"
#include "Infra/app_dbg.h"
#include "ReportManager/ReportManagerAPI.h"
#include "database.h"

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

#define APITEST_TASK_STACKSIZE (3072)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_INVALID_ARG ("INVALID")
#define APITEST_NULL_ARG ("NULL")

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

#define LWM2M_DEVICE_OBJECT_ID 3
#define LWM2M_CONNECTIVITY_MONITORING_OBJECT_ID 4
#define LWM2M_LOCATION_OBJECT_ID 6
#define LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID 19

#define REPORT_MANAGER_TIMEOUT 8000  // 8sec

#define CONNECTIVITY_MONITORING_OBJID 4
#define LOCATION_OBJID 6
#define APPLICATION_DATA_CONTAINER_OBJID 19
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
static bool is_rm_init = false;
static report_type_bitmask_t retportMngrReportTypes =
    REPORT_TYPE_LWM2M_LOCATION | REPORT_TYPE_LWM2M_LTE_CELL | REPORT_TYPE_LWM2M_BIN_APP_DATA;

client_inst_t lwm2m_client = CLIENT_INST_OPERATOR;

char *bindstr[] = {"SERVER_BINDING_UNKNOWN",      "SERVER_BINDING_UDP",
                   "SERVER_BINDING_UDP_QUEUE",    "SERVER_BINDING_SMS",
                   "SERVER_BINDING_SMS_QUEUE",    "SERVER_BINDING_UDP_SMS",
                   "SERVER_BINDING_UDP_QUEUE_SMS"};

char *statstr[] = {"SERVER_STAT_NOT_REG_OR_BOOTSTRAP_NOT_STARTED",
                   "SERVER_STAT_REG_PENDING",
                   "SERVER_STAT_REG_SUCCESS",
                   "SERVER_STAT_LAST_REG_FAILED",
                   "SERVER_STAT_REG_UPDATE_PENDING",
                   "SERVER_STAT_DEREG_PENGING",
                   "SERVER_STAT_BOOTSTRAP_HOLD_OFF_TIME",
                   "SERVER_STAT_BOOTSTRAP_REQ_SENT",
                   "SERVER_STAT_BOOTSTRAP_ONGOING",
                   "SERVER_STAT_BOOTSTRAP_DONE",
                   "SERVER_STAT_BOOTSTRAP_FAILED"};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void print_retval(bool val, char *str) {
  if (!val) {
    APP_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APP_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

static void reportManagerResultCB(reportResult_t *reportResult) {
  APP_DBG_PRINT("[CB]reportResultCB with resultType=%d result=%d, failed reason=%d\r\n",
                (int)reportResult->type, (int)reportResult->result, (int)reportResult->reasonFail);
}

static int prepare_report(reportType_t rpType, repDesc_t *rpDesc) {
  APP_DBG_PRINT("Preparing report, type: 0x%02X", (unsigned int)rpType);
  rpDesc->reportType = rpType;
  switch (rpType) {
    case REPORT_TYPE_LOCATION:
      return database_read_obj(LOCATION_OBJID, &rpDesc->u.lwm2m_location);

    case REPORT_TYPE_CELLID:
      return database_read_obj(CONNECTIVITY_MONITORING_OBJID, &rpDesc->u.lwm2m_cell_id);

    case REPORT_TYPE_APPDATA_CONTAINER:
      return database_read_obj(APPLICATION_DATA_CONTAINER_OBJID, &rpDesc->u.lwm2m_app_data);

    default:
      APP_ERR_PRINT("Invalid report type: %d", (int)rpType);
      return -1;
  }
}

static void apitest_task(void *arg) {
  int cmp_res;
  osStatus_t ret;
  struct apitest_command_s *command;
  int retcode;

  retcode = database_init();
  APP_ASSERT(!retcode, "database_init failed!\r\n");

  taskrunning = true;
  if (NULL == cmdq) {
    APP_DBG_PRINT("cmdq is NULL!!\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until send commands */
    APP_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APP_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APP_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      /* Set active client */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "actclnt", strlen("actclnt"));
      if (cmp_res == 0) {
        if (command->argc == 2) {
          APP_DBG_PRINT("Active client is %d", lwm2m_client);
        } else if (command->argc == 3) {
          uint32_t client;

          client = strtoul(command->argv[APITEST_CMD_PARAM_1], NULL, 10);
          if (client <= CLIENT_INST_ALTAIR) {
            lwm2m_client = (client_inst_t)client;
            APP_DBG_PRINT("Change active client to %d", lwm2m_client);
          } else {
            APP_DBG_PRINT("Invalid client id %lu", client);
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }

      /* Start bootstrap */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "btstrp", strlen("btstrp"));
      if (cmp_res == 0) {
        if (command->argc == 2 || command->argc == 3) {
          int force;

          force = command->argc > 2 ? strtoul(command->argv[APITEST_CMD_PARAM_1], NULL, 10) : 0;
          if (reportManagerConfigApi(retportMngrReportTypes, force) == REPORT_MNGR_CONFIG_UPDATED) {
            APP_DBG_PRINT("Bootstrap completed, please restart system\r\n");
          } else {
            APP_DBG_PRINT("reportManagerConfigApi failed\r\n");
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }

      /* Start ReportManager */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "startrm", strlen("startrm"));
      if (cmp_res == 0) {
        if (command->argc == 2) {
          if (!is_rm_init) {
            Report_result res;
            int ret;

            ret = reportMngrBindExeIndApi(LWM2M_DEVICE_OBJECT_ID, database_exe);
            APP_ASSERT(!ret, "reportMngrBindExeIndApi failed\r\n");

            ret = reportMngrBindReadIndApi(LWM2M_LOCATION_OBJECT_ID, database_read);
            APP_ASSERT(!ret, "reportMngrBindReadIndApi failed\r\n");

            ret = reportMngrBindReadIndApi(LWM2M_CONNECTIVITY_MONITORING_OBJECT_ID, database_read);
            APP_ASSERT(!ret, "reportMngrBindReadIndApi failed\r\n");

            ret =
                reportMngrBindReadIndApi(LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID, database_read);
            APP_ASSERT(!ret, "reportMngrBindReadIndApi failed\r\n");

            ret = reportMngrBindWriteIndApi(LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID,
                                            database_write);
            APP_ASSERT(!ret, "reportMngrBindReadIndApi failed\r\n");

            res = reportManagerInitApi(reportManagerResultCB, REPORT_MANAGER_TIMEOUT,
                                       retportMngrReportTypes, true);
            APP_ASSERT(res == REPORT_MNGR_RET_CODE_OK, "reportMngrBindReadIndApi failed\r\n");
            is_rm_init = true;
          } else {
            APP_ERR_PRINT("ReportManager already initialized!\r\n");
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }

      /* Update registration by ReportManager */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "updtrm", strlen("updtrm"));
      if (cmp_res == 0) {
        if (command->argc == 2) {
          Report_result res;

          res = reportManagerInitRefreshApi();
          if (res != REPORT_MNGR_RET_CODE_OK) {
            APP_ERR_PRINT("ReportManager update failed!\r\n");
            goto cmdfinish;
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }

      /* Get server information */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtsrvinfo", strlen("gtsrvinfo"));
      if (cmp_res == 0) {
        if (command->argc == 2) {
          lwm2m_serverinfo_t srvinfo;
          LWM2MError_e retcode;

          print_retval(
              LWM2M_SUCCESS == (retcode = altcom_lwm2mGetServerInfo(lwm2m_client, &srvinfo)),
              TOSTR(altcom_lwm2mGetServerInfo));
          if (retcode == LWM2M_SUCCESS) {
            int i;

            for (i = 0; i < MAX_LWM2M_SERVER_NUM; i++) {
              if (srvinfo.info[i].valid != 0) {
                APP_DBG_PRINT("Server %d\r\n", i);
                APP_DBG_PRINT("Server URL: %s\r\n", srvinfo.info[i].server_uri);
                APP_DBG_PRINT("Server ID: %lu\r\n", srvinfo.info[i].server_id);
                APP_DBG_PRINT("Life time: %ld seconds\r\n", srvinfo.info[i].liftime);
                APP_DBG_PRINT("Binding: %s\r\n", bindstr[srvinfo.info[i].binding]);
                APP_DBG_PRINT("State: %s\r\n", statstr[srvinfo.info[i].server_stat]);

                char timeStr[32];
                time_t regtime = (time_t)srvinfo.info[i].last_regdate;

                if (CTIME_R(&regtime, timeStr)) {
                  APP_DBG_PRINT("Registered since: %s\r\n", timeStr);
                } else {
                  APP_DBG_PRINT("Something wrong in regtime or timeStr\r\n");
                }

                APP_DBG_PRINT("\r\n");
              }
            }
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }

      /* Update Registration by ReportManager */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "sendrp", strlen("sendrp"));
      if (cmp_res == 0) {
        if (command->argc == 3) {
          repDesc_t rp;
          Report_result res;
          int ret;

          ret = prepare_report((reportType_t)strtoul(command->argv[APITEST_CMD_PARAM_1], NULL, 10),
                               &rp);
          if (ret) {
            APP_ERR_PRINT("prepare_report failed!\r\n");
            goto cmdfinish;
          }

          res = reportManagerSendReportsApi(&rp);
          if (res != REPORT_MNGR_RET_CODE_OK) {
            APP_ERR_PRINT("reportManagerSendReportsApi failed!\r\n");
          }
        } else {
          APP_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        }

        goto cmdfinish;
      }
    }
  cmdfinish:

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
    app_log_mtx = osMutexNew(NULL);
    if (!app_log_mtx) {
      APP_DBG_PRINT("app_log_mtx init failed\n");
      goto errout;
    }

    cmdq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(struct apitest_command_s *), NULL);
    if (!cmdq) {
      APP_DBG_PRINT("cmdq init failed\n");
      goto errout;
    }
  } else {
    APP_DBG_PRINT("App already initialized\n");
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
    APP_DBG_PRINT("App not yet initialized\n");
    return -1;
  }

  if (2 <= argc) {
    if (APITEST_MAX_NUMBER_OF_PARAM < argc) {
      APP_DBG_PRINT("too many arguments\n");
      return -1;
    }

    if ((command = malloc(sizeof(struct apitest_command_s))) == NULL) {
      APP_DBG_PRINT("malloc fail\n");
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
        APP_DBG_PRINT("osThreadNew failed\n");
        APITEST_FREE_CMD(command, command->argc);
        return -1;
      }
    }

    if (osOK != osMessageQueuePut(cmdq, (const void *)&command, 0, osWaitForever)) {
      APP_DBG_PRINT("osMessageQueuePut to apitest_task Failed!!\n");
      APITEST_FREE_CMD(command, command->argc);
    }
  }

  return 0;
}
