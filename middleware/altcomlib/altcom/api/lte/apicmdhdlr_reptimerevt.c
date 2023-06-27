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
#include "apicmd_reptimerevt.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern timerevt_report_cb_t g_timerevt_report_callback;
extern void *g_timerevt_report_cbpriv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: repnetstat_job
 *
 * Description:
 *   This function is an API callback for event report receive.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void reptimerevt_job(FAR void *arg) {
  FAR struct apicmd_cmddat_reptimerevt_s *data;
  lte_timerevt_evsel_t status;
  timerevt_report_cb_t callback;
  void *cbpriv;

  data = (FAR struct apicmd_cmddat_reptimerevt_s *)arg;

  ALTCOM_GET_CALLBACK(g_timerevt_report_callback, callback, g_timerevt_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_WARNING("When callback is null called report netstat.\n");
  } else {
    status = (lte_timerevt_evsel_t)(1 << data->status);
    callback((lte_timerevt_evttype_t)data->ev_type, status, cbpriv);
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
 * Name: apicmdhdlr_reptimerevt
 *
 * Description:
 *   This function is an API command handler for timer event report.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_SET_TIMER_EVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_reptimerevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_REPORT_TIMER_EVT, reptimerevt_job);
}
