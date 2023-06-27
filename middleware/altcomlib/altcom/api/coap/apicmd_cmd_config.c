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
#include <stdbool.h>
#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_coap.h"
#include "apicmd_cmd_config.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COAP_CONFIG_RES_DATALEN (sizeof(struct apicmd_coapConfig_res_s))
#define COAP_CONFIG_REQ_DATALEN (sizeof(struct apicmd_coapConfig_s))
#define NULLCHAR 1
#define ISPROFILE_VALID(p) (p > 0 && p <= 5)
#define CMD_TIMEOUT 10000 /* 10 secs */
#define COAPEV_RET_SUCCESS 0
#define MAX_TRANSMIT_SPAN 45

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 @brief altcom_coap_get_config()	Retrieve opened configuration of a given profile.

  @param [in] profileId:            Assigned profile between 1 and 5.
  @params [out] context:			Retrieved profile configuration.

 @return:                           COAP_SUCCESS or COAP_FAILURE.
 */

Coap_err_code altcom_coap_set_config(uint8_t profileId, CoapCfgContext_t *context) {
  int32_t ret = 0;
  uint16_t resLen = 0;
  FAR struct apicmd_coapConfig_s *cmd = NULL;
  FAR struct apicmd_coapConfig_res_s *res = NULL;
  int32_t sendLen = 0;
  uint16_t strLen;
  char *str;
  Coap_err_code err = COAP_FAILURE;

  DBGIF_LOG_DEBUG("altcom_coap_set_config()");

  if (context == NULL) {
    DBGIF_LOG_ERROR("No context structures\n");
    goto end;
  }

  if (context->coapCfgDtls.CipherList) sendLen += strlen(context->coapCfgDtls.CipherList) + 1;
  if (context->coapCfgIp.dest_addr) sendLen += strlen(context->coapCfgIp.dest_addr) + 1;

  if (sendLen > COAP_CONFIG_REQSTR_MAXLEN) {
    DBGIF_LOG1_ERROR("[coap config request] strData length too long: %ld\n", sendLen);
    goto end;
  }

  sendLen += COAP_CONFIG_REQ_DATALEN - COAP_CONFIG_REQSTR_MAXLEN;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_COAP_CMDCONFIG, sendLen,
                                          (FAR void **)&res, COAP_CONFIG_RES_DATALEN)) {
    goto end;
  }

  /* Mandatory data */

  /* Profile id  */
  cmd->profileId = profileId;
  /* DTLS profile */
  cmd->dtls_profile = context->coapCfgDtls.dtls_profile;
  /* Authentication mode */
  cmd->auth_mode = context->coapCfgDtls.auth_mode;
  /* DTLS mode */
  cmd->dtls_mode = context->coapCfgDtls.dtls_mode;
  /* Resumption */
  cmd->session_resumption = context->coapCfgDtls.session_resumption;
  /* IP type */
  cmd->ip_type = context->coapCfgIp.ip_type;
  /* URI Mask */
  cmd->uri_mask = context->uriMask;
  /* Protocol */
  cmd->protocol = htonl(context->protocols);

  /* Optional data */

  /* Session no */
  cmd->sessionId =
      context->coapCfgIp.sessionId ? htonl(*(context->coapCfgIp.sessionId)) : htonl(-1);

  str = cmd->strData;

  /* CipherList Filtering Type  (WHITE/BLACK) list */
  cmd->CipherListLen = 0;
  cmd->CipherListFilteringType = context->coapCfgDtls.CipherListFilteringType
                                     ? *(context->coapCfgDtls.CipherListFilteringType)
                                     : -1;

  if (cmd->CipherListFilteringType != -1 && context->coapCfgDtls.CipherList != NULL) {
    if (MAX_CIPHER_LIST - 1 > strlen(context->coapCfgDtls.CipherList)) {
      strLen = snprintf(str, MAX_CIPHER_LIST, "%s", context->coapCfgDtls.CipherList);
      DBGIF_LOG2_DEBUG("CipherList: %s, len: %d\n", str, strLen);
      cmd->CipherListLen = htons(strLen);
      str += strLen + 1;
    }
  }

  if (!context->coapCfgIp.dest_addr) {
    DBGIF_LOG_ERROR("Dest_addr is mandatory\n");
    goto sign_out;
  }

  /* destination address - Mandatory */
  cmd->urlLen = 0;
  strLen = strlen(context->coapCfgIp.dest_addr) + 1;
  if (MAX_URL_SIZE - 1 > strLen) {
    strLen = snprintf(str, MAX_URL_SIZE, "%s", context->coapCfgIp.dest_addr);
    cmd->urlLen = htons(strLen);
    DBGIF_LOG2_DEBUG("dest_addr: %s, len: %d\n", str, strLen);
  } else {
    DBGIF_LOG1_ERROR("Invalid dest_addr too long: %d\n", strLen);
    goto sign_out;
  }

  /* Sanity check */

  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", profileId);
    goto sign_out;
  }
  if (cmd->auth_mode > COAP_NONE_AUTH) {
    DBGIF_LOG1_ERROR("Incorrect authentication mode parameter: %d\n", profileId);
    goto sign_out;
  }
  if (cmd->dtls_mode > COAP_UNSEC_MODE) {
    DBGIF_LOG1_ERROR("Incorrect dtls mode parameter: %d\n", profileId);
    goto sign_out;
  }
  if (cmd->ip_type > COAP_IPTYPE_V4V6) {
    DBGIF_LOG1_ERROR("Incorrect ip type parameter: %d\n", profileId);
    goto sign_out;
  }
  if (context->uriMask > 0xf) {
    DBGIF_LOG1_ERROR("Incorrect Uri mask parameter: %d\n", profileId);
    goto sign_out;
  }
  if (context->protocols < 0 || context->protocols > MAX_TRANSMIT_SPAN) {
    DBGIF_LOG1_ERROR("Protocol value should be > 0 and <= 45: %d\n", context->protocols);
    goto sign_out;
  }
  if (cmd->session_resumption > COAP_DTLS_RESUMP_SESSION_ENABLE) {
    DBGIF_LOG1_ERROR("Incorrect resumption parameter: %d\n", profileId);
    goto sign_out;
  }
  if (cmd->CipherListFilteringType > COAP_CIPHER_BLACK_LIST) {
    DBGIF_LOG1_ERROR("Incorrect filter type parameter: %d\n", profileId);
    goto sign_out;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, COAP_CONFIG_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != COAP_CONFIG_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);

  DBGIF_LOG1_DEBUG("[altcom_coapConfig-res]ret: %d\n", ret);

  if (ret == (int32_t)COAPEV_RET_SUCCESS) err = COAP_SUCCESS;

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

end:
  return err;
}
