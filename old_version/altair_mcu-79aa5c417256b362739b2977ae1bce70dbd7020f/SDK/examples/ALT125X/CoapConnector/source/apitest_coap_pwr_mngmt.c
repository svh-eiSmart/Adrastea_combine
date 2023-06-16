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

#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

__attribute__((section("GPMdata"))) command_handler_t command_handler;
__attribute__((section("GPMdata"))) int iteration;
__attribute__((section("GPMdata"))) int pwr_mngmt;
__attribute__((section("GPMdata"))) enum SM stateM;

/**************  GPM functions ****************/

static void setup(command_handler_t handler) { command_handler = handler; }

/* Power management statistics utilities */

static void counter_prevent_sleep(void) {
  PWR_MNGR_PwrCounters counters;
  int res;

  res = pwr_mngr_get_prevent_sleep_cntr(&counters);
  if (res == 0) {
    if (counters.sleep_disable)
      printf("\tPower manager disable               ---> %lu\r\n", counters.sleep_disable);
    if (counters.uart_incativity_time)
      printf("\tCLI inactivity time                 ---> %lu\r\n", counters.uart_incativity_time);
    if (counters.hifc_busy)
      printf("\tHiFC channel busy                   ---> %lu\r\n", counters.hifc_busy);
    if (counters.mon_gpio_busy)
      printf("\tMonitored GPIO busy                 ---> %lu\r\n", counters.mon_gpio_busy);
    if (counters.sleep_manager)
      printf("\tSleep manager total                 ---> %lu\r\n", counters.sleep_manager);

    pwr_mngr_clr_prevent_sleep_cntr();
  } else {
    printf("Error: Failed to get sleep counters!\r\n");
  }
}

static void do_sleepStat(void) {
  DRV_PM_Statistics statistics;
  char boot_type_str[20] = {0}, cause_str[20] = {0};
  int res;

  res = DRV_PM_GetStatistics(&statistics);
  if (res == 0) {
    switch (statistics.boot_type) {
      case DRV_PM_DEV_COLD_BOOT:
        strncpy(boot_type_str, "Cold", sizeof(boot_type_str) - 1);
        break;
      case DRV_PM_DEV_WARM_BOOT:
        strncpy(boot_type_str, "Warm", sizeof(boot_type_str) - 1);
        break;
      default:
        strncpy(boot_type_str, "Unknown", sizeof(boot_type_str) - 1);
        break;
    }
    switch (statistics.last_cause) {
      case DRV_PM_WAKEUP_CAUSE_NONE:
        strncpy(cause_str, "N/A", sizeof(cause_str) - 1);
        break;
      case DRV_PM_WAKEUP_CAUSE_TIMEOUT:
        strncpy(cause_str, "Timeout", sizeof(cause_str) - 1);
        break;
      case DRV_PM_WAKEUP_CAUSE_IO_ISR:
        strncpy(cause_str, "IO interrupt", sizeof(cause_str) - 1);
        break;
      case DRV_PM_WAKEUP_CAUSE_MODEM:
        strncpy(cause_str, "Modem", sizeof(cause_str) - 1);
        break;
      default:
        strncpy(cause_str, "Unknown", sizeof(cause_str) - 1);
        break;
    }

    printf("\t------------------ Wakup information ------------------\r\n");
    printf("\tLast boot type           : %s (%d) \r\n", boot_type_str, statistics.boot_type);
    printf("\tLast wakeUp cause        : %s (%d) \r\n", cause_str, statistics.last_cause);
    printf("\tLast sleep duration left : %lu ms\r\n", statistics.last_dur_left);
    printf("\tWakeUp counter           : %ld \r\n", statistics.counter);
  } else {
    printf("Error: Failed to get sleep statistics!\r\n");
  }
  counter_prevent_sleep();
}

/* Sleep enabling/disabling */
static void set_sleep_enable(int on) {
  if (pwr_mngmt) {
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, STDBY);
    set_inactivity_timer(HIFC_TIMEOUT);
    pwr_mngr_enable_sleep(on ? 1 : 0);
  }
}

/* Suspending CB */
int32_t sleep_callback(sleep_notify_state state, void *arg) {
  unsigned int ElapseTime;
  printf("\r\n%s\r\n", (state == SUSPENDING) ? "\tSUSPENDING" : "\tRESUMING");
  ElapseTime = (osKernelGetTickCount() / OS_TICK_PERIOD_MS) - CurrentTime;
  printf("\tElapsed time to suspend: %d ms\r\n", ElapseTime);

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

/* Cold boot flow
 *
 * Reset State machine
 * Initialize demo command for WARM resume
 * Put given data to payloads
 * Reset Iteration #
 */

void cold_saving_context() {
  COAP_DBG_PRINT("******* COLD BOOT**************\r\n");
  stateM = coap_config;
  resume_session = 0;
  setup(do_Coap_PwrMngmt_demo);
  iteration = START_ITERATION;
}

/* Resuming flow
 *
 * Show stats.
 * Run demo command.
 */
int warm_restore_context() {
  COAP_DBG_PRINT("\t******* WARM BOOT**************\r\n");
  resume_session = 1;
  do_sleepStat();
  if (command_handler) {
    pwr_mngmt ? command_handler("-p") : command_handler(NULL);
    return 0;
  }

  return -1;
}

/************** URC registration and callback functions *********************/

static void urc_term_cb(uint8_t profile, Coap_ev_cmdterm_e err, char *token) {
  Send_status = (uint8_t)err;
}

static void urc_rst_cb(uint8_t profile, char *token) { Send_status = (uint8_t)-1; }

/* Print out URC values */
static void urc_cmd_cb(uint8_t profile, void *userPriv) {
  CoapCallBackData_t *urc_data = (CoapCallBackData_t *)userPriv;
  //  printf("****** CB : cmd: %d status: %d\n", urc_data->cmd, urc_data->rspCode);
  Send_status = (uint8_t)urc_data->rspCode;
  memcpy(urc_recv_payload, urc_data->payload, urc_data->dataLen);
}

static void register_urc(void) {
  /* Register callback */
  coap_EventRegister((void *)urc_cmd_cb, COAP_CB_CMDS);
  coap_EventRegister((void *)urc_rst_cb, COAP_CB_CMDRST);
  coap_EventRegister((void *)urc_term_cb, COAP_CB_CMDTERM);
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
  int timeout = TIMEOUT;
  char payload[DATA_LEN];
  Coap_Ev_resp_code_e expected = (cmd == COAP_CMD_PUT ? EV_RESP_CHANGED : EV_RESP_CONTENT);
  unsigned int ElapseTime;

  /* Check if SM is OK in case of resume */
  if (stateM <= current) {
    //    printf("resume = %d\n", resume_session);
    if (resume_session == 0) {
      COAP_DBG_PRINT("\tSending packet - Packet#: %d\r\n", stateM);
      //    printf("Cmd: %d - Expected: %d stateM: %d current: %d\n", cmd, expected, stateM,
      //    current);
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

      timeout = TIMEOUT;
      CurrentTime = osKernelGetTickCount() / OS_TICK_PERIOD_MS;

      /* Check expected COAP status returned by URC */
      set_sleep_enable(1);
    }

    //   printf("Before while Send_status: %d\n", Send_status);
    while (--timeout && Send_status == (uint8_t)0) {
      osDelay(100 / OS_TICK_PERIOD_MS);
      counter_prevent_sleep();
    }

    set_sleep_enable(0);

    ElapseTime = (osKernelGetTickCount() / OS_TICK_PERIOD_MS) - CurrentTime;
    COAP_DBG_PRINT("\tUrc received: %d ms\r\n", ElapseTime);
    //   printf("After urc received expected: %d recv: %d\n", expected, Send_status);

    if (Send_status != expected) {
      if (timeout <= (uint8_t)0) {
        //     printf("TIMEOUT: Send_status: %d\n", Send_status);
        COAP_DBG_PRINT("\tTimeout  - Packet: %d\r\n", current);
        return res;
      }
      if (Send_status != 0) {
        COAP_DBG_PRINT("Cmd: %d - Error : %d  - Expected: %d - Packet #: %d\r\n", cmd, Send_status,
                       expected, current);
        return res;
      }
    }

    Send_status = (uint8_t)0;
    resume_session = 0;

    if (current == put_packet1 || current == get_packet1)
      memset(&payload, 1, DATA_LEN);
    else if (current == put_packet2 || current == get_packet2)
      memset(&payload, 2, DATA_LEN);
    else if (current == put_packet3 || current == get_packet3)
      memset(&payload, 3, DATA_LEN);
    stateM++;
  }
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
  unsigned int ElapseTime;

start:
  CmdParams.optionsArgc = 0;
  CmdParams.data = NULL;

  /* Indicates that the test ended  (START_ITERATION) */
  if (iteration == 0) goto end_demo;
  COAP_DBG_PRINT("\r\n****** Iteration: %d\r\n", iteration);

  if (false == param_field_alloc(argv)) goto end_command;
  argc = parse_arg(s, argv);
  pwr_mngmt = 0;
  if (argc == 2 && s) {
    if (!strncmp(s, "-p", 2)) pwr_mngmt = 1;
  }

  if (DRV_PM_GetDevBootType() == DRV_PM_DEV_WARM_BOOT)
    COAP_DBG_PRINT("\tLast state machine found after resuming: %d\r\n", stateM);

  if (stateM == coap_config) {
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
      iteration = 0;
      goto end_command;
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
      iteration = 0;
      goto end_command;
    }

    COAP_DBG_PRINT("\tSend configuration: %d ms\r\n", ElapseTime);

    free_config_data(&context);
    stateM++;
  }

  /* URC registration */
  register_urc();

  /* Data allocation for command */
  coap_command_allocate(&CmdParams);

  /* Send series of PUT packets */
  if (send_all_packets(profile_id, COAP_CMD_PUT, &CmdParams) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure with PUT packets\r\n");
    goto end_demo;
  }

  /* Send series of GET packets */
  if (send_all_packets(profile_id, COAP_CMD_GET, &CmdParams) == COAP_FAILURE) {
    COAP_DBG_PRINT("Failure with GET packets\r\n");
    goto end_demo;
  }

  iteration--;
  COAP_DBG_PRINT("\tEnd of session\r\n");

end_command:
  /* Preparing to restart the scenario unless all iterations are completed */
  coap_command_free(&CmdParams);
  param_field_free(argv);
  stateM = coap_config;
  if (iteration) goto start;

  /* This is the end of test.
   * Waiting for sleep-standby process
   */
  if (pwr_mngmt) {
    COAP_DBG_PRINT("\r\n\tEntering STANDBY\r\n");
    set_sleep_enable(1);
    osDelay(5000 / OS_TICK_PERIOD_MS);
  }

end_demo:
  set_sleep_enable(0);
  iteration = START_ITERATION;
  stateM = coap_config;
  resume_session = 0;

  COAP_DBG_PRINT("**** End of Demo *****\r\n");
}
