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
#include "apicmd_httpConfigNode.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_CONFIG_NODE_RES_DATALEN (sizeof(struct apicmd_httpConfigNode_res_s))
#define HTTP_CONFIG_NODE_REQ_DATALEN (sizeof(struct apicmd_httpConfigNode_s))
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
Http_err_code_e altcom_http_node_config(Http_profile_id_e profile_id,
                                        Http_config_node_t *http_node_config) {
  struct apicmd_httpConfigNode_res_s *res = NULL;
  struct apicmd_httpConfigNode_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE, strLen = 0, sendLen = 0;
  uint16_t resLen = 0;
  char *str;

  DBGIF_LOG_DEBUG("Into altcom_http_node_config()");

  if (http_node_config == NULL) {
    DBGIF_LOG_ERROR("No Http_config_node_t structures\n");
    return ret;
  }

  /* URL is Mandatory */
  if (http_node_config->dest_addr == NULL) {
    DBGIF_LOG_ERROR("Destination address is mandatory\n");
    return ret;
  }

  sendLen += strlen(http_node_config->dest_addr) + 1;
  if (http_node_config->user != NULL) sendLen += strlen(http_node_config->user) + 1;
  if (http_node_config->passwd != NULL) sendLen += strlen(http_node_config->passwd) + 1;
  if (sendLen > HTTP_CONFIG_NODE_MAXLEN) {
    DBGIF_LOG1_ERROR("[http configNode request] strData length too long: %ld\n", sendLen);
    return ret;
  }

  sendLen += HTTP_CONFIG_NODE_REQ_DATALEN - HTTP_CONFIG_NODE_MAXLEN;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDCONFIGNODE, sendLen,
                                          (FAR void **)&res, HTTP_CONFIG_NODE_RES_DATALEN)) {
    return ret;
  }

  /* Validate parameters */

  DBGIF_LOG1_DEBUG("profileId: %d\n", profile_id);

  if (!ISPROFILE_VALID(profile_id)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", cmd->profileId);
    goto sign_out;
  }
  cmd->profileId = profile_id;

  if (http_node_config->user != NULL) {
    if (http_node_config->encode_format >= HTTP_ENC_MAX) {
      DBGIF_LOG1_ERROR("Incorrect value for username/password encoding: %d\n", cmd->encode_format);
      goto sign_out;
    }
    if (http_node_config->passwd == NULL) {
      DBGIF_LOG_ERROR("Password is required\n");
      goto sign_out;
    }

    cmd->encode_format = (uint8_t)http_node_config->encode_format;
  }

  str = cmd->strData;

  strLen = strlen(http_node_config->dest_addr);
  DBGIF_LOG1_DEBUG("Destination address length (%d)", strLen);
  if (MAX_URL_SIZE - 1 > strLen) {
    cmd->url_len = strLen;
    strLen = snprintf(str, MAX_URL_SIZE, "%s", http_node_config->dest_addr);
    str += strLen + 1;
    DBGIF_LOG1_DEBUG("Destination address length (%d)", strLen);
  } else {
    DBGIF_LOG1_ERROR("Invalid dest_addr too long: %d\n", strLen);
    goto sign_out;
  }

  cmd->user_len = 0;
  cmd->passwd_len = 0;

  if (http_node_config->user != NULL) {
    strLen = strlen(http_node_config->user);
    if (MAX_USER_SIZE - 1 > strLen) {
      strLen = snprintf(str, strLen + 1, "%s", http_node_config->user);
      str += strLen + 1;
      cmd->user_len = strLen;
    } else {
      DBGIF_LOG1_ERROR("Invalid username too long: %d\n", strLen);
      goto sign_out;
    }
  }

  if (http_node_config->passwd != NULL) {
    strLen = strlen(http_node_config->passwd);
    if (MAX_PASSWD_SIZE - 1 > strLen) {
      strLen = snprintf(str, MAX_PASSWD_SIZE, "%s", http_node_config->passwd);
      cmd->passwd_len = strLen;
    } else {
      DBGIF_LOG1_ERROR("Invalid password too long: %d\n", strLen);
      goto sign_out;
    }
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_CONFIG_NODE_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_CONFIG_NODE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_httpConfigNode-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
