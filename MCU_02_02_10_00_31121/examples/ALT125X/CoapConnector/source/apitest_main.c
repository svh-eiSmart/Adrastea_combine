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
#include "coap/altcom_coap.h"
#include "apicmd_cmd_urc.h"

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
static CoapCfgContext_t context;
static int contextinit = 0;
static CoapCmdData_t CmdParams;
static int cmdparaminit = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static unsigned char hextobin(unsigned char digit) {
  unsigned char value;

  if ((toupper(digit) >= 'A') && (toupper(digit) <= 'F')) {
    value = (digit - 'A') + 10;
  } else if ((digit >= '0') && (digit <= '9')) {
    value = digit - '0';
  } else {
    value = 255;  // error - not handling this below
  }

  return value;
}

static void print_retval(bool val, FAR char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

typedef enum {
  COAP_CMD_CLEAR_ARG = 3,
  COAP_CMD_ABORT_ARG = 3,
  COAP_CMD_CONFIGIP_ARG = 5,
  COAP_CMD_CONFIGDTLS_ARG = 8,
  COAP_CMD_CONFIGURI_ARG = 4,
  COAP_CMD_SEND_CONFIG_ARG = 3,
  COAP_CMD_CMDPARAMS_ARG = 10,
  COAP_CMD_OPTIONS_ARG = 6,
  COAP_CMD_SENDCMD_ARG = 3,
} Coap_arg_no_t_e;

static void log_term_cb(uint8_t profile, uint8_t err, char *token, void *priv) {
  APITEST_DBG_PRINT("\n[CB_TERM_URC]\n");
  APITEST_DBG_PRINT("Profile: %d\n", profile);
  APITEST_DBG_PRINT("Err: %d\n", err);
  if (token) {
    APITEST_DBG_PRINT("token: %s\n", token);
  }

  APITEST_DBG_PRINT("priv: %p\n", priv);
  APITEST_DBG_PRINT("--------\n");
}

static void log_rst_cb(uint8_t profile, char *token, void *priv) {
  APITEST_DBG_PRINT("\n[CB_RST_URC]\n");
  APITEST_DBG_PRINT("Profile: %d\n", profile);
  if (token) {
    APITEST_DBG_PRINT("token: %s\n", token);
  }

  APITEST_DBG_PRINT("priv: %p\n", priv);
  APITEST_DBG_PRINT("--------\n");
}

/* Print out URC values */
static void log_cmd_cb(uint8_t profile, void *data, void *priv) {
  CoapCallBackData_t *urc_data = (CoapCallBackData_t *)data;
  int i, len;
  char *str_ptr, *str_id, *str_val;

  APITEST_DBG_PRINT("\n[CB_CMD_URC]\n");
  APITEST_DBG_PRINT("--------\n");
  APITEST_DBG_PRINT("Command: %d\n", (coap_cmd_method_t)urc_data->cmd);
  APITEST_DBG_PRINT("Profile: %d\n", urc_data->profileId);
  APITEST_DBG_PRINT("Response Code: %d\n", (Coap_Ev_resp_code_e)urc_data->rspCode);
  APITEST_DBG_PRINT("Status: %d\n", (Coap_Ev_err_e)urc_data->status);
  APITEST_DBG_PRINT("Payload length: %d\n", urc_data->dataLen);
  APITEST_DBG_PRINT("Content: %d\n", (Coap_cmd_sent_content_format_e)urc_data->content);
  APITEST_DBG_PRINT("Block size: %d\n", urc_data->blk_size);
  APITEST_DBG_PRINT("Block No: %d\n", urc_data->blk_num);
  APITEST_DBG_PRINT("Block M: %d\n", urc_data->blk_m);
  APITEST_DBG_PRINT("Option No.: %x\n", urc_data->optionsArgc);

  /* String type values */
  str_id = urc_data->opt_id;
  str_val = urc_data->opt_value;

  if (urc_data->optionsArgc) {
    for (i = 0; i < urc_data->optionsArgc; i++) {
      len = strlen(str_id);
      APITEST_DBG_PRINT("#%d - ID: %ld\n", i, strtol(str_id, &str_ptr, 10));
      str_id += len + NULLCHAR;
      len = strlen(str_val);
      APITEST_DBG_PRINT("#%d - Value: %s\n", i, str_val);
      str_val += len + NULLCHAR;
    }
  } else {
    APITEST_DBG_PRINT("Options: N/A\n");
  }

  len = urc_data->tokenLen;
  if (len) {
    APITEST_DBG_PRINT("Token: %s\n", urc_data->token);
  }

  if (urc_data->dataLen) {
    APITEST_DBG_PRINT("Payload: ");
    for (i = 0; i < urc_data->dataLen; i++) {
      APITEST_DBG_PRINT("%02x", (uint8_t)urc_data->payload[i]);
    }

    APITEST_DBG_PRINT("\n");
  } else {
    APITEST_DBG_PRINT("Payload: N/A\n");
  }

  APITEST_DBG_PRINT("priv: %p\n", priv);
  APITEST_DBG_PRINT("--------\n");
}

static void apitest_task(FAR void *arg) {
  int i, Strlen = 0;
  BaseType_t ret;
  struct apitest_command_s *command;
  uint8_t profileId = 0;
  char *str_ptr;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\n");
    while (1) {
      ;
    }
  }

  while (1) {
    /* keep waiting until send commands */
    Coap_err_code result = COAP_FAILURE;

    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }
    if (command && command->argc >= 1) {
      APITEST_DBG_PRINT("command->argv[APITEST_CMD_ARG] -> %s\n", command->argv[APITEST_CMD_ARG]);
      if (!contextinit) {
        memset(&context, 0, sizeof(context));
      }

      if (!cmdparaminit) {
        memset(&CmdParams, 0, sizeof(CmdParams));
      }

      if (IS_STR_EQ(CLEAR, APITEST_CMD_ARG)) {
        /**********  C L E A R  *****************/
        /* Send CLEAR command for a specific profile */
        if (command->argc != COAP_CMD_CLEAR_ARG) {
          APITEST_DBG_PRINT("Clear command syntax is apitest clear <profile_id>\n");
          goto command_out;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);
        print_retval(COAP_SUCCESS == (result = altcom_coap_clear_profile(profileId)),
                     APITEST_GETFUNCNAME(altcom_coap_clear_profile));
        APITEST_DBG_PRINT("altcom_coap_clear_profile: %s\n",
                          (result == COAP_SUCCESS) ? "OK" : "FAIL");

        /**********  A B O R T  *****************/
        /* Send ABORT command for a specific profile */

      } else if (IS_STR_EQ(ABORT, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_ABORT_ARG) {
          APITEST_DBG_PRINT("Clear command syntax is apitest abort <profile_id>\n");
          goto command_out;
        }

        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);
        print_retval(COAP_SUCCESS == (result = altcom_coap_abort_profile(profileId)),
                     APITEST_GETFUNCNAME(altcom_coap_abort_profile));
        APITEST_DBG_PRINT("altcom_coap_abort_profile: %s\n",
                          (result == COAP_SUCCESS) ? "OK" : "FAIL");

        /**********  CONFIG IP  *****************/
        /* Configure IP parameters for COAP connection */

      } else if (IS_STR_EQ(CONFIGIP, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_CONFIGIP_ARG) {
          APITEST_DBG_PRINT(
              "Config IP command syntax is apitest configIp <url> <sessionId> <IP family> "
              "\n");
          goto command_out;
        }

        contextinit = 1;

        /* Destination address - Mandatory */
        Strlen = strlen(command->argv[APITEST_CMD_PARAM_1]);
        if (Strlen == 0) {
          APITEST_DBG_PRINT("Destination address is mandatory\n");
          goto command_out;
        }

        context.coapCfgIp.dest_addr = malloc(Strlen + 1);
        if (!context.coapCfgIp.dest_addr) {
          APITEST_DBG_PRINT("No room for destination address");
          context.coapCfgIp.dest_addr = NULL;
          goto command_out;
        }

        snprintf(context.coapCfgIp.dest_addr, Strlen + 1, command->argv[APITEST_CMD_PARAM_1]);

        /* SessionId */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_2)) {
          context.coapCfgIp.sessionId = NULL;
        } else {
          context.coapCfgIp.sessionId = malloc(sizeof(int));
          if (!context.coapCfgIp.sessionId) {
            APITEST_DBG_PRINT("No room for sessionId\n");
            goto exit_config;
          }

          *(context.coapCfgIp.sessionId) = strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        }

        /* IP Type */
        context.coapCfgIp.ip_type =
            (Coap_ip_type)strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);

        /**********  CONFIG DTLS  *****************/
        /* Security parameters */

      } else if (IS_STR_EQ(CONFIGDTLS, APITEST_CMD_ARG)) {
        if (command->argc < COAP_CMD_CONFIGDTLS_ARG) {
          APITEST_DBG_PRINT(
              "No enough parameters - Config dtls command syntax is apitest configDtls <dts "
              "profile> "
              "<auth "
              "mode><Cert mode> <Resumption> <CipherListFilteringType> <Cipherlist delimited with "
              ";>\n");
          goto exit_config;
        }
        /* DTLS profile */
        context.coapCfgDtls.dtls_profile =
            (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);  // Mention in Doc
        /* Auth Mode */
        context.coapCfgDtls.auth_mode =
            (Coap_dtls_auth_mode_e)strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);
        /* DTLS mode */
        context.coapCfgDtls.dtls_mode =
            (Coap_dtls_mode_e)strtol(command->argv[APITEST_CMD_PARAM_3], &str_ptr, 10);
        /* Resumption */
        context.coapCfgDtls.session_resumption = (Coap_dtls_session_resumption_e)strtol(
            command->argv[APITEST_CMD_PARAM_4], &str_ptr, 10);
        /* CipherListFilteringType */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          context.coapCfgDtls.CipherListFilteringType = NULL;
        } else {
          context.coapCfgDtls.CipherListFilteringType =
              malloc(sizeof(Coap_cypher_filtering_type_e));
          if (!context.coapCfgDtls.CipherListFilteringType) {
            APITEST_DBG_PRINT("No room for CipherListFilteringType\n");
            goto exit_config;
          }

          *(context.coapCfgDtls.CipherListFilteringType) = (Coap_cypher_filtering_type_e)strtol(
              command->argv[APITEST_CMD_PARAM_5], &str_ptr, 10);
        }

        /* CipherList */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_6)) {
          context.coapCfgDtls.CipherList = NULL;
        } else {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_6]) + 1;
          context.coapCfgDtls.CipherList = malloc(Strlen);
          if (!context.coapCfgDtls.CipherList) {
            APITEST_DBG_PRINT("No room for CipherListFilteringType\n");
            goto exit_config;
          }

          snprintf(context.coapCfgDtls.CipherList, Strlen, command->argv[APITEST_CMD_PARAM_6]);
        }

        /**********  URI MASK Configuration  *****************/

      } else if (IS_STR_EQ(CONFIGURI, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_CONFIGURI_ARG) {
          APITEST_DBG_PRINT(
              "Config Uri command syntax is apitest configUri <uri mask> <protocol>\n");
          goto exit_config;
        }

        /* URI mask */
        context.uriMask =
            (Coap_option_uri_e)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);
        /* Protocol */
        context.protocols = (int)strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);

        /* ****** Send configuration parameters *****************
         * Overall configurations are propagated via this command */

      } else if (IS_STR_EQ(SENDCONFIG, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_SEND_CONFIG_ARG) {
          APITEST_DBG_PRINT("Send Config command syntax is apitest SENDCONFIG <profileId>\n");
          goto exit_config;
        }

        /* ProfileId */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        print_retval(COAP_SUCCESS == (result = altcom_coap_set_config(profileId, &context)),
                     APITEST_GETFUNCNAME(altcom_coap_set_config));

        APITEST_DBG_PRINT("altcom_coap_set_config: %s\n", (result == COAP_SUCCESS) ? "OK" : "FAIL");
      exit_config:
        if (context.coapCfgIp.dest_addr) {
          free(context.coapCfgIp.dest_addr);
        }

        if (context.coapCfgDtls.CipherList) {
          free(context.coapCfgDtls.CipherList);
        }

        if (context.coapCfgDtls.CipherListFilteringType) {
          free(context.coapCfgDtls.CipherListFilteringType);
        }

        if (context.coapCfgIp.sessionId) {
          free(context.coapCfgIp.sessionId);
        }

        contextinit = 0;

        /* This part is for the COAP Command - GET - POST - PUT - DELETE  */

        /**********  CMD PARAMS  *****************/

      } else if (IS_STR_EQ(CMDPARAMS, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_CMDPARAMS_ARG) {
          APITEST_DBG_PRINT(
              "No enough parameters Send command parameters command syntax is apitest CMDPARAMS "
              "<profile id> <uri> "
              "<confirm> <token> <content> <data_len> <block size> <block no> <block m> \n");
          goto command_out;
        }

        cmdparaminit = 1;

        /* URL - Optional */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_1)) {
          CmdParams.uri = NULL;
        } else {
          Strlen = strlen(command->argv[APITEST_CMD_PARAM_1]);
          CmdParams.uri = NULL;
          CmdParams.uri = malloc(Strlen + 1);
          if (!CmdParams.uri) {
            APITEST_DBG_PRINT("No room for CmdParams.uri\n");
            goto command_exit;
          }

          snprintf(CmdParams.uri, Strlen, command->argv[APITEST_CMD_PARAM_1]);
        }

        /* Confirm */
        CmdParams.confirm =
            (Coap_cmd_confirm_e)strtol(command->argv[APITEST_CMD_PARAM_2], &str_ptr, 10);

        /* Token */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_3)) {
          CmdParams.token = NULL;
        } else {
          CmdParams.token = malloc(sizeof(CoapToken_t));
          if (!CmdParams.token) {
            APITEST_DBG_PRINT("No room for token\n");
            goto command_exit;
          }

          Strlen = strlen(command->argv[APITEST_CMD_PARAM_3]);
          if (Strlen == 0) {
            APITEST_DBG_PRINT("Empty token\n");
            goto command_exit;
          }

          CmdParams.token->token = malloc(Strlen + 1);
          if (!CmdParams.token->token) {
            APITEST_DBG_PRINT("No room for token\n");
            goto command_exit;
          }

          snprintf((char *)CmdParams.token->token, Strlen + 1, command->argv[APITEST_CMD_PARAM_3]);
          CmdParams.token->token_length = strlen(command->argv[APITEST_CMD_PARAM_3]);
        }

        /* Content */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_4))
          CmdParams.content = NULL;
        else {
          CmdParams.content = malloc(sizeof(Coap_cmd_sent_content_format_e));
          if (!CmdParams.content) {
            APITEST_DBG_PRINT("No room for content\n");
            goto command_exit;
          }

          *(CmdParams.content) = (Coap_cmd_sent_content_format_e)strtol(
              command->argv[APITEST_CMD_PARAM_4], &str_ptr, 10);
        }

        /* Data length */
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_5)) {
          CmdParams.data_len = 0;
        } else {
          CmdParams.data_len =
              (unsigned short)strtol(command->argv[APITEST_CMD_PARAM_5], &str_ptr, 10);
        }

        /* Block are set all together otherwise all are default
         Block size */
        int block_default = 0;
        if (IS_STR_EQ(X, APITEST_CMD_PARAM_6)) {
          CmdParams.blk_size = NULL;
          block_default = 1;
        } else {
          CmdParams.blk_size = malloc(sizeof(unsigned short));
          if (!CmdParams.blk_size) {
            APITEST_DBG_PRINT("No room for block size\n");
            goto command_exit;
          }

          *(CmdParams.blk_size) =
              (unsigned short)strtol(command->argv[APITEST_CMD_PARAM_6], &str_ptr, 10);
          if (!IS_STR_EQ(X, APITEST_CMD_PARAM_7) && block_default == 0) {
            CmdParams.block_num = malloc(sizeof(int));
            if (!CmdParams.block_num) {
              APITEST_DBG_PRINT("No room for block number\n");
              goto command_exit;
            }

            *(CmdParams.block_num) = (int)strtol(command->argv[APITEST_CMD_PARAM_7], &str_ptr, 10);
          }

          if (!IS_STR_EQ(X, APITEST_CMD_PARAM_8) && block_default == 0) {
            CmdParams.block_m = malloc(sizeof(Coap_pending_block_e));
            if (!CmdParams.block_m) {
              APITEST_DBG_PRINT("No room for block M\n");
              goto command_exit;
            }

            *(CmdParams.block_m) =
                (Coap_pending_block_e)strtol(command->argv[APITEST_CMD_PARAM_8], &str_ptr, 10);
          }
        }
      }
      /**********  CMD DATA/ OPTIONS  *****************/
      else if (IS_STR_EQ(CMDOPTIONS, APITEST_CMD_ARG)) {
        if (command->argc < COAP_CMD_OPTIONS_ARG) {
          APITEST_DBG_PRINT(
              "Not enough parameters - command options parameters command syntax is apitest CMDOPTIONS   \
    	            <command> <data> <Option-Id0> <Option-value0> .... <Option-Id3> <Option-value3> \n");
          goto command_exit;
        }

        /* Coap cmd */
        CmdParams.cmd = (coap_cmd_method_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        int argv_ptr = APITEST_CMD_PARAM_2;

        CmdParams.data = NULL;
        if (CmdParams.data_len > 0) {
          /* Payload */
          CmdParams.data = malloc(CmdParams.data_len);
          if (!CmdParams.data) {
            APITEST_DBG_PRINT("No room for payload\n");
            goto command_exit;
          }

          int ptr_bytes = 0;

          for (i = 0; i < CmdParams.data_len; i++) {
            CmdParams.data[i] = hextobin(command->argv[argv_ptr][ptr_bytes]) << 4u;
            CmdParams.data[i] |= hextobin(command->argv[argv_ptr][ptr_bytes + 1]) & 0xfu;
            ptr_bytes += 2;
          }
        }

        argv_ptr++;

        if (IS_STR_EQ(X, argv_ptr))
          CmdParams.optionsArgc = 0;
        else {
          CmdParams.optionsArgc = (command->argc - 4) / 2;  // APITEST + CMDOPTIONS + CMD + data = 4
          CmdParams.optionsArgV = NULL;
          CmdParams.optionsArgV = (CoapCmdOption_type_value_t **)malloc(
              sizeof(CoapCmdOption_type_value_t *) * CmdParams.optionsArgc);
          if (!CmdParams.optionsArgV) {
            APITEST_DBG_PRINT("No room for CoapCmdOption_type_value_t \n");
            goto command_exit;
          }

          for (i = 0; i < CmdParams.optionsArgc; i++) {
            CmdParams.optionsArgV[i] =
                (CoapCmdOption_type_value_t *)malloc(sizeof(CoapCmdOption_type_value_t));
            if (!CmdParams.optionsArgV[i]) {
              APITEST_DBG_PRINT("No room for CoapCmdOption_type_value_t\n");
              goto command_exit;
            }

            CmdParams.optionsArgV[i]->option_type =
                (Coap_option_predefined_e)strtol(command->argv[argv_ptr], &str_ptr, 10);
            argv_ptr++;
            Strlen = strlen(command->argv[argv_ptr]);
            if (Strlen == 0) {
              APITEST_DBG_PRINT("option_value is empty");
              goto command_exit;
            }

            CmdParams.optionsArgV[i]->option_value = malloc(Strlen + 1);
            if (!CmdParams.optionsArgV[i]->option_value) {
              APITEST_DBG_PRINT("No room for option_value\n");
              goto command_exit;
            }

            snprintf(CmdParams.optionsArgV[i]->option_value, Strlen + 1, command->argv[argv_ptr]);
            argv_ptr++;
          }
        }

        /**********  SEND COMMAND  *****************/
      } else if (IS_STR_EQ(SENDCMD, APITEST_CMD_ARG)) {
        if (command->argc != COAP_CMD_SENDCMD_ARG) {
          APITEST_DBG_PRINT(
              "Send command parameters command syntax is apitest CMDPARAMS <profile id>\n");
          goto command_exit;
        }

        /* Unregister callback first */
        coap_EventRegister(COAP_CB_CMDS, NULL, NULL);
        coap_EventRegister(COAP_CB_CMDRST, NULL, NULL);
        coap_EventRegister(COAP_CB_CMDTERM, NULL, NULL);

        /* Register callbacks */
        coap_EventRegister(COAP_CB_CMDS, (void *)log_cmd_cb, (void *)0xDEADBEEF);
        coap_EventRegister(COAP_CB_CMDRST, (void *)log_rst_cb, (void *)0xDEADBEEF);
        coap_EventRegister(COAP_CB_CMDTERM, (void *)log_term_cb, (void *)0xDEADBEEF);

        /* ProfileId */
        profileId = (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], &str_ptr, 10);

        print_retval(COAP_SUCCESS == (result = altcom_coap_cmd(profileId, &CmdParams)),
                     APITEST_GETFUNCNAME(altcom_coap_cmd));

        APITEST_DBG_PRINT("altcom_coap_cmd: %s\n", (result == COAP_SUCCESS) ? "OK" : "FAIL");

      command_exit:
        if (CmdParams.uri) {
          free(CmdParams.uri);
        }

        if (CmdParams.token) {
          if (CmdParams.token->token) {
            free(CmdParams.token->token);
          }

          free(CmdParams.token);
        }

        if (CmdParams.content) {
          free(CmdParams.content);
        }

        if (CmdParams.blk_size) {
          free(CmdParams.blk_size);
        }

        if (CmdParams.block_num) {
          free(CmdParams.block_num);
        }

        if (CmdParams.block_m) {
          free(CmdParams.block_m);
        }

        if (CmdParams.data) {
          free(CmdParams.data);
        }

        int i;

        for (i = 0; i < CmdParams.optionsArgc; i++) {
          if (!CmdParams.optionsArgV) {
            break;
          }

          if (CmdParams.optionsArgV[i]) {
            free(CmdParams.optionsArgV[i]);
          }

          if (CmdParams.optionsArgV[i]->option_value) {
            free(CmdParams.optionsArgV[i]->option_value);
          }
        }
        if (CmdParams.optionsArgV) {
          free(CmdParams.optionsArgV);
        }

        cmdparaminit = 0;
      }
    }

  command_out:
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
