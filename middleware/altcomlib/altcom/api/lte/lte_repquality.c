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
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_repquality.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define QUALITY_DATA_LEN (sizeof(struct apicmd_cmddat_setrepquality_s))
#define QUALITY_SETRES_DATA_LEN (sizeof(FAR struct apicmd_cmddat_setrepquality_res_s))
#define QUALITY_PERIOD_MIN (1)
#define QUALITY_PERIOD_MAX (4233600)
#define GET_QUALITY_DATA_LEN (0)
#define GET_QUALITY_RESP_LEN (sizeof(struct apicmd_cmddat_repquality_s))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_lte_setrepquality_isproc = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern quality_report_cb_t g_quality_callback;
extern void *g_quality_cbpriv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_set_report_quality() Change the report setting of the quality
 * information. The default report setting is disable.
 *
 * @param [in] quality_callback: Callback function to notify that
 * quality information. If NULL is set, the report setting is disabled.
 * @param [in] period: Reporting cycle in sec (1-4233600)
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_quality(quality_report_cb_t quality_callback, uint32_t period,
                               void *userPriv) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_setrepquality_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_setrepquality_res_s *resbuff = NULL;
  uint16_t resbufflen = QUALITY_SETRES_DATA_LEN;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EPERM;
  }

  /* Check if callback registering only */

  if (altcom_isCbRegOnly()) {
    ALTCOM_CLR_CALLBACK(g_quality_callback, g_quality_cbpriv);
    if (quality_callback) {
      ALTCOM_REG_CALLBACK(ret, g_quality_callback, quality_callback, g_quality_cbpriv, userPriv);
    }

    return ret;
  }

  if (quality_callback) {
    if (QUALITY_PERIOD_MIN > period || QUALITY_PERIOD_MAX < period) {
      DBGIF_LOG_ERROR("Invalid parameter.\n");
      return -EINVAL;
    }
  }

  if (g_lte_setrepquality_isproc) {
    return -EBUSY;
  }

  g_lte_setrepquality_isproc = true;

  /* Accept the API */
  /* Allocate API command buffer to send */

  cmdbuff = (FAR struct apicmd_cmddat_setrepquality_s *)apicmdgw_cmd_allocbuff(
      APICMDID_SET_REP_QUALITY, QUALITY_DATA_LEN);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    g_lte_setrepquality_isproc = false;
    return -ENOSPC;
  } else {
    resbuff = (FAR struct apicmd_cmddat_setrepquality_res_s *)BUFFPOOL_ALLOC(resbufflen);
    if (!resbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      altcom_free_cmd((FAR uint8_t *)cmdbuff);
      g_lte_setrepquality_isproc = false;
      return -ENOSPC;
    }

    /* Set event field */

    cmdbuff->enability =
        !quality_callback ? APICMD_SET_REPQUALITY_DISABLE : APICMD_SET_REPQUALITY_ENABLE;
    cmdbuff->interval = htonl(period);

    ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, resbufflen, &reslen,
                        ALTCOM_SYS_TIMEO_FEVR);
  }

  if (0 <= ret && resbufflen == reslen) {
    if (APICMD_SET_REPQUALITY_RES_OK == resbuff->result) {
      /* Register API callback */

      ALTCOM_CLR_CALLBACK(g_quality_callback, g_quality_cbpriv);
      if (quality_callback) {
        ALTCOM_REG_CALLBACK(ret, g_quality_callback, quality_callback, g_quality_cbpriv, userPriv);
      }
    } else {
      DBGIF_LOG_ERROR("API command response is err.\n");
      ret = -EIO;
    }
  }

  if (0 <= ret) {
    ret = 0;
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  (void)BUFFPOOL_FREE(resbuff);
  g_lte_setrepquality_isproc = false;

  return ret;
}

/**
 * @brief lte_get_quality() get the current cell quality information.
 *
 * @param [in] quality: Quality information.
 * See @ref lte_quality_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_quality(lte_quality_t *quality) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR void *cmd;
  FAR struct apicmd_cmddat_repquality_s *res;

  /* Return error if callback is NULL */
  if (!quality) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_QUALITY,
                                         GET_QUALITY_DATA_LEN, (FAR void **)&res,
                                         GET_QUALITY_RESP_LEN)) {
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GET_QUALITY_RESP_LEN, &resLen,
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

  if (GET_QUALITY_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  quality->valid = APICMD_REP_QUALITY_ENABLE == res->enability ? LTE_VALID : LTE_INVALID;
  if (quality->valid) {
    quality->rsrp = ntohs(res->rsrp);
    quality->rsrq = ntohs(res->rsrq);
    quality->sinr = ntohs(res->sinr);
    quality->rssi = ntohs(res->rssi);
    if (quality->rsrp < APICMD_REP_QUALITY_RSRP_MIN ||
        APICMD_REP_QUALITY_RSRP_MAX < quality->rsrp) {
      DBGIF_LOG1_ERROR("quality->rsrp error:%d\n", quality->rsrp);
      quality->valid = LTE_INVALID;
    } else if (quality->rsrq < APICMD_REP_QUALITY_RSRQ_MIN ||
               APICMD_REP_QUALITY_RSRQ_MAX < quality->rsrq) {
      DBGIF_LOG1_ERROR("quality->rsrq error:%d\n", quality->rsrq);
      quality->valid = LTE_INVALID;
    } else if (quality->sinr < APICMD_REP_QUALITY_SINR_MIN ||
               APICMD_REP_QUALITY_SINR_MAX < quality->sinr) {
      DBGIF_LOG1_ERROR("quality->sinr error:%d\n", quality->sinr);
      quality->valid = LTE_INVALID;
    } else {
      /* Do nothing. */
    }
  }

  ret = LTE_RESULT_OK;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}