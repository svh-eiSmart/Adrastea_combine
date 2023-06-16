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

#include "assert.h"

/* ALTCOM API includes */
#include "mqtt/altcom_mqtt.h"

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
#define APITEST_CMD_PARAM_9 (10)
#define APITEST_CMD_PARAM_10 (11)
#define APITEST_CMD_PARAM_11 (12)
#define APITEST_CMD_PARAM_12 (13)

#define APITEST_MAX_NUMBER_OF_PARAM (15)
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

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;
static MQTTSession_t *gSession = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void mqttMessageCb(MQTTSession_t *session, MqttEvent_e evt, MqttResultData_t *result,
                          void *userPriv) {
  MessageData *msgData = &result->messageData;

  assert(gSession == session);
  switch (evt) {
    case MQTT_EVT_CONCONF:
    case MQTT_EVT_DISCONF:
    case MQTT_EVT_SUBCONF:
    case MQTT_EVT_UNSCONF:
      APITEST_DBG_PRINT("API event %d arrival, retcode:  %d, errcode %d, msgId %d\n", evt,
                        result->resultCode, result->errorCode, result->messageId);
      break;

    case MQTT_EVT_PUBRCV:

      APITEST_DBG_PRINT(
          "[QoS:%u] Receive topic: ==%.*s==, len: %hu\nmessage: ==%.*s==, len: %hu\npriv: %p, "
          "msgId: %d\n",
          msgData->qos, msgData->topicLen, msgData->topic, msgData->topicLen, msgData->msgLen,
          msgData->message, msgData->msgLen, userPriv, msgData->id);

      break;

    case MQTT_EVT_CONNFAIL:
      APITEST_DBG_PRINT("MQTT connection failure!\n");
      break;

    default:
      APITEST_DBG_PRINT("Invalid evt code: %d\n", evt);
  }
}

static void print_retval(bool val, char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
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
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "mqtt", strlen("mqtt"));
      if (cmp_res == 0) {
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "create", strlen("create"));
        if (cmp_res == 0) {
          if (!gSession) {
            MQTTSessionParams_t params;
            memset(&params, 0, sizeof(MQTTSessionParams_t));
            /* MQTT default paramters */
            params.sessionType = MQTT_SESSION_TYPE_MQTT;
            params.ip.PdnSessionId = 0;
            params.ip.ipType = 0;
            params.protocol.cleanSessionFlag = 1;
            if (command->argc == 12) {
              params.tls.authMode = atoi(command->argv[APITEST_CMD_PARAM_2]);
              params.tls.authContext = atoi(command->argv[APITEST_CMD_PARAM_3]);
              params.protocol.keepAliveTime = atoi(command->argv[APITEST_CMD_PARAM_4]);
              params.protocol.cleanSessionFlag =
                  atoi(command->argv[APITEST_CMD_PARAM_5]) != 0 ? 1 : 0;
              strncpy(params.nodes.url, command->argv[APITEST_CMD_PARAM_6],
                      sizeof(params.nodes.url));
              params.ip.destPort = atoi(command->argv[APITEST_CMD_PARAM_7]);

              if (strncmp(command->argv[APITEST_CMD_PARAM_8], APITEST_NULL_ARG,
                          strlen(APITEST_NULL_ARG))) {
                strncpy(params.nodes.clientId, command->argv[APITEST_CMD_PARAM_8],
                        sizeof(params.nodes.clientId));
              }

              if (strncmp(command->argv[APITEST_CMD_PARAM_9], APITEST_NULL_ARG,
                          strlen(APITEST_NULL_ARG))) {
                strncpy(params.nodes.username, command->argv[APITEST_CMD_PARAM_9],
                        sizeof(params.nodes.username));
              }

              if (strncmp(command->argv[APITEST_CMD_PARAM_10], APITEST_NULL_ARG,
                          strlen(APITEST_NULL_ARG))) {
                strncpy(params.nodes.password, command->argv[APITEST_CMD_PARAM_10],
                        sizeof(params.nodes.password));
              }

              gSession = altcom_mqttSessionCreate(&params);
              print_retval(gSession != NULL, APITEST_GETFUNCNAME(altcom_mqttSessionCreate));
            }
          } else {
            APITEST_DBG_PRINT("session already created, please delete first\n");
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "delete", strlen("delete"));
        if (cmp_res == 0) {
          if (command->argc == 3) {
            if (gSession) {
              print_retval(MQTT_SUCCESS == altcom_mqttSessionDelete(gSession),
                           APITEST_GETFUNCNAME(altcom_mqttSessionDelete));
              gSession = NULL;
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "conn", strlen("conn"));
        if (cmp_res == 0) {
          if (command->argc == 3) {
            if (gSession) {
              print_retval(
                  MQTT_SUCCESS == altcom_mqttConnect(gSession, mqttMessageCb, (void *)0xDEADBEEF),
                  APITEST_GETFUNCNAME(altcom_mqttConnect));
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "disconn", strlen("disconn"));
        if (cmp_res == 0) {
          if (command->argc == 3) {
            if (gSession) {
              print_retval(MQTT_SUCCESS == altcom_mqttDisconnect(gSession),
                           APITEST_GETFUNCNAME(altcom_mqttDisconnect));
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "sub", strlen("sub"));
        if (cmp_res == 0) {
          if (command->argc == 5) {
            if (gSession) {
              print_retval(
                  MQTT_FAILURE != altcom_mqttSubscribe(
                                      gSession, (MQTTQoS_e)atoi(command->argv[APITEST_CMD_PARAM_2]),
                                      command->argv[APITEST_CMD_PARAM_3]),
                  APITEST_GETFUNCNAME(altcom_mqttSubscribe));
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "unsub", strlen("unsub"));
        if (cmp_res == 0) {
          if (command->argc == 4) {
            if (gSession) {
              print_retval(MQTT_FAILURE !=
                               altcom_mqttUnsubscribe(gSession, command->argv[APITEST_CMD_PARAM_2]),
                           APITEST_GETFUNCNAME(altcom_mqttUnsubscribe));
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
          }
        }

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "pub", strlen("pub"));
        if (cmp_res == 0) {
          if (command->argc == 6) {
            if (gSession) {
              print_retval(
                  MQTT_FAILURE != altcom_mqttPublish(
                                      gSession, (MQTTQoS_e)atoi(command->argv[APITEST_CMD_PARAM_2]),
                                      0, command->argv[APITEST_CMD_PARAM_3],
                                      command->argv[APITEST_CMD_PARAM_4],
                                      strlen(command->argv[APITEST_CMD_PARAM_4])),
                  APITEST_GETFUNCNAME(altcom_mqttPublish));
            } else {
              APITEST_DBG_PRINT("invalid session handle\n");
            }
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
