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

#include "lte/lte_api.h"
#include "buffpoolwrapper.h"
#include "apicmd_atchnet.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern attach_net_cb_t g_attach_net_callback;
extern void *g_attach_net_cbpriv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: attachnet_job
 *
 * Description:
 *   This function is an API callback for attach network.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void attachnet_job(FAR void *arg) {
  int32_t ret;
  FAR struct apicmd_cmddat_atchnetres_s *data;
  attach_net_cb_t callback;
  void *cbpriv;
  lteresult_e result;
  lteerrcause_e errcause = LTE_ERR_NOERR;

  data = (FAR struct apicmd_cmddat_atchnetres_s *)arg;

  ALTCOM_GET_AND_CLR_CALLBACK(ret, g_attach_net_callback, callback, g_attach_net_cbpriv, cbpriv);

  if ((ret == 0) && (callback)) {
    if (APICMD_ATCHNET_RES_OK == data->result) {
      result = LTE_RESULT_OK;
    } else if (APICMD_ATCHNET_RES_ERR == data->result) {
      result = LTE_RESULT_ERROR;
      switch (data->errorcause) {
        case APICMD_ATCHNET_RES_ERRCAUSE_WAITENTERPIN: {
          errcause = LTE_ERR_WAITENTERPIN;
        } break;
        case APICMD_ATCHNET_RES_ERRCAUSE_REJECT: {
          errcause = LTE_ERR_REJECT;
        } break;
        case APICMD_ATCHNET_RES_ERRCAUSE_MAXRETRY: {
          errcause = LTE_ERR_MAXRETRY;
        } break;
        case APICMD_ATCHNET_RES_ERRCAUSE_BARRING: {
          errcause = LTE_ERR_BARRING;
        } break;
        default: {
          errcause = LTE_ERR_UNEXPECTED;
        } break;
      }
    } else {
      result = LTE_RESULT_CANCEL;
    }

    callback(result, errcause, cbpriv);
  } else {
    DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
  }

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_attachnet
 *
 * Description:
 *   This function is an API command handler for attach network result.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_ATTACH_NET_RES,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_attachnet(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_CONVERT_RES(APICMDID_ATTACH_NET), attachnet_job);
}
