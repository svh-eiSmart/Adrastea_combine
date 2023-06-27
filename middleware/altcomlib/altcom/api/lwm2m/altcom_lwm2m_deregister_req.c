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
#include "lwm2m/altcom_lwm2m.h"
#include "osal/altcom_osal.h"
#include "util/apiutil.h"
#include "apicmd.h"
#include "lwm2m/apicmd_lwm2m_deregister_req.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MDEREGISTERREQ_REQ_DATALEN (sizeof(struct apicmd_lwm2mderegisterreq_s))
#define LWM2MDEREGISTERREQ_RES_DATALEN (sizeof(struct apicmd_lwm2mderegisterreqres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lwm2mlwm2mDeregisterReq
 *
 * Description:
 *   Send APICMDID_LWM2M_REGISTER.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mlwm2mDeregisterReq(client_inst_t client, unsigned int server_id) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mderegisterreq_s *cmd = NULL;
  FAR struct apicmd_lwm2mderegisterreqres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_DEREGISTER,
                                          LWM2MDEREGISTERREQ_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MDEREGISTERREQ_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->server_id = htonl((uint32_t)server_id);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MDEREGISTERREQ_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MDEREGISTERREQ_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_LWM2M_DEREGISTER_REQ_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response is failed ret :%ld.\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }
  ret = LWM2M_SUCCESS;
  DBGIF_LOG1_DEBUG("[lwm2mlwm2mDeregisterReq-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mDeregisterReq() ask MAP lwm2m client to send deregister req to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] server_id: server id to register req for.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mDeregisterReq(client_inst_t client, unsigned int server_id) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  ret = lwm2mlwm2mDeregisterReq(client, server_id);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("altcom_lwm2mDeregisterReq fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
