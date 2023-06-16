/****************************************************************************
 *
 *  (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.
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
#include <stdbool.h>
#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_http.h"
#include "apicmd_httpCommands.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_COMMANDS_RES_DATALEN (sizeof(struct apicmd_httpCommands_res_s))
#define HTTP_COMMANDS_REQ_DATALEN (sizeof(struct apicmd_httpCommands_s))
#define ISPROFILE_VALID(p) (p > 0 && p <= 5)
#define CMD_TIMEOUT 10000 /* 10 secs */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
Http_err_code_e altcom_http_send_cmd(Http_profile_id_e profile_id,
                                     Http_command_data_t *http_cmd_data) {
  struct apicmd_httpCommands_res_s *res = NULL;
  struct apicmd_httpCommands_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE, strLen = 0, sendLen;
  uint16_t resLen = 0;
  uint8_t *str;

  DBGIF_LOG_DEBUG("Into altcom_http_send_cmd()");

  if (http_cmd_data == NULL) {
    DBGIF_LOG_ERROR("No http_cmd_data structures\n");
    return ret;
  }

  DBGIF_LOG1_DEBUG("cmd: %d\n", http_cmd_data->cmd);
  DBGIF_LOG1_DEBUG("data_len: %d\n", http_cmd_data->data_len);
  DBGIF_LOG1_DEBUG("pending_data: %lx\n", http_cmd_data->pending_data);
  DBGIF_LOG1_DEBUG("profile_id: %d\n", http_cmd_data->profile_id);

  if (http_cmd_data->data_len > HTTP_MAX_DATA_LENGTH) {
    DBGIF_LOG_ERROR("Data length exceeds maximum\n");
    return ret;
  }

  sendLen = 0;

  /* Headers length */
  if (http_cmd_data->headers != NULL) {
    sendLen += strlen(http_cmd_data->headers) + 1;
  }

  /* Payload length */
  sendLen += http_cmd_data->data_len;

  if (sendLen > HTTP_COMMANDS_MAXLEN) {
    DBGIF_LOG1_ERROR("[http commands request] sendLen too long: %ld\n", sendLen);
    return ret;
  }

  sendLen += HTTP_COMMANDS_REQ_DATALEN - HTTP_COMMANDS_MAXLEN;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDCOMMANDS, sendLen,
                                          (FAR void **)&res, HTTP_COMMANDS_RES_DATALEN)) {
    return ret;
  }

  /* Validate parameters */

  cmd->profileId = profile_id;

  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", cmd->profileId);
    goto sign_out;
  }

  str = cmd->strData;
  cmd->headers_len = 0;

  /* Check headers */
  if (http_cmd_data->headers) {
    strLen = strlen(http_cmd_data->headers);
    if (MAX_HEADERS_SIZE - 1 > strLen) {
      if (strLen > 0) {
        cmd->headers_len = htons(strLen);
        strLen = snprintf((char *)str, strLen + 1, http_cmd_data->headers);
        str += strLen + 1;
      }
    } else {
      DBGIF_LOG1_ERROR("Invalid headers too long: %d\n", strLen);
      goto sign_out;
    }
  }

  /* Check payload */
  if (http_cmd_data->data_to_send && http_cmd_data->data_len) {
    if (MAX_CHUNK_SIZE >= http_cmd_data->data_len) {
      memcpy(str, http_cmd_data->data_to_send, http_cmd_data->data_len);
    } else {
      goto sign_out;
    }
  }

  /* Command (cmd) */
  if (http_cmd_data->cmd >= HTTP_CMD_GET && http_cmd_data->cmd <= HTTP_CMD_POST) {
    cmd->cmd = http_cmd_data->cmd;
  } else {
    DBGIF_LOG1_ERROR("Invalid command (cmd) : %d\n", http_cmd_data->cmd);
    goto sign_out;
  }

  /* Chunk length */
  cmd->data_len = htons(http_cmd_data->data_len);

  /* Pending data  */
  cmd->pending_data = htonl(http_cmd_data->pending_data);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_COMMANDS_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_COMMANDS_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_httpCommands-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
