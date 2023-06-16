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
#include <ctype.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "alt_osal.h"
#include "altcom.h"
#include "http/altcom_http.h"
#include "certmgmt/altcom_certmgmt.h"

/* App includes */
#include "apitest_main.h"
#include "demo_http.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APITEST_RESULT_OK (0)
#define APITEST_RESULT_ERROR (1)

/* Certificate management */
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

#define IS_STR_EQ(cmd, arg_no) (strncmp(command->argv[arg_no], #cmd, strlen(#cmd)) == 0)
#define ISNOT_STR_EQ(cmd, arg_no) (strncmp(command->argv[arg_no], #cmd, strlen(#cmd)) != 0)

#define http_free(p) \
  do {               \
    free(p);         \
    p = NULL;        \
  } while (0)

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
 * Private Functions
 ****************************************************************************/

static void swap_chars(char *str) {
  char *pr = str, *pw = str;
  while (*pr) {
    *pw = *pr++;
    if (*pw == '@') *pw = ' ';
    pw++;
  }
  *pw = '\0';
}

static void print_retval(bool val, FAR char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\r\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\r\n", str, val);
  }
}

/* Number of parameters */
typedef enum {
  APITEST_CMD_CLEAR_ARG = 3,
  APITEST_CMD_ABORT_ARG = 3,
  APITEST_CMD_CFGNODE_ARG = 7,
  APITEST_CMD_CFGIP_ARG = 7,
  APITEST_CMD_CFGFORMAT_ARG = 5,
  APITEST_CMD_CFGTIMEOUT_ARG = 4,
  APITEST_CMD_CFGTLS_ARG = 8,
  APITEST_CMD_SENDCMD_ARG = 8,
  APITEST_CMD_READDATA_ARG = 4
} Http_arg_no_e;

static void apitest_task(FAR void *arg) {
  BaseType_t ret;
  struct apitest_command_s *command;
  uint8_t profileId = 0;
  char *str_ptr;
  Http_err_code_e result = HTTP_FAILURE;
  int Strlen = 0;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\r\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until commands are sent */
    result = HTTP_FAILURE;

    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\r\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\r\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\r\n");
    }

    if (command && command->argc >= 1) {
      APITEST_DBG_PRINT("command->argv[APITEST_CMD_ARG] -> %s\r\n", command->argv[APITEST_CMD_ARG]);

      /* Certificate management - Read command */

      if (IS_STR_EQ(CERT, APITEST_CMD_ARG) || IS_STR_EQ(cert, APITEST_CMD_ARG)) {
        if (IS_STR_EQ(READ, APITEST_CMD_PARAM_1) || IS_STR_EQ(read, APITEST_CMD_PARAM_1)) {
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
                APITEST_DBG_PRINT("read malloc failed!\r\n");
                goto command_exit;
              }
            } else {
              credBuf = (void *)0xDEADBEEF;
            }

            print_retval(CERTMGMT_SUCCESS == (result = altcom_ReadCredential(credName, credBuf,
                                                                             credBufLen, &credLen)),
                         APITEST_GETFUNCNAME(altcom_ReadCredential));
            if (CERTMGMT_SUCCESS == result) {
              APITEST_DBG_PRINT(
                  "Credential Name: %s\r\nCredential Length: %hu\r\n===Credential "
                  "Content===\r\n%.*s\r\n=====================\r\n",
                  credName, credLen, (int)credLen, credBuf);
            } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
              APITEST_DBG_PRINT("credBuf length: %hu, user provide length: %hu\r\n", credLen,
                                credBufLen);
            }

            if (0 != credBufLen) {
              free(credBuf);
            }

            credBuf = NULL;
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
            goto command_exit;
          }
        }
      }

      /* Certificate management - Write command */

      if (IS_STR_EQ(CERT, APITEST_CMD_ARG) || IS_STR_EQ(cert, APITEST_CMD_ARG)) {
        if (IS_STR_EQ(WRITE, APITEST_CMD_PARAM_1) || IS_STR_EQ(write, APITEST_CMD_PARAM_1)) {
          if (5 == command->argc) {
            char *credName;
            CredentialType_e credType;

            if (0 == credDataLen || NULL == credBuf) {
              APITEST_DBG_PRINT(
                  "Credential data not ready! Please use \"writecred\" to import credential.\r\n");
              goto command_exit;
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
            APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
            goto command_exit;
          }
        }
      }

      /* Certificate management - Delete command */

      if (IS_STR_EQ(CERT, APITEST_CMD_ARG) || IS_STR_EQ(cert, APITEST_CMD_ARG)) {
        if (IS_STR_EQ(DELETE, APITEST_CMD_PARAM_1) || IS_STR_EQ(delete, APITEST_CMD_PARAM_1)) {
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
            APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
            goto command_exit;
          }
        }
      }

      /* Certificate management - Gtlist command */

      if (IS_STR_EQ(CERT, APITEST_CMD_ARG) || IS_STR_EQ(cert, APITEST_CMD_ARG)) {
        if (IS_STR_EQ(GTLIST, APITEST_CMD_PARAM_1) || IS_STR_EQ(gtlist, APITEST_CMD_PARAM_1)) {
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
                APITEST_DBG_PRINT("gtlist malloc failed!\r\n");
                goto command_exit;
              }
            } else {
              credBuf = (void *)0xDEADBEEF;
            }

            print_retval(CERTMGMT_SUCCESS == (result = altcom_GetCredentialList(
                                                  caPath, credBuf, listBufLen, &listLen)),
                         APITEST_GETFUNCNAME(altcom_GetCredentialList));
            if (CERTMGMT_SUCCESS == result) {
              APITEST_DBG_PRINT(
                  "Credential Path: %s\r\nCredential List Length: %hu\r\n===Credential "
                  "List===\r\n%.*s\r\n=====================\r\n",
                  CAPATH_ROOT == caPath ? "ROOT" : "USER", listLen, (int)listLen, credBuf);
            } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
              APITEST_DBG_PRINT("listBuf length: %hu, user provide length: %hu\r\n", listLen,
                                listBufLen);
            }

            if (0 != listBufLen) {
              free(credBuf);
            }

            credBuf = NULL;
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
            goto command_exit;
          }
        }
      }

      /* Certificate management - Config command */

      if (IS_STR_EQ(CERT, APITEST_CMD_ARG) || IS_STR_EQ(cert, APITEST_CMD_ARG)) {
        if (IS_STR_EQ(CONFIG, APITEST_CMD_PARAM_1) || IS_STR_EQ(config, APITEST_CMD_PARAM_1)) {
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
              APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
              goto command_exit;
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
            APITEST_DBG_PRINT("unexpected parameter number %d\r\n", command->argc);
            goto command_exit;
          }
        }
      }

      /* HTTP - Clear command */
      if (IS_STR_EQ(CLEAR, APITEST_CMD_ARG) || IS_STR_EQ(clear, APITEST_CMD_ARG)) {
        if (command->argc != APITEST_CMD_CLEAR_ARG) {
          APITEST_DBG_PRINT("Syntax: apitest clear <profile_id>\r\n");
          goto command_exit;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        print_retval(HTTP_SUCCESS == (result = altcom_http_clear_profile(profileId)),
                     APITEST_GETFUNCNAME(altcom_http_clear_profile));

        APITEST_DBG_PRINT("altcom_http_clear_profile: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - Abort command */
      if (IS_STR_EQ(ABORT, APITEST_CMD_ARG) || IS_STR_EQ(abort, APITEST_CMD_ARG)) {
        if (command->argc != APITEST_CMD_ABORT_ARG) {
          APITEST_DBG_PRINT("Syntax: apitest abort <profile_id>\r\n");
          goto command_exit;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        print_retval(HTTP_SUCCESS == (result = altcom_http_abort_profile(profileId)),
                     APITEST_GETFUNCNAME(altcom_http_abort_profile));

        APITEST_DBG_PRINT("altcom_http_abort_profile: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - Node configuration command */
      if (IS_STR_EQ(CFGNODE, APITEST_CMD_ARG) || IS_STR_EQ(cfgnode, APITEST_CMD_ARG)) {
        if (command->argc < APITEST_CMD_CFGNODE_ARG) {
          APITEST_DBG_PRINT(
              "Syntax: apitest CFGNODE <profile> <URL> <user|X> <password|X> <encoding|X>\r\n");
          goto command_exit;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        /* Destination address - Mandatory */
        Strlen = strlen(command->argv[APITEST_CMD_PARAM_2]);
        if (Strlen == 0) {
          APITEST_DBG_PRINT("Destination address is mandatory\r\n");
          goto command_exit;
        }

        Http_config_node_t http_node_config;

        http_node_config.dest_addr = malloc(Strlen + 1);
        if (http_node_config.dest_addr == NULL) {
          APITEST_DBG_PRINT("No room for destination address");
          return;
        }

        snprintf(http_node_config.dest_addr, Strlen + 1, command->argv[APITEST_CMD_PARAM_2]);

        /* Username & Password (Optional) */
        http_node_config.user = NULL;
        http_node_config.passwd = NULL;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_3)) {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_3]);
          http_node_config.user = malloc(Strlen + 1);
          if (http_node_config.user == NULL) {
            APITEST_DBG_PRINT("No room for userName");
            http_node_config.user = NULL;
            http_free(http_node_config.dest_addr);
            return;
          }
          snprintf(http_node_config.user, Strlen + 1, command->argv[APITEST_CMD_PARAM_3]);

          /* Password */
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_4]);
          if (Strlen == 0) {
            APITEST_DBG_PRINT("Password is required\r\n");
            http_free(http_node_config.user);
            http_free(http_node_config.dest_addr);
            goto command_exit;
          }
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_4]);
          http_node_config.passwd = malloc(Strlen + 1);
          if (http_node_config.passwd == NULL) {
            APITEST_DBG_PRINT("No room for password");
            http_node_config.passwd = NULL;
            http_free(http_node_config.dest_addr);
            http_free(http_node_config.user);
            return;
          }
          snprintf(http_node_config.passwd, Strlen + 1, command->argv[APITEST_CMD_PARAM_4]);
        }

        /* Format encryption (Optional) - default is plain text */
        http_node_config.encode_format = HTTP_ENC_PLAIN_TEXT;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          http_node_config.encode_format = strtol(command->argv[APITEST_CMD_PARAM_5], &str_ptr, 10);
        }

        print_retval(
            HTTP_SUCCESS == (result = altcom_http_node_config(profileId, &http_node_config)),
            APITEST_GETFUNCNAME(altcom_http_node_config));

        APITEST_DBG_PRINT("altcom_http_node_config: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");

        http_free(http_node_config.dest_addr);
        http_free(http_node_config.user);
        http_free(http_node_config.passwd);
      }

      /* HTTP - IP configuration command */
      if (IS_STR_EQ(CFGIP, APITEST_CMD_ARG) || IS_STR_EQ(cfgip, APITEST_CMD_ARG)) {
        if (command->argc < APITEST_CMD_CFGIP_ARG) {
          APITEST_DBG_PRINT(
              "Syntax: apitest CFGIP <profile> <session_id|X> <ip_type|X> <dest_port#|X> "
              "<src_port#|X>\r\n");
          goto command_exit;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        Http_config_ip_t http_ip_config;

        /* Session ID (Optional) - default = 0 */
        http_ip_config.sessionId = 0;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_2)) {
          http_ip_config.sessionId = strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        }

        /* Ip type (Optional) - default = HTTP_IPTYPE_V4V6 */
        http_ip_config.ip_type = HTTP_IPTYPE_V4V6;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_3)) {
          http_ip_config.ip_type = strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);
        }

        /* Source port (Optional) - default = 0 */
        http_ip_config.Source_port_nb = 0;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_4)) {
          http_ip_config.Source_port_nb = strtol(command->argv[APITEST_CMD_PARAM_4], &str_ptr, 10);
        }

        /* Destination port (Optional) - default = 0 */
        http_ip_config.Destination_port_nb = 0;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          http_ip_config.Destination_port_nb =
              strtol(command->argv[APITEST_CMD_PARAM_5], &str_ptr, 10);
        }

        print_retval(HTTP_SUCCESS == (result = altcom_http_ip_config(profileId, &http_ip_config)),
                     APITEST_GETFUNCNAME(altcom_http_ip_config));

        APITEST_DBG_PRINT("altcom_http_ip_profile: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - Format configuration command */
      if (IS_STR_EQ(CFGFORMAT, APITEST_CMD_ARG) || IS_STR_EQ(cfgformat, APITEST_CMD_ARG)) {
        if (command->argc < APITEST_CMD_CFGFORMAT_ARG) {
          APITEST_DBG_PRINT(
              "Syntax: apitest CFGFORMAT <profile> <response_header|X> <request_header|X>\r\n");
          goto command_exit;
        }

        /* Profile ID */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        Http_config_format_t http_format_config;

        /* Response header presence (Optional) - default HTTP_RESP_HEADER_PRESENCE_DISABLE */
        http_format_config.respHeaderPresent = HTTP_RESP_HEADER_PRESENCE_DISABLE;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_2)) {
          http_format_config.respHeaderPresent =
              strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        }

        /* Request header presence (Optional) - default HTTP_REQ_HEADER_PRESENCE_DISABLE */
        http_format_config.reqHeaderPresent = HTTP_REQ_HEADER_PRESENCE_DISABLE;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_3)) {
          http_format_config.reqHeaderPresent =
              strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);
        }

        print_retval(
            HTTP_SUCCESS == (result = altcom_http_format_config(profileId, &http_format_config)),
            APITEST_GETFUNCNAME(altcom_http_format_config));

        APITEST_DBG_PRINT("altcom_http_format_config: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - Timeout configuration command */
      if (IS_STR_EQ(CFGTIMEOUT, APITEST_CMD_ARG) || IS_STR_EQ(cfgtimeout, APITEST_CMD_ARG)) {
        /* TIMEOUT configuration */
        if (command->argc < APITEST_CMD_CFGTIMEOUT_ARG) {
          APITEST_DBG_PRINT("Syntax: apitest CFGTIMEOUT <profile> <timeout|X>\r\n");
          goto command_exit;
        }

        /* Profile ID */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        Http_config_timeout_t http_timeout_config;

        /* Timeout configuration (Optional) - default = 120 sec  */
        http_timeout_config.timeout = 120;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_2)) {
          http_timeout_config.timeout = strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        }

        print_retval(
            HTTP_SUCCESS == (result = altcom_http_timeout_config(profileId, &http_timeout_config)),
            APITEST_GETFUNCNAME(altcom_http_timeout_config));

        APITEST_DBG_PRINT("altcom_http_timeout_config: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - READ command */
      if (IS_STR_EQ(READDATA, APITEST_CMD_ARG) || IS_STR_EQ(readdata, APITEST_CMD_ARG)) {
        /* Read data command */
        if (command->argc < APITEST_CMD_READDATA_ARG) {
          APITEST_DBG_PRINT("Syntax: apitest READDATA <profile> <chunklen|X>\r\n");
          goto command_exit;
        }

        /* Profile ID */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        /*Chunk length configuration  */
        uint16_t chunk_len = 3000;

        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_2)) {
          chunk_len = (uint16_t)strtoul(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        }

        uint8_t *buf;
        uint16_t read_len;
        uint32_t pend_len;

        read_len = 0;
        buf = malloc((size_t)chunk_len);
        if (buf == NULL) {
          APITEST_DBG_PRINT("No room to store data");
          goto command_exit;
        }

        print_retval(HTTP_SUCCESS == (result = altcom_http_read_data(profileId, chunk_len, buf,
                                                                     &read_len, &pend_len)),
                     APITEST_GETFUNCNAME(altcom_http_read_data));

        APITEST_DBG_PRINT("altcom_http_read_data: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
        if (result == HTTP_SUCCESS) {
          APITEST_DBG_PRINT("chunk_len: %hu, read_len: %hu pend_len: %lu\r\n", chunk_len, read_len,
                            pend_len);
          hexdump(buf, (uint32_t)read_len);
        }

        free(buf);
      }

      /* HTTP - TLS configuration command */
      if (IS_STR_EQ(CFGTLS, APITEST_CMD_ARG) || IS_STR_EQ(cfgtls, APITEST_CMD_ARG)) {
        if (command->argc < APITEST_CMD_CFGTLS_ARG) {
          APITEST_DBG_PRINT(
              "Syntax: apitest CFGTLS "
              "<profile> <profile_tls> <authentication|X> <session_resumption|X> "
              "<cipher_filtering|X> <cipherList|X>\r\n");
          goto command_exit;
        }

        /* Profile ID */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        Http_config_tls_t http_tls_config;

        /* TLS profile - mandatory */
        if (command->argv[APITEST_CMD_PARAM_2] != NULL) {
          http_tls_config.profile_tls = strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        } else {
          APITEST_DBG_PRINT("Tls profile is mandatory\r\n");
          goto command_exit;
        }

        /* Authentication (Optional) - default = HTTP_MUTUAL_AUTH */
        http_tls_config.authentication_type = HTTP_MUTUAL_AUTH;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_3)) {
          http_tls_config.authentication_type =
              strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);
        }

        /* Session resumption (Optional) - default = HTTP_TLS_RESUMP_SESSION_DISABLE */
        http_tls_config.session_resumption = HTTP_TLS_RESUMP_SESSION_DISABLE;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_4)) {
          http_tls_config.session_resumption =
              strtol(command->argv[APITEST_CMD_PARAM_4], &str_ptr, 10);
        }

        /* Cipher type (Optional) - default = HTTP_CIPHER_NONE_LIST */
        http_tls_config.CipherListFilteringType = HTTP_CIPHER_NONE_LIST;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          http_tls_config.CipherListFilteringType =
              strtol(command->argv[APITEST_CMD_PARAM_5], &str_ptr, 10);
        }

        /* Cipher list (Optional) - default = NULL
         * cipher are delimited with ';
         */

        http_tls_config.CipherList = NULL;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_6)) {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_6]);
          http_tls_config.CipherList = malloc(Strlen + 1);
          if (http_tls_config.CipherList == NULL) {
            APITEST_DBG_PRINT("No room for CipherList");
            http_tls_config.CipherList = NULL;
            return;
          }

          snprintf(http_tls_config.CipherList, Strlen + 1, command->argv[APITEST_CMD_PARAM_6]);
        }

        print_retval(HTTP_SUCCESS == (result = altcom_http_tls_config(profileId, &http_tls_config)),
                     APITEST_GETFUNCNAME(altcom_http_tls_config));

        APITEST_DBG_PRINT("altcom_http_tls_config: %s\r\n",
                          (result == HTTP_SUCCESS) ? "OK" : "FAIL");
      }

      /* HTTP - Commands configuration */
      if (IS_STR_EQ(SENDCMD, APITEST_CMD_ARG) || IS_STR_EQ(sendcmd, APITEST_CMD_ARG)) {
        /* HTTP command sending */
        if (command->argc < APITEST_CMD_SENDCMD_ARG) {
          APITEST_DBG_PRINT(
              "Syntax: apitest SENDCMD "
              "<profile> <cmd> <data_len> <pending_data> <headers|X> <payload|X>\r\n");
          goto command_exit;
        }
        /* Profile ID - mandatory */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        Http_command_data_t http_send_command;

        /* HTTP command - mandatory */
        if (IS_STR_EQ(PUT, APITEST_CMD_PARAM_2) || IS_STR_EQ(put, APITEST_CMD_PARAM_2))
          http_send_command.cmd = HTTP_CMD_PUT;
        if (IS_STR_EQ(POST, APITEST_CMD_PARAM_2) || IS_STR_EQ(post, APITEST_CMD_PARAM_2))
          http_send_command.cmd = HTTP_CMD_POST;
        if (IS_STR_EQ(GET, APITEST_CMD_PARAM_2) || IS_STR_EQ(get, APITEST_CMD_PARAM_2))
          http_send_command.cmd = HTTP_CMD_GET;
        if (IS_STR_EQ(DELETE, APITEST_CMD_PARAM_2) || IS_STR_EQ(delete, APITEST_CMD_PARAM_2))
          http_send_command.cmd = HTTP_CMD_DELETE;

        /* Data length - Optional field but mandatory for PUT/POST */
        http_send_command.data_len = strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);

        /* Pending data - Optional field but mandatory for PUT/POST */
        http_send_command.pending_data = strtol(command->argv[APITEST_CMD_PARAM_4], &str_ptr, 10);

        /* Headers - Optional default = NULL */
        http_send_command.headers = NULL;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_5]);
          http_send_command.headers = malloc(Strlen + 1);
          if (http_send_command.headers == NULL) {
            APITEST_DBG_PRINT("No room for command headers");
            return;
          }

          snprintf(http_send_command.headers, Strlen + 1, command->argv[APITEST_CMD_PARAM_5]);
          swap_chars(http_send_command.headers);
          APITEST_DBG_PRINT("headers: %s\r\n", http_send_command.headers);
        }

        /* Payload - Optional field but mandatory for PUT/POST */
        http_send_command.data_to_send = NULL;
        if (ISNOT_STR_EQ(X, APITEST_CMD_PARAM_6)) {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_6]);
          http_send_command.data_to_send = malloc(Strlen + 1);
          if (http_send_command.data_to_send == NULL) {
            APITEST_DBG_PRINT("No room for command data_to_send");
            http_free(http_send_command.headers);
            return;
          }
          snprintf((char *)http_send_command.data_to_send, Strlen + 1,
                   command->argv[APITEST_CMD_PARAM_6]);
        }

        /* Callback registration */
        altcom_http_event_register(profileId, HTTP_CMD_SESTERM_EV, NULL, NULL);
        if (http_send_command.cmd == HTTP_CMD_PUT) {
          altcom_http_event_register(profileId, HTTP_CMD_PUTCONF_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_PRSPRCV_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_PUTCONF_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
          altcom_http_event_register(profileId, HTTP_CMD_PRSPRCV_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
        }

        if (http_send_command.cmd == HTTP_CMD_POST) {
          altcom_http_event_register(profileId, HTTP_CMD_POSTCONF_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_PRSPRCV_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_POSTCONF_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
          altcom_http_event_register(profileId, HTTP_CMD_PRSPRCV_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
        }

        if (http_send_command.cmd == HTTP_CMD_DELETE) {
          altcom_http_event_register(profileId, HTTP_CMD_DELCONF_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_DELCONF_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
        }

        if (http_send_command.cmd == HTTP_CMD_GET) {
          altcom_http_event_register(profileId, HTTP_CMD_GETRCV_EV, NULL, NULL);
          altcom_http_event_register(profileId, HTTP_CMD_GETRCV_EV, (void *)http_event_cb,
                                     (void *)0xDEADBEEF);
        }

        altcom_http_event_register(profileId, HTTP_CMD_SESTERM_EV, (void *)http_event_cb,
                                   (void *)0xDEADBEEF);

        print_retval(HTTP_SUCCESS == (result = altcom_http_send_cmd(profileId, &http_send_command)),
                     APITEST_GETFUNCNAME(altcom_http_send_cmd));

        APITEST_DBG_PRINT("altcom_http_send_cmd: %s\r\n", (result == HTTP_SUCCESS) ? "OK" : "FAIL");

        if (http_send_command.headers) {
          http_free(http_send_command.headers);
        }

        if (http_send_command.data_to_send) {
          http_free(http_send_command.data_to_send);
        }
      } /* SENDCMD */

      if (IS_STR_EQ(help, APITEST_CMD_ARG) || IS_STR_EQ(HELP, APITEST_CMD_ARG)) {
        APITEST_DBG_PRINT("CLEAR Syntax:\r\n\tapitest CLEAR <profile_id>\r\n");
        APITEST_DBG_PRINT("ABORT Syntax:\r\n\tapitest ABORT <profile_id>\r\n");
        APITEST_DBG_PRINT(
            "CFGNODE Syntax:\r\n\tapitest CFGNODE "
            "<profile> <URL> <user|X> <password|X> <encoding|X>\r\n");
        APITEST_DBG_PRINT(
            "CFGIP Syntax:\r\n\tapitest CFGIP <profile> <session_id|X> <ip_type|X> <dest_port#|X> "
            "<src_port#|X>\r\n");
        APITEST_DBG_PRINT(
            "CFGFORMAT Syntax:\r\n\tapitest CFGFORMAT "
            "<profile> <response_header|X> <request_header|X>\r\n");
        APITEST_DBG_PRINT("CFGTIMEOUT Syntax:\r\n\tapitest CFGTIMEOUT <profile> <timeout|X>\r\n");
        APITEST_DBG_PRINT(
            "CFGTLS Syntax:\r\n\tapitest CFGTLS "
            "<profile> <profile_tls> <authentication|X> <session_resumption|X> "
            "<cipher_filtering|X> <cipherList|X>\r\n");
        APITEST_DBG_PRINT(
            "SENDCMD Syntax:\r\n\tapitest SENDCMD "
            "<profile> <cmd> <data_len> <pending_data> <headers|X> <payload|X>\r\n");
        APITEST_DBG_PRINT("READDATA Syntax:\r\n\tapitest READDATA <profile> <chunklen|X>\r\n");
        APITEST_DBG_PRINT(
            "DEMO:\r\n\tHttp_demo <-p|X> <[0-4]|X>\r\n"
            "\t-p:Run Permanently\r\n"
            "\t0:Mutual Authentication\r\n"
            "\t1:Client Only Authentication\r\n"
            "\t2:Server Only Authentication\r\n"
            "\t3:No Authentication\r\n"
            "\t4:NO TLS(Default)\r\n");
      }
    }

  command_exit:
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
      APITEST_DBG_PRINT("app_log_mtx init failed\r\n");
      goto errout;
    }

    cmdq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(struct apitest_command_s *), NULL);
    if (!cmdq) {
      APITEST_DBG_PRINT("cmdq init failed\r\n");
      goto errout;
    }
  } else {
    APITEST_DBG_PRINT("App already initialized\r\n");
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
    APITEST_DBG_PRINT("App not yet initialized\r\n");
    return -1;
  }

  if (2 <= argc) {
    if (APITEST_MAX_NUMBER_OF_PARAM < argc) {
      APITEST_DBG_PRINT("too many arguments\r\n");
      return -1;
    }

    if ((command = malloc(sizeof(struct apitest_command_s))) == NULL) {
      APITEST_DBG_PRINT("malloc fail\r\n");
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
        APITEST_DBG_PRINT("osThreadNew failed\r\n");
        APITEST_FREE_CMD(command, command->argc);
        return -1;
      }
    }

    if (osOK != osMessageQueuePut(cmdq, (const void *)&command, 0, osWaitForever)) {
      APITEST_DBG_PRINT("osMessageQueuePut to apitest_task Failed!!\r\n");
      APITEST_FREE_CMD(command, command->argc);
    }
  }

  return 0;
}
