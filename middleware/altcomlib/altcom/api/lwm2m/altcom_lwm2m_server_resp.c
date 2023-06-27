/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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
#include "lwm2m/altcom_lwm2m.h"
#include "osal/altcom_osal.h"
#include "util/apiutil.h"
#include "apicmd.h"
#include "lwm2m/apicmd_lwm2m_server_resp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MSERVER_RESP_DATALEN (sizeof(struct apicmd_lwm2mserverresp_s))
#define LWM2MSERVER_RESP_RESP_DATALEN (sizeof(struct apicmd_lwm2mserverrespres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Name: mqttConnect_request
 *
 * Description:
 *   Send APICMDID_LWM2M_HOST_ENABLE.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mSendServerResp(client_inst_t client, int seqNum, char *respCode) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mserverresp_s *cmd = NULL;
  FAR struct apicmd_lwm2mserverrespres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_SERVER_RESP,
                                          LWM2MSERVER_RESP_DATALEN, (FAR void **)&res,
                                          LWM2MSERVER_RESP_RESP_DATALEN)) {
    return LWM2M_FAILURE;
  }
  DBGIF_LOG1_DEBUG("lwm2mSendServResp seq=%d\r\n", seqNum);
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->seqNum = htonl((uint32_t)seqNum);
  strncpy(&cmd->CoAPresponseCode[0], respCode, COAP_RESP_CODE_STR_LEN);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MSERVER_RESP_RESP_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MSERVER_RESP_RESP_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendServerResp-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mServerResp()  send back to MAP lwm2m client resp ACK with the reqNum a server
 * request was recv.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] seqNum: sequence number of request that should.
 * @param [in] respCode: COAP respond code to be sent to the server back.
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mServerResp(client_inst_t client, int seqNum, char *respCode) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  DBGIF_LOG1_DEBUG("call to SERVER RESP via C-API with seqNum=%d\r\n", seqNum);
  ret = lwm2mSendServerResp(client, seqNum, respCode);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendServerResp fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
