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
#include "apicmd_httpConfigIp.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_CONFIG_IP_RES_DATALEN (sizeof(struct apicmd_httpConfigIp_res_s))
#define HTTP_CONFIG_IP_REQ_DATALEN (sizeof(struct apicmd_httpConfigIp_s))
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
Http_err_code_e altcom_http_ip_config(Http_profile_id_e profile_id,
                                      Http_config_ip_t *http_ip_config) {
  struct apicmd_httpConfigIp_res_s *res = NULL;
  struct apicmd_httpConfigIp_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE;
  uint16_t resLen = 0;

  DBGIF_LOG_DEBUG("altcom_http_ip_config()");

  if (http_ip_config == NULL) {
    DBGIF_LOG_ERROR("No http_ip_config \n");
    return ret;
  }

  DBGIF_LOG2_DEBUG("sessionId(%d) ip_type(%d)", http_ip_config->sessionId, http_ip_config->ip_type);
  DBGIF_LOG2_DEBUG("Source_port_nb(%d) Destination_port_nb(%d)", http_ip_config->Source_port_nb,
                   http_ip_config->Destination_port_nb);

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDCONFIGIP,
                                          HTTP_CONFIG_IP_REQ_DATALEN, (FAR void **)&res,
                                          HTTP_CONFIG_IP_RES_DATALEN)) {
    return ret;
  }

  /* Validate parameters */

  cmd->profileId = profile_id;

  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", cmd->profileId);
    goto sign_out;
  }

  /* Session no */
  cmd->sessionId = htonl(http_ip_config->sessionId);

  /* IP type */
  if (http_ip_config->ip_type > HTTP_IPTYPE_V6) {
    DBGIF_LOG1_ERROR("Invalid IP type : %d\n", cmd->ip_type);
    goto sign_out;
  }

  cmd->ip_type = http_ip_config->ip_type;

  /* Source port */
  cmd->src_port = 0;
  if (http_ip_config->Source_port_nb > 0) cmd->src_port = htons(http_ip_config->Source_port_nb);

  /* Destination port */
  cmd->dest_port = 0;
  if (http_ip_config->Destination_port_nb > 0)
    cmd->dest_port = htons(http_ip_config->Destination_port_nb);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_CONFIG_IP_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_CONFIG_IP_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_httpConfigIP-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
