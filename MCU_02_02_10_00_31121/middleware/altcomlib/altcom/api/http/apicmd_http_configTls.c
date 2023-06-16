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
#include "apicmd_httpConfigTls.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_CONFIG_TLS_RES_DATALEN (sizeof(struct apicmd_httpConfigTls_res_s))
#define HTTP_CONFIG_TLS_REQ_DATALEN (sizeof(struct apicmd_httpConfigTls_s))
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
Http_err_code_e altcom_http_tls_config(Http_profile_id_e profile_id,
                                       Http_config_tls_t *http_tls_config) {
  struct apicmd_httpConfigTls_res_s *res = NULL;
  struct apicmd_httpConfigTls_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE, strLen = 0, sendLen = 0;
  uint16_t resLen = 0;
  char *str;

  DBGIF_LOG_DEBUG("altcom_http_tls_config()");

  if (http_tls_config == NULL) {
    DBGIF_LOG_ERROR("No Http_config_tls structure\n");
    return ret;
  }

  DBGIF_LOG1_DEBUG("Profile (%d)", http_tls_config->profile_tls);
  DBGIF_LOG3_DEBUG("authentication_type (%d) CipherListFilteringType (%d) session_resumption (%d)",
                   http_tls_config->authentication_type, http_tls_config->CipherListFilteringType,
                   http_tls_config->session_resumption);

  sendLen += HTTP_CONFIG_TLS_REQ_DATALEN - HTTP_CONFIG_CIPHER_MAXLEN + 1;
  if (http_tls_config->CipherList != NULL) {
    sendLen += strlen(http_tls_config->CipherList);
  }

  if (sendLen > HTTP_CONFIG_CIPHER_MAXLEN) {
    DBGIF_LOG1_ERROR("[http configTls request] strData length too long: %ld\n", sendLen);
    return ret;
  }

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDCONFIGTLS, sendLen,
                                          (FAR void **)&res, HTTP_CONFIG_TLS_RES_DATALEN)) {
    return ret;
  }

  /* Validate parameters */

  cmd->profileId = profile_id;

  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", cmd->profileId);
    goto sign_out;
  }

  if (http_tls_config->authentication_type > HTTP_NONE_AUTH) {
    DBGIF_LOG1_ERROR("Invalid authentication type : %d\n", cmd->authentication_type);
    goto sign_out;
  }

  if (http_tls_config->CipherListFilteringType > HTTP_CIPHER_NONE_LIST) {
    DBGIF_LOG1_ERROR("Invalid cipher filtering type type : %d\n", cmd->CipherListFilteringType);
    goto sign_out;
  }

  if (http_tls_config->profile_tls == 0) {
    DBGIF_LOG1_ERROR("Invalid cipher filtering type type : %d\n", cmd->profile_tls);
    goto sign_out;
  }

  if (http_tls_config->session_resumption > HTTP_TLS_RESUMP_SESSION_ENABLE) {
    DBGIF_LOG1_ERROR("Invalid cipher session resumption type : %d\n", cmd->session_resumption);
    goto sign_out;
  }

  cmd->authentication_type = http_tls_config->authentication_type;
  cmd->CipherListFilteringType = http_tls_config->CipherListFilteringType;
  cmd->session_resumption = http_tls_config->session_resumption;
  cmd->profile_tls = http_tls_config->profile_tls;

  str = cmd->cipherList;

  if (http_tls_config->CipherList) {
    strLen = strlen(http_tls_config->CipherList) + 1;
    if (HTTP_CONFIG_CIPHER_MAXLEN > strLen) {
      snprintf(str, strLen, "%s", http_tls_config->CipherList);
      DBGIF_LOG2_DEBUG("cipher list: %s, len: %d\n", str, strLen);
    } else {
      DBGIF_LOG1_ERROR("Invalid cipher list too long: %d\n", strLen);
      goto sign_out;
    }
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_CONFIG_TLS_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_CONFIG_TLS_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_httpConfigTls-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
