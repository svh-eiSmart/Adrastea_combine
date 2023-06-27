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
#include "lwm2m/apicmd_lwm2mHostEnable.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MHOSTENABLE_REQ_DATALEN (sizeof(struct apicmd_lwm2mhostenable_s))
#define LWM2MHOSTENABLE_RES_DATALEN (sizeof(struct apicmd_lwm2mhostenableres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lwm2mEnableHost
 *
 * Description:
 *   Send APICMDID_LWM2M_HOST_ENABLE.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mEnableHost(client_inst_t client) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mhostenable_s *cmd = NULL;
  FAR struct apicmd_lwm2mhostenableres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_HOST_ENABLE,
                                          LWM2MHOSTENABLE_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MHOSTENABLE_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->mode = htonl((uint32_t)APICMD_HOST_ENABLE_MODE_ENABLE);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MHOSTENABLE_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MHOSTENABLE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_HOST_ENABLE_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mEnableHost-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mEnableHost() ask MAP lwm2m client to send/get host indications to host/MCU.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mEnableHost(client_inst_t client) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  DBGIF_LOG_DEBUG("Call to HOST enable via C-API\r\n");
  ret = lwm2mEnableHost(client);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("altcom_lwm2mEnableHost fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
