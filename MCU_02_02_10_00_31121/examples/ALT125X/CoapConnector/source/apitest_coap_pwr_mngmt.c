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
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "alt_osal.h"
#include "coap/altcom_coap.h"
#include "apicmd_cmd_urc.h"

/* Power managment includes */
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "hifc_api.h"
#include "sleep_mngr.h"
#include "sleep_notify.h"

/* App includes */
#include "apitest_main.h"
#include "apitest_coap_pwr_mngmt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

/****************************************************************************
 * Private Data
 ****************************************************************************/
static osSemaphoreId_t cbsem = NULL;
static uint8_t Send_status = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Sleep enabling/disabling */
static void set_sleep_enable(int on) {
  pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0);
  pwr_mngr_enable_sleep(on ? 1 : 0);
}

/* Suspending CB */
int32_t sleep_callback(sleep_notify_state state, PWR_MNGR_PwrMode mode, void *arg) {
  printf("\r\n%s\r\n", (state == SUSPENDING) ? "\tSUSPENDING" : "\tRESUMING");
  printf("\r\nPower Mode:%lu\r\n", (uint32_t)mode);
  assert(mode == PWR_MNGR_MODE_STANDBY || mode == PWR_MNGR_MODE_STOP);

  return SLEEP_NOTIFY_SUCCESS;
}

/* Load Coap configuration from GPM */
static void load_conf_param(CoapCfgContext_t *context) {
  /* IP */
  snprintf(context->coapCfgIp.dest_addr, strlen(URL) + 1, URL);
  context->coapCfgIp.ip_type = (Coap_ip_type)IP_TYPE;
  context->coapCfgIp.sessionId = DEFAULT;

  /* Security */
  context->coapCfgDtls.dtls_profile = (Coap_dtls_mode_e)COAP_DTLS_PROFILE_1;
  context->coapCfgDtls.dtls_mode = (Coap_dtls_mode_e)COAP_DTLS_MODE;
  context->coapCfgDtls.auth_mode = (Coap_dtls_auth_mode_e)COAP_AUTH_MODE;
  context->coapCfgDtls.session_resumption = (Coap_dtls_session_resumption_e)COAP_RESUMPTION;
  *(context->coapCfgDtls.CipherListFilteringType) =
      (Coap_cypher_filtering_type_e)COAP_CIPHER_WHITE_LIST;

  snprintf(context->coapCfgDtls.CipherList, strlen(CIPHERLIST) + 1, CIPHERLIST);
  context->uriMask = (Coap_option_uri_e)URI_MASK;
  context->protocols = (int)COAP_PROTOCOL;
}

/************** URC registration and callback functions *********************/

static void urc_term_cb(uint8_t profile, Coap_ev_cmdterm_e err, char *token, void *priv) {
  uint8_t *status = (uint8_t *)priv;

  if (status && *status == 0) {
    *status = (uint8_t)err;
    osSemaphoreRelease(cbsem);
  }
}

static void urc_rst_cb(uint8_t profile, char *token, void *priv) {
  uint8_t *status = (uint8_t *)priv;

  if (status && *status == 0) {
    *status = (uint8_t)-1;
    osSemaphoreRelease(cbsem);
  }
}

/* Print out URC values */
static void urc_cmd_cb(uint8_t profile, void *data, void *priv) {
  CoapCallBackData_t *urc_data = (CoapCallBackData_t *)data;
  uint8_t *status = (uint8_t *)priv;

  if (status && *status == 0) {
    *status = (uint8_t)urc_data->rspCode;
    osSemaphoreRelease(cbsem);
  }
}

static void register_urc(void) {
  /* Unregister callback first */
  coap_EventRegister(COAP_CB_CMDS, NULL, NULL);
  coap_EventRegister(COAP_CB_CMDRST, NULL, NULL);
  coap_EventRegister(COAP_CB_CMDTERM, NULL, NULL);

  /* Register callback */
  coap_EventRegister(COAP_CB_CMDS, (void *)urc_cmd_cb, &Send_status);
  coap_EventRegister(COAP_CB_CMDRST, (void *)urc_rst_cb, &Send_status);
  coap_EventRegister(COAP_CB_CMDTERM, (void *)urc_term_cb, &Send_status);
}

/**************** Data Allocations and Releases **************/

/* Coap network configuration allocations */
Coap_err_code allocate_config_data(CoapCfgContext_t *context) {
  Coap_err_code ret = COAP_FAILURE;

  context->coapCfgIp.dest_addr = malloc(strlen(URL) + 1);
  if (!context->coapCfgIp.dest_addr) {
    COAP_DBG_PRINT("No room for destination address");
    return ret;
  }

  context->coapCfgDtls.CipherListFilteringType = malloc(sizeof(Coap_cypher_filtering_type_e));
  if (!context->coapCfgDtls.CipherListFilteringType) {
    COAP_DBG_PRINT("No room for CipherListFilteringType\r\n");
    return ret;
  }

  context->coapCfgDtls.CipherList = malloc(strlen(CIPHERLIST) + 1);
  if (!context->coapCfgDtls.CipherList) {
    COAP_DBG_PRINT("No room for CipherListFilteringType\r\n");
    return ret;
  }
  return COAP_SUCCESS;
}

void free_config_data(CoapCfgContext_t *context) {
  if (context->coapCfgDtls.CipherListFilteringType)
    free(context->coapCfgDtls.CipherListFilteringType);

  if (context->coapCfgDtls.CipherList) free(context->coapCfgDtls.CipherList);

  free(context->coapCfgIp.dest_addr);
}

/* Coap commands configuration allocations */
Coap_err_code coap_command_allocate(CoapCmdData_t *CmdParams) {
  int i;
  Coap_err_code ret = COAP_FAILURE;

  CmdParams->data = malloc(DATA_LEN);
  if (!CmdParams->data) {
    COAP_DBG_PRINT("No room for payload\r\n");
    return ret;
  }

  /* Options */
  CmdParams->optionsArgV =
      (CoapCmdOption_type_value_t **)malloc(sizeof(CoapCmdOption_type_value_t *) * OPTION_NB);
  if (!CmdParams->optionsArgV) {
    COAP_DBG_PRINT("No room for CoapCmdOption_type_value_t \r\n");
    return ret;
  }

  for (i = 0; i < OPTION_NB; i++) {
    CmdParams->optionsArgV[i] =
        (CoapCmdOption_type_value_t *)malloc(sizeof(CoapCmdOption_type_value_t));
    if (!CmdParams->optionsArgV[i]) {
      COAP_DBG_PRINT("No room for CoapCmdOption_type_value_t\r\n");
      return ret;
    }

    CmdParams->optionsArgV[i]->option_value = malloc(strlen(OPTION_VALUE) + 1);
    if (!CmdParams->optionsArgV[i]->option_value) {
      COAP_DBG_PRINT("No room for option_value\r\n");
      return ret;
    }
  }

  /* Block's */
  CmdParams->blk_size = malloc(sizeof(unsigned short));
  if (!CmdParams->blk_size) {
    COAP_DBG_PRINT("No room for block size\r\n");
    return ret;
  }
  CmdParams->block_num = malloc(sizeof(int));
  if (!CmdParams->block_num) {
    COAP_DBG_PRINT("No room for block_num\r\n");
    return ret;
  }
  CmdParams->block_m = malloc(sizeof(Coap_pending_block_e));
  if (!CmdParams->block_m) {
    COAP_DBG_PRINT("No room for block M\r\n");
    return ret;
  }

  return COAP_SUCCESS;
}

void coap_command_free(CoapCmdData_t *CmdParams) {
  int i;
  if (CmdParams->data) free(CmdParams->data);
  for (i = 0; i < CmdParams->optionsArgc; i++) {
    free(CmdParams->optionsArgV[i]->option_value);
    free(CmdParams->optionsArgV[i]);
  }

  free(CmdParams->optionsArgV);
  free(CmdParams->blk_size);
  free(CmdParams->block_num);
  free(CmdParams->block_m);
}

/******* Command line arg's utilities */

static bool param_field_alloc(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    argv[i] = malloc(CMD_PARAM_LENGTH);
    if (!argv[i]) {
      printf("cmd field alloc failed.\r\n");
      return false;
    }
    memset(argv[i], 0, CMD_PARAM_LENGTH);
  }
  return true;
}

static void param_field_free(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    if (argv[i]) {
      free(argv[i]);
    }
  }
  return;
}

static int parse_arg(char *s, char *argv[]) {
  int argc = 1; /* At least, include "apitest" or other */
  char *tmp_s;
  int i;

  /* Parse arg */
  if (s && strlen(s)) {
    for (i = 1; i < CMD_NUM_OF_PARAM_MAX; i++) {
      tmp_s = strchr(s, ' ');
      if (!tmp_s) {
        tmp_s = strchr(s, '\0');
        if (!tmp_s) {
          printf("Invalid arg.\r\n");
          break;
        }
        memcpy(argv[i], s, tmp_s - s);
        break;
      }
      memcpy(argv[i], s, tmp_s - s);
      s += tmp_s - s + 1;
    }
    argc += i;
  }

  return argc;
}

/* Configure Coap commands configurations
 *
 * Configuration is based on #define'd parameters for easy modifications
 * Commands are PUT or GET and impact blockwise params and payload.
 */

static Coap_err_code coap_command_params(CoapCmdData_t *CmdParams, coap_cmd_method_t cmd,
                                         int packet_no) {
  int len, i;
  Coap_err_code ret = COAP_FAILURE;

  /* Default command configurations */
  CmdParams->uri = URI;
  CmdParams->confirm = (Coap_cmd_confirm_e)CONFIRMABLE;
  CmdParams->token = TOKEN;
  CmdParams->content = CONTENT;
  *(CmdParams->blk_size) = (unsigned short)BLK_SIZE;

  if (cmd == COAP_CMD_PUT) {
    CmdParams->cmd = (coap_cmd_method_t)COAP_CMD_PUT;
    CmdParams->data_len = (unsigned short)DATA_LEN;
  } else if (cmd == COAP_CMD_GET) {
    CmdParams->cmd = (coap_cmd_method_t)COAP_CMD_GET;
    CmdParams->data_len = 0;
  }

  switch (packet_no) {
    case 1:
      *(CmdParams->block_num) = (unsigned int)BLK_NO_1;
      *(CmdParams->block_m) = (Coap_pending_block_e)COAP_CMD_BLK_MORE_BLOCK;
      break;
    case 2:
      *(CmdParams->block_num) = (unsigned int)BLK_NO_2;
      *(CmdParams->block_m) = (Coap_pending_block_e)COAP_CMD_BLK_MORE_BLOCK;
      break;
    case 3:
      *(CmdParams->block_num) = (unsigned int)BLK_NO_3;
      *(CmdParams->block_m) = (Coap_pending_block_e)COAP_CMD_BLK_LAST_BLOCK;
      break;
  }

  /* Payload */
  if (cmd == COAP_CMD_PUT) {
    if (packet_no >= PKT_1 && packet_no <= PKT_3)
      memset(CmdParams->data, packet_no, DATA_LEN);
    else {
      COAP_DBG_PRINT("Only 3 blocks are supported\r\n");
      return ret;
    }
  }

  /* Options */
  CmdParams->optionsArgc = OPTION_NB;
  for (i = 0; i < OPTION_NB; i++) {
    if (CmdParams->optionsArgV && CmdParams->optionsArgV[i]) {
      CmdParams->optionsArgV[i]->option_type = (Coap_option_predefined_e)OPTION_TYPE;
      len = strlen(OPTION_VALUE);
      snprintf(CmdParams->optionsArgV[i]->option_value, len + 1, OPTION_VALUE);
    }
  }
  return COAP_SUCCESS;
}

/* Generic command for sending a packet.
 *
 * Each command expect incoming URC and this is where pwr management is enabled.
 */

static Coap_err_code send_packet(uint8_t profile_id, coap_cmd_method_t cmd,
                                 CoapCmdData_t *CmdParams, int packet_no, enum SM current) {
  Coap_err_code res = COAP_FAILURE;
  Coap_Ev_resp_code_e expected = (cmd == COAP_CMD_PUT ? EV_RESP_CHANGED : EV_RESP_CONTENT);
  unsigned int CurrentTime, ElapseTime;

  COAP_DBG_PRINT("\tSending packet - Packet#: %d\r\n", packet_no);
  /* Load commands parameters based on packet # and command type */
  if ((res = coap_command_params(CmdParams, cmd, packet_no)) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure in coap command configuration - Packet#: %d\r\n", current);
    return res;
  }

  /* Send it */
  CurrentTime = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
  res = altcom_coap_cmd(profile_id, CmdParams);
  COAP_DBG_PRINT("\taltcom_coap_cmd: ");
  if (res == COAP_SUCCESS)
    COAP_DBG_PRINT("OK\r\n");
  else
    COAP_DBG_PRINT("FAIL\r\n");

  ElapseTime = (osKernelGetTickCount() / OS_TICK_PERIOD_MS) - CurrentTime;
  COAP_DBG_PRINT("\tSend packet: %d ms\r\n", ElapseTime);

  CurrentTime = osKernelGetTickCount() / OS_TICK_PERIOD_MS;

  osStatus_t status;

  status = osSemaphoreAcquire(cbsem, TIMEOUT / OS_TICK_PERIOD_MS);
  if (status == osOK) {
    ElapseTime = (osKernelGetTickCount() / OS_TICK_PERIOD_MS) - CurrentTime;
    COAP_DBG_PRINT("\tUrc received: %d ms\r\n", ElapseTime);
  } else if (status == osErrorTimeout) {
    COAP_DBG_PRINT("\tTimeout  - Packet: %d\r\n", current);
    return COAP_FAILURE;
  }

  if (Send_status != expected) {
    if (Send_status != 0) {
      COAP_DBG_PRINT("Cmd: %d - Error : %d  - Expected: %d - Packet #: %d\r\n", cmd, Send_status,
                     expected, current);
      return COAP_FAILURE;
    }
  }

  Send_status = (uint8_t)0;
  return COAP_SUCCESS;
}

/* Serie of multiple packets (PUT or GET) */
static Coap_err_code send_all_packets(uint8_t profile_id, coap_cmd_method_t cmd,
                                      CoapCmdData_t *CmdParams) {
  Coap_err_code res = COAP_FAILURE;
  if (send_packet(profile_id, cmd, CmdParams, PKT_1,
                  (cmd == COAP_CMD_PUT ? put_packet1 : get_packet1)) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure sending packet1\r\n");
    return res;
  }
  if (send_packet(profile_id, cmd, CmdParams, PKT_2,
                  (cmd == COAP_CMD_PUT ? put_packet2 : get_packet2)) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure sending packet2\r\n");
    return res;
  }
  if (send_packet(profile_id, cmd, CmdParams, PKT_3,
                  (cmd == COAP_CMD_PUT ? put_packet3 : get_packet3)) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure sending packet3\r\n");
    return res;
  }
  return COAP_SUCCESS;
}

/* The main part
 *
 * flag '-p' indicates to run the demo with power management support
 */
void do_Coap_PwrMngmt_demo(char *s) {
  uint8_t profile_id = (uint8_t)COAP_PROFILE_1;
  Coap_err_code res;
  CoapCfgContext_t context;
  CoapCmdData_t CmdParams;
  int argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};
  unsigned int CurrentTime, ElapseTime;

  if (cbsem == NULL) {
    cbsem = osSemaphoreNew(1, 0, NULL);
    assert(cbsem != NULL);
  }

  if (false == param_field_alloc(argv)) {
    COAP_DBG_PRINT("param_field_alloc fail\r\n");
    return;
  }

  int pwr_mngmt;

  pwr_mngmt = 0;
  argc = parse_arg(s, argv);
  if (argc == 2 && s) {
    if (!strncmp(s, "-p", 2)) pwr_mngmt = 1;
  }

  param_field_free(argv);

  if (pwr_mngmt) {
    set_sleep_enable(1);
  }

  /* URC registration */
  register_urc();

  int iteration;

start:
  iteration = START_ITERATION;
  while (iteration) {
    COAP_DBG_PRINT("\r\n****** Iteration: %d\r\n", START_ITERATION - iteration + 1);

    /* Allocate data */
    memset(&context, 0, sizeof(context));
    memset(&CmdParams, 0, sizeof(CmdParams));
    if (allocate_config_data(&context) == COAP_FAILURE) {
      COAP_DBG_PRINT("Failure allocating config data\r\n");
      return;
    }

    /* Clear selected profile */
    res = altcom_coap_clear_profile(profile_id);
    COAP_DBG_PRINT("\taltcom_coap_clear_profile: ");
    if (res == COAP_SUCCESS)
      COAP_DBG_PRINT("OK\r\n");
    else {
      COAP_DBG_PRINT("FAIL\r\n");
      break;
    }

    /* Load network context */
    load_conf_param(&context);
    /* Send configuration to COAP */
    CurrentTime = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
    res = altcom_coap_set_config(profile_id, &context);
    ElapseTime = (osKernelGetTickCount() / OS_TICK_PERIOD_MS) - CurrentTime;

    COAP_DBG_PRINT("\taltcom_coap_set_config: ");
    if (res == COAP_SUCCESS)
      COAP_DBG_PRINT("OK\r\n");
    else {
      COAP_DBG_PRINT("FAIL\r\n");
      break;
    }

    COAP_DBG_PRINT("\tSend configuration: %d ms\r\n", ElapseTime);

    free_config_data(&context);

    /* Data allocation for command */
    coap_command_allocate(&CmdParams);

    /* Clear send status */
    Send_status = 0;

    /* Send series of PUT packets */
    if (send_all_packets(profile_id, COAP_CMD_PUT, &CmdParams) == COAP_FAILURE) {
      COAP_DBG_PRINT("Failure with PUT packets\r\n");
      coap_command_free(&CmdParams);
      pwr_mngmt = 0;
      goto end_demo;
    }

    /* Send series of GET packets */
    if (send_all_packets(profile_id, COAP_CMD_GET, &CmdParams) == COAP_FAILURE) {
      COAP_DBG_PRINT("Failure with GET packets\r\n");
      coap_command_free(&CmdParams);
      pwr_mngmt = 0;
      goto end_demo;
    }

    COAP_DBG_PRINT("\tEnd of session\r\n");
    coap_command_free(&CmdParams);
    iteration--;
    if (iteration) {
      osDelay(7000 / OS_TICK_PERIOD_MS);
    }
  }

end_demo:
  COAP_DBG_PRINT("**** End of Demo *****\r\n");
  if (pwr_mngmt) {
    osDelay(7000 / OS_TICK_PERIOD_MS);
    goto start;
  } else {
    set_sleep_enable(0);
  }
}
