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
#include "certmgmt/altcom_certmgmt.h"

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

#define APITEST_MAX_NUMBER_OF_PARAM (16)
#define APITEST_MAX_API_MQUEUE (16)

#define APITEST_TASK_STACKSIZE (2048)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_INVALID_ARG ("INVALID")
#define APITEST_NULL_ARG ("NULL")
#define APITEST_CERTIFICATE ("CERTIFICATE")
#define APITEST_PRIVATEKEY ("PRIVATEKEY")
#define APITEST_PSKID ("PSKID")
#define APITEST_PSK ("PSK")
#define APITEST_TRUSTED_ROOT ("TRUSTEDROOT")
#define APITEST_TRUSTED_USER ("TRUSTEDUSER")
#define APITEST_UNDEF ("UNDEF")
#define APITEST_GET_PROFILE ("GET")
#define APITEST_ADD_PROFILE ("ADD")
#define APITEST_DELETE_PROFILE ("DELETE")

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

uint8_t *credBuf = NULL;
uint16_t credDataLen = 0;

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

static void apitest_task(void *arg) {
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
      if (!strncmp(command->argv[APITEST_CMD_ARG], "cert", strlen("cert"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "read", strlen("read"))) {
          if (5 == command->argc) {
            CertMgmt_Error_e result;
            char *credName;
            uint16_t credBufLen;
            uint16_t credLen;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              credName = NULL;
            } else {
              credName = command->argv[APITEST_CMD_PARAM_2];
            }

            credBufLen = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_3]);
            if (credBuf) {
              free(credBuf);
              credBuf = NULL;
            }

            if (0 != credBufLen) {
              credBuf = malloc(credBufLen);
              if (!credBuf) {
                APITEST_DBG_PRINT("read malloc failed!\n");
                goto errout_with_param;
              }
            } else {
              credBuf = (void *)0xDEADBEEF;
            }

            print_retval(CERTMGMT_SUCCESS == (result = altcom_ReadCredential(credName, credBuf,
                                                                             credBufLen, &credLen)),
                         APITEST_GETFUNCNAME(altcom_ReadCredential));
            if (CERTMGMT_SUCCESS == result) {
              APITEST_DBG_PRINT(
                  "Credential Name: %s\nCredential Length: %hu\n===Credential "
                  "Content===\n%.*s\n=====================\n",
                  credName, credLen, (int)credLen, credBuf);
            } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
              APITEST_DBG_PRINT("credBuf length: %hu, user provide length: %hu\n", credLen,
                                credBufLen);
            }

            if (0 != credBufLen) {
              free(credBuf);
            }

            credBuf = NULL;
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "cert", strlen("cert"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "write", strlen("write"))) {
          if (5 == command->argc) {
            char *credName;
            CredentialType_e credType;

            if (0 == credDataLen || NULL == credBuf) {
              APITEST_DBG_PRINT(
                  "Credential data not ready! Please use \"writecred\" to import credential.\n");
              goto errout_with_param;
            }

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_CERTIFICATE,
                         strlen(APITEST_CERTIFICATE))) {
              credType = CREDTYPE_CERT;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_PRIVATEKEY,
                                strlen(APITEST_PRIVATEKEY))) {
              credType = CREDTYPE_PRIVKEY;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_PSKID,
                                strlen(APITEST_PSKID))) {
              credType = CREDTYPE_PSKID;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_PSK,
                                strlen(APITEST_PSK))) {
              credType = CREDTYPE_PSK;
            } else {
              credType = CREDTYPE_MAX;
            }

            if (!strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              credName = NULL;
            } else {
              credName = command->argv[APITEST_CMD_PARAM_3];
            }

            print_retval(CERTMGMT_SUCCESS ==
                             altcom_WriteCredential(credType, credName, credBuf, credDataLen),
                         APITEST_GETFUNCNAME(altcom_WriteCredential));
            free(credBuf);
            credBuf = NULL;
            credDataLen = 0;

          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "cert", strlen("cert"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "delete", strlen("delete"))) {
          if (4 == command->argc) {
            char *credName;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                         strlen(APITEST_NULL_ARG))) {
              credName = NULL;
            } else {
              credName = command->argv[APITEST_CMD_PARAM_2];
            }

            print_retval(CERTMGMT_SUCCESS == altcom_DeleteCredential(credName),
                         APITEST_GETFUNCNAME(altcom_DeleteCredential));
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "cert", strlen("cert"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "gtlist", strlen("gtlist"))) {
          if (5 == command->argc) {
            CertMgmt_Error_e result;
            TrustedCaPath_e caPath;
            uint16_t listBufLen;
            uint16_t listLen;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_TRUSTED_ROOT,
                         strlen(APITEST_TRUSTED_ROOT))) {
              caPath = CAPATH_ROOT;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_TRUSTED_USER,
                                strlen(APITEST_TRUSTED_USER))) {
              caPath = CAPATH_USER;
            } else {
              caPath = CAPATH_USER + 1;
            }

            listBufLen = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_3]);
            if (credBuf) {
              free(credBuf);
              credBuf = NULL;
            }

            if (0 != listBufLen) {
              credBuf = malloc(listBufLen);
              if (!credBuf) {
                APITEST_DBG_PRINT("gtlist malloc failed!\n");
                goto errout_with_param;
              }
            } else {
              credBuf = (void *)0xDEADBEEF;
            }

            print_retval(CERTMGMT_SUCCESS == (result = altcom_GetCredentialList(
                                                  caPath, credBuf, listBufLen, &listLen)),
                         APITEST_GETFUNCNAME(altcom_GetCredentialList));
            if (CERTMGMT_SUCCESS == result) {
              APITEST_DBG_PRINT(
                  "Credential Path: %s\nCredential List Length: %hu\n===Credential "
                  "List===\n%.*s\n=====================\n",
                  CAPATH_ROOT == caPath ? "ROOT" : "USER", listLen, (int)listLen, credBuf);
            } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
              APITEST_DBG_PRINT("listBuf length: %hu, user provide length: %hu\n", listLen,
                                listBufLen);
            }

            if (0 != listBufLen) {
              free(credBuf);
            }

            credBuf = NULL;
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "cert", strlen("cert"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "config", strlen("config"))) {
          CertMgmt_Error_e result;
          ProfileCfgOp_e oper;
          uint8_t profileId;
          TrustedCaPath_e caPath = CAPATH_ROOT;
          char caNameBuf[CERTMGMT_MAX_FILENAME_LEN + 1];
          char *caName = caNameBuf;
          char certNameBuf[CERTMGMT_MAX_FILENAME_LEN + 1];
          char *certName = certNameBuf;
          char keyNameBuf[CERTMGMT_MAX_FILENAME_LEN + 1];
          char *keyName = keyNameBuf;
          char pskIdNameBuf[CERTMGMT_MAX_FILENAME_LEN + 1];
          char *pskIdName = pskIdNameBuf;
          char pskNameBuf[CERTMGMT_MAX_FILENAME_LEN + 1];
          char *pskName = pskNameBuf;

          if (5 <= command->argc) {
            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_GET_PROFILE,
                         strlen(APITEST_GET_PROFILE))) {
              oper = PROFCFG_GET;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_ADD_PROFILE,
                                strlen(APITEST_ADD_PROFILE))) {
              oper = PROFCFG_ADD;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_DELETE_PROFILE,
                                strlen(APITEST_DELETE_PROFILE))) {
              oper = PROFCFG_DELETE;
            } else {
              oper = PROFCFG_DELETE + 1;
            }

            profileId = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_3]);
            if (11 == command->argc) {
              if (!strncmp(command->argv[APITEST_CMD_PARAM_4], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
                caName = NULL;
              } else {
                caName = command->argv[APITEST_CMD_PARAM_4];
              }

              if (!strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_TRUSTED_ROOT,
                           strlen(APITEST_TRUSTED_ROOT))) {
                caPath = CAPATH_ROOT;
              } else if (!strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_TRUSTED_USER,
                                  strlen(APITEST_TRUSTED_USER))) {
                caPath = CAPATH_USER;
              } else if (!strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_UNDEF,
                                  strlen(APITEST_UNDEF))) {
                caPath = CAPATH_UNDEF;
              } else {
                caPath = CAPATH_UNDEF + 1;
              }

              if (!strncmp(command->argv[APITEST_CMD_PARAM_6], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
                certName = NULL;
              } else {
                certName = command->argv[APITEST_CMD_PARAM_6];
              }

              if (!strncmp(command->argv[APITEST_CMD_PARAM_7], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
                keyName = NULL;
              } else {
                keyName = command->argv[APITEST_CMD_PARAM_7];
              }

              if (!strncmp(command->argv[APITEST_CMD_PARAM_8], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
                pskIdName = NULL;
              } else {
                pskIdName = command->argv[APITEST_CMD_PARAM_8];
              }

              if (!strncmp(command->argv[APITEST_CMD_PARAM_9], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
                pskName = NULL;
              } else {
                pskName = command->argv[APITEST_CMD_PARAM_9];
              }
            }

            if (5 != command->argc && 11 != command->argc) {
              APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
              goto errout_with_param;
            } else {
              print_retval(CERTMGMT_SUCCESS == (result = altcom_ConfigCredProfile(
                                                    oper, profileId, caName, &caPath, certName,
                                                    keyName, pskIdName, pskName)),
                           APITEST_GETFUNCNAME(altcom_ConfigCredProfile));

              if (CERTMGMT_SUCCESS == result && PROFCFG_GET == oper) {
                char *caPathStr[] = {"ROOT", "USER", "UNDEF"};

                APITEST_DBG_PRINT(
                    "Profile ID: %lu\r\nRootCA File: %s\r\nCA Path: %s\r\nCert File: %s\r\nKey "
                    "File: %s\r\nPSK ID: %s\r\nPSK: %s\r\n",
                    (uint32_t)profileId, caName, caPathStr[caPath], certName, keyName, pskIdName,
                    pskName);
              }
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
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
