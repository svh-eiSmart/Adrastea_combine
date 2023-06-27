/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include "altcom_coap.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "apicmd_cmd_urc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern coapCmdEvCb g_coapCmdEvCb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_urc_handling_job
 *
 * Description:
 *   This function is an API callback for event report receive.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void cmd_urc_handling_job(void *arg) {
  coapCmdEvCb callback;
  struct apicmd_coapCmdUrc_s *incoming = NULL;
  CoapCallBackData_t *urc_data = NULL;
  uint8_t i;
  int len;
  char *str_incoming = NULL, *str_id, *str_val;
  void *cbpriv;

  incoming = (struct apicmd_coapCmdUrc_s *)arg;

  /* Allocate structure for returned URC */
  urc_data = (CoapCallBackData_t *)BUFFPOOL_ALLOC(sizeof(CoapCallBackData_t));
  if (!urc_data) {
    DBGIF_LOG_DEBUG("No room for urc_data\n");
    altcom_free_cmd((FAR uint8_t *)arg);
    return;
  }
  memset(urc_data, 0, sizeof(CoapCallBackData_t));

  /* Copy the values */
  urc_data->optionsArgc = ntohl(incoming->optionsArgc);
  urc_data->dataLen = ntohs(incoming->dataLen);
  urc_data->cmd = (coap_cmd_method_t)incoming->cmd;
  urc_data->profileId = incoming->profileId;
  urc_data->rspCode = (Coap_Ev_resp_code_e)incoming->rspCode;
  urc_data->status = (Coap_Ev_err_e)incoming->status;
  urc_data->content = (Coap_cmd_sent_content_format_e)incoming->content;
  urc_data->blk_m = ntohl(incoming->blk_m);
  urc_data->blk_num = ntohl(incoming->blk_num);
  urc_data->blk_size = ntohl(incoming->blk_size);

  /* Extraction of add'l elements stored in a char array according the following scheme:
   *
   * OptionId0 + NULL -  Short + NULL
   * OptionVal0 + NULL - len(OptionVal0) + NULL
   * ..............
   * OptionId(n) + NULL - Short + NULL
   * OptionVal(n) + NULL- len(OptionValn) + NULL
   * Token + NULL
   * Payload   - Payload concerted from hexa to raw for saving room
   *
   * After conversion: New format is :
   *
   * urc_data->opt_id : option id0 + null + id1 + null ....
   * urc_data->opt_value: option value0 + null + option value1 + null ....
   * urc_data->token: Token (8 bytes + null)
   * urc_data->payload: Payload 0:1023.
   */

  /* Copy string type values */
  str_incoming = incoming->strData;
  str_id = urc_data->opt_id;
  str_val = urc_data->opt_value;

  /* Option ID - Value */
  for (i = 0; i < urc_data->optionsArgc; i++) {
    len = strlen(str_incoming);
    snprintf(str_id, len + NULLCHAR, str_incoming);
    str_incoming += len + NULLCHAR;
    str_id += len + NULLCHAR;
    len = strlen(str_incoming);
    snprintf(str_val, len + NULLCHAR, str_incoming);
    str_incoming += len + NULLCHAR;
    str_val += len + NULLCHAR;
  }

  /* Token */
  len = ntohs(incoming->tokenLen);
  if (len && str_incoming) {
    snprintf(urc_data->token, COAP_URC_TOKEN_LENGTH, str_incoming);
    str_incoming += len + NULLCHAR;
  }

  /* Payload */
  if (str_incoming && urc_data->dataLen) {
    memcpy(urc_data->payload, str_incoming, urc_data->dataLen);
    urc_data->payload[urc_data->dataLen + 1] = 0;
  }

  /* Setup callback */
  ALTCOM_GET_CALLBACK(g_coapCmdEvCb, callback, urc_data, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_coapCmdEvCb is not registered.\n");
    goto release;
  }

  callback(incoming->profileId, cbpriv);

release:
  BUFFPOOL_FREE(urc_data);
  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_cmdevt
 *
 * Description:
 *   This function is an API command handler for COAP URC handling.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_REPORT_CELLINFO,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_cmdevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_COAP_CMD_URC, cmd_urc_handling_job);
}
