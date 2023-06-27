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
#include "apicmd_setreptimerevt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SETREP_TIMEREVT_DATA_LEN (sizeof(struct apicmd_cmddat_setreptimerevt_s))
#define SETREP_TIMEREVT_RES_DATA_LEN (sizeof(struct apicmd_cmddat_setreptimerevtres_s))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_lte_setreptimerevt_isproc = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern timerevt_report_cb_t g_timerevt_report_callback;
extern void *g_timerevt_report_cbpriv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_set_report_timerevt () Change the report setting of LTE timer event.
 * The default report setting is disable.
 *
 * @param [in] timerevt_callback: Callback function to notify that
 * LTE timer event.
 * @param [in] tmrevt: parameters of timer event
 * @param[in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_timerevt(timerevt_report_cb_t timerevt_callback, lte_settimerevt_t *tmrevt,
                                void *userPriv) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_setreptimerevt_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_setreptimerevtres_s *resbuff = NULL;
  uint16_t resbufflen = SETREP_TIMEREVT_RES_DATA_LEN;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EPERM;
  }

  /* Check if callback registering only */

  if (altcom_isCbRegOnly()) {
    ALTCOM_CLR_CALLBACK(g_timerevt_report_callback, g_timerevt_report_cbpriv);
    if (timerevt_callback) {
      ALTCOM_REG_CALLBACK(ret, g_timerevt_report_callback, timerevt_callback,
                          g_timerevt_report_cbpriv, userPriv);
    }

    return ret;
  }

  /* Check this process runnning. */

  if (g_lte_setreptimerevt_isproc) {
    return -EBUSY;
  }

  g_lte_setreptimerevt_isproc = true;

  /* Accept the API */
  /* Allocate API command buffer to send */

  cmdbuff = (FAR struct apicmd_cmddat_setreptimerevt_s *)apicmdgw_cmd_allocbuff(
      APICMDID_SET_TIMER_EVT, SETREP_TIMEREVT_DATA_LEN);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    g_lte_setreptimerevt_isproc = false;
    return -ENOMEM;
  } else {
    resbuff = (FAR struct apicmd_cmddat_setreptimerevtres_s *)BUFFPOOL_ALLOC(resbufflen);
    if (!resbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      altcom_free_cmd((FAR uint8_t *)cmdbuff);
      g_lte_setreptimerevt_isproc = false;
      return -ENOMEM;
    }

    /* Set command parameter. */
    cmdbuff->ev_type = tmrevt->ev_type;
    cmdbuff->mode = tmrevt->mode;
    cmdbuff->ev_sel = tmrevt->ev_sel;
    cmdbuff->preceding_time = htonl(tmrevt->preceding_time);

    /* Send API command to modem */

    ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, resbufflen, &reslen,
                        ALTCOM_SYS_TIMEO_FEVR);
  }

  if (0 <= ret) {
    /* Register API callback */

    if (APICMD_SETREP_TIMEREVT_RES_OK == resbuff->result) {
      ALTCOM_CLR_CALLBACK(g_timerevt_report_callback, g_timerevt_report_cbpriv);
      if (timerevt_callback) {
        ALTCOM_REG_CALLBACK(ret, g_timerevt_report_callback, timerevt_callback,
                            g_timerevt_report_cbpriv, userPriv);
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
  g_lte_setreptimerevt_isproc = false;

  return ret;
}
