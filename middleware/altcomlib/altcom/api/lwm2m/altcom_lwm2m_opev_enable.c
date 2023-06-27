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
#include "lwm2m/apicmd_lwm2m_opev_enable.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MOPEVENABLE_REQ_DATALEN (sizeof(struct apicmd_lwm2mopevenable_s))
#define LWM2MOPEVENABE_RES_DATALEN (sizeof(struct apicmd_lwm2mopevenableres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lwm2mEnableOpevents
 *
 * Description:
 *   Send APICMDID_LWM2M_OPEV_ENABLE.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mEnableOpevents(client_inst_t client, uint32_t opevTypeBitMask) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mopevenable_s *cmd = NULL;
  FAR struct apicmd_lwm2mopevenableres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_ENABLE_OPEV_EVENTS,
                                          LWM2MOPEVENABLE_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MOPEVENABE_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->mode = htonl((uint32_t)APICMD_OPEV_ENABLE_MODE_ENABLE);
  cmd->evBitmask = htonl((uint32_t)opevTypeBitMask);
  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MOPEVENABE_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MOPEVENABE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_OPEV_ENABLE_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mEnableOpevents-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mEnableOpev() ask MAP lwm2m client to ENABLE specific sending OPEVs to
 * host/MCU.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] opevet_bitmask: bitmask of LWM2M opev event to be enable.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mEnableOpev(client_inst_t client, unsigned int opevet_bitmask) {
  LWM2MError_e ret;
  uint32_t opevet_bitmask_check;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  if ((opevet_bitmask & ~APICMDID_ENABLE_ALL_OPEV) != 0) {
    DBGIF_LOG1_ERROR("Invalid bitmask 0x%X\r\n", opevet_bitmask);
    return LWM2M_FAILURE;
  }

  for (opevet_bitmask_check = APICMDID_ENABLE_REGISTER_OPEV;
       opevet_bitmask_check <= APICMDID_ENABLE_LAST_OPEV; opevet_bitmask_check <<= 1) {
    if (opevet_bitmask & opevet_bitmask_check) {
      ret = lwm2mEnableOpevents(client, opevet_bitmask_check);
      if (ret != LWM2M_SUCCESS) {
        DBGIF_LOG2_ERROR("altcom_lwm2mEnableOpev fail bitmask %d, ret = %d\n", opevet_bitmask, ret);
        return LWM2M_FAILURE;
      }
    }
  }

  return LWM2M_SUCCESS;
}
