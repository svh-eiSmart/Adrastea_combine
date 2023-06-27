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

#include "dbg_if.h"
#include "apicmd.h"
#include "util/apiutil.h"
#include "osal/altcom_osal.h"
#include "lwm2m/altcom_lwm2m.h"
#include "lwm2m/apicmd_lwm2m_get_serverinfo.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MSERVERINFO_REQ_DATALEN (sizeof(struct apicmd_lwm2mserverinfo_s))
#define LWM2MSERVERINFO_RESP_DATALEN (sizeof(struct apicmd_lwm2mserverinfores_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief altcom_lwm2mGetServerInfo get the server information from MAP side.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [out] serverinfo: Server information struct ptr
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 */

LWM2MError_e altcom_lwm2mGetServerInfo(client_inst_t client, lwm2m_serverinfo_t *serverinfo) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mserverinfo_s *cmd = NULL;
  FAR struct apicmd_lwm2mserverinfores_s *res = NULL;

  /* Return error if parameter is invalid */
  if (!serverinfo) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LWM2M_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_GET_SERVERINFO,
                                          LWM2MSERVERINFO_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MSERVERINFO_RESP_DATALEN)) {
    return LWM2M_FAILURE;
  }

  cmd->client = (uint8_t)client;
  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MSERVERINFO_RESP_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MSERVERINFO_RESP_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = res->ret_code;
  if (ret == LWM2M_SUCCESS) {
    memset((void *)serverinfo, 0, sizeof(lwm2m_serverinfo_t));

    uint8_t i;
    for (i = 0; i < res->server_cnt; i++) {
      serverinfo->info[i].valid = 1;
      memcpy((void *)serverinfo->info[i].server_uri, (void *)res->info[i].server_uri,
             MAX_LWM2M_SERVER_URI);
      serverinfo->info[i].server_id = ntohl(res->info[i].server_id);
      serverinfo->info[i].liftime = ntohl(res->info[i].liftime);
      serverinfo->info[i].binding = (server_binding_t)res->info[i].binding;
      serverinfo->info[i].server_stat = (server_stat_t)res->info[i].server_stat;
      serverinfo->info[i].last_regdate = ntohll(res->info[i].last_regdate);
    }
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendServerInfo-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}
