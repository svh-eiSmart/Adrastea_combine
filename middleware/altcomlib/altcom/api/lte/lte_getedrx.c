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

#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "dbg_if.h"
#include "altcom_osal.h"
#include "apiutil.h"
#include "apicmdgw.h"
#include "apicmd_getedrx.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETEDRX_DATA_LEN (sizeof(struct apicmd_cmddat_getedrx_s))
#define GETEDRX_RESP_LEN (sizeof(struct apicmd_cmddat_getedrxres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_get_edrx() get the requested/network provided eDRX settings.
 *
 * @param [inout] settings: eDRX settings, and it's valid only if LTE_RESULT_OK returned;
 * See @ref lte_edrx_setting_t
 * @param [in] cfgType: Get setting from Requested/Network-Provided configuration
 * See @ref ltecfgtype_e
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_edrx(lte_edrx_setting_t *settings, ltecfgtype_e cfgType) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_getedrx_s *cmd;
  FAR struct apicmd_cmddat_getedrxres_s *res = NULL;

  /* Return error if parameter is invalid */
  if (!settings) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  if (LTE_CFGTYPE_MAX <= cfgType) {
    DBGIF_LOG1_ERROR("Invalid configuration type %lu.\n", (uint32_t)cfgType);
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_EDRX, GETEDRX_DATA_LEN,
                                         (FAR void **)&res, GETEDRX_RESP_LEN)) {
    cmd->cfgType = (uint8_t)cfgType;
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETEDRX_RESP_LEN, &resLen,
                        ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return LTE_RESULT_ERROR;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  if (GETEDRX_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (LTE_RESULT_OK == ret) {
    if (LTE_EDRX_ACTTYPE_UNSUPPORTED == res->acttype || LTE_EDRX_ACTTYPE_WBS1 == res->acttype ||
        LTE_EDRX_ACTTYPE_NBS1 == res->acttype) {
      settings->act_type = (lteedrxtype_e)res->acttype;
    } else {
      settings->act_type = LTE_EDRX_ACTTYPE_UNSUPPORTED;
      DBGIF_LOG1_ERROR("Invalid parameter. acttype:%lu\n", (uint32_t)res->acttype);
    }

    if (LTE_DISABLE == res->enable) {
      settings->enable = LTE_DISABLE;
    } else if (LTE_ENABLE == res->enable) {
      settings->enable = LTE_ENABLE;
    } else {
      settings->enable = LTE_DISABLE;
      DBGIF_LOG1_ERROR("Invalid parameter. enable:%lu\n", (uint32_t)res->enable);
    }

    settings->edrx_cycle = (lteedrxcyc_e)res->edrx_cycle;
    if (LTE_EDRX_CYC_262144 < res->edrx_cycle) {
      DBGIF_LOG1_ERROR("Invalid parameter. edrx_cycle:%lu\n", (uint32_t)res->edrx_cycle);
    }

    settings->ptw_val = (lteedrxptw_e)res->ptw_val;
    if (LTE_EDRX_PTW_2048 < res->ptw_val) {
      DBGIF_LOG1_ERROR("Invalid parameter. ptw_val:%lu\n", (uint32_t)res->ptw_val);
    }
  } else {
    DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}