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
#include "apicmd_getpsm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETPSM_DATA_LEN (sizeof(struct apicmd_cmddat_getpsm_s))
#define GETPSM_RESP_LEN (sizeof(struct apicmd_cmddat_getpsmres_s))

#define GETACTPSM_DATA_LEN (0)
#define GETACTPSM_RESP_LEN (sizeof(struct apicmd_cmddat_getactpsmres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_get_psm() get the requested/network provided PSM settings.
 *
 * @param [inout] settings: PSM settings, and it's valid only if LTE_RESULT_OK returned;
 * See @ref lte_psm_setting_t
 * @param [in] cfgType: Get setting from Requested/Network-Provided configuration
 * See @ref ltecfgtype_e
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_psm(lte_psm_setting_t *settings, ltecfgtype_e cfgType) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_getpsm_s *cmd;
  FAR struct apicmd_cmddat_getpsmres_s *res = NULL;

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
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_PSM, GETPSM_DATA_LEN,
                                         (FAR void **)&res, GETPSM_RESP_LEN)) {
    cmd->cfgType = (uint8_t)cfgType;
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETPSM_RESP_LEN, &resLen,
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

  if (GETPSM_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (LTE_RESULT_OK == ret) {
    if (APICMD_GETPSM_DISABLE == res->enable) {
      settings->enable = LTE_DISABLE;
    } else if (APICMD_GETPSM_ENABLE == res->enable) {
      settings->enable = LTE_ENABLE;
    } else {
      DBGIF_LOG1_ERROR("Invalid parameter. enable:%d\n", res->enable);
      settings->enable = LTE_DISABLE;
    }

    settings->active_time.unit = res->at_val.unit;
    if (LTE_PSM_T3324_UNIT_6MIN < res->at_val.unit) {
      DBGIF_LOG1_ERROR("Invalid parameter. RAT unit:%lu\n", (uint32_t)res->at_val.unit);
    }

    settings->active_time.time_val = res->at_val.time_val;
    if (APICMD_GETPSM_TIMER_MAX < res->at_val.time_val) {
      DBGIF_LOG1_ERROR("Invalid parameter. RAT time_val:%lu\n", (uint32_t)res->at_val.time_val);
    }

    settings->ext_periodic_tau_time.unit = res->tau_val.unit;
    if (LTE_PSM_T3412_UNIT_320HOUR < res->tau_val.unit) {
      DBGIF_LOG1_ERROR("Invalid parameter. TAU unit:%lu\n", (uint32_t)res->tau_val.unit);
    }

    settings->ext_periodic_tau_time.time_val = res->tau_val.time_val;
    if (APICMD_GETPSM_TIMER_MIN > res->tau_val.time_val ||
        APICMD_GETPSM_TIMER_MAX < res->tau_val.time_val) {
      DBGIF_LOG1_ERROR("Invalid parameter. TAU time_val:%lu\n", (uint32_t)res->tau_val.time_val);
    }
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}

/**
 * @brief lte_get_active_psm() get if we are in active PSM state
 *
 * @param [inout] actpsm: PSM activity state, and it's valid only if LTE_RESULT_OK returned;
 * See @ref ltepsmactstate_e
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_active_psm(ltepsmactstate_e *actpsm) {
  int32_t ret;
  uint16_t resLen = 0;
  void *cmd;
  FAR struct apicmd_cmddat_getactpsmres_s *res = NULL;

  /* Return error if parameter is invalid */
  if (!actpsm) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_ACTPSM, GETACTPSM_DATA_LEN,
                                         (FAR void **)&res, GETACTPSM_RESP_LEN)) {
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETACTPSM_RESP_LEN, &resLen,
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

  if (GETACTPSM_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (LTE_RESULT_OK == ret) {
    *actpsm = (ltepsmactstate_e)res->status;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
