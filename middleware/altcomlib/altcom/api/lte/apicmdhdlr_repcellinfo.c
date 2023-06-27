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
#include "apicmd_repcellinfo.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern cellinfo_report_cb_t g_cellinfo_callback;
extern void *g_cellinfo_cbpriv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: check_arrydigitnum
 *
 * Description:
 *   Evaluate the validity of each digit of arrayed numerical value.
 *
 * Input Parameters:
 *  number    Array type number.
 *  digit     @number digit.
 *
 * Returned Value:
 *   Returns true if everything is valid. Otherwise it returns false.
 *
 ****************************************************************************/

bool check_arrydigitnum(FAR uint8_t number[], uint8_t digit) {
  uint8_t cnt;

  for (cnt = 0; cnt < digit; cnt++) {
    if (APICMD_SET_REPCELLINFO_DIGIT_NUM_MAX < number[cnt]) {
      return false;
    }
  }

  return true;
}

/****************************************************************************
 * Name: repcellinfo_job
 *
 * Description:
 *   This function is an API callback for cellinfo report receive.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repcellinfo_job(FAR void *arg) {
  FAR struct apicmd_cmddat_repcellinfo_s *data;
  lte_cellinfo_t *repdat = NULL;
  cellinfo_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_cellinfo_callback, callback, g_cellinfo_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_cellinfo_callback is not registered.\n");
    return;
  }

  data = (FAR struct apicmd_cmddat_repcellinfo_s *)arg;

  repdat = (lte_cellinfo_t *)BUFFPOOL_ALLOC(sizeof(lte_cellinfo_t));
  if (!repdat) {
    DBGIF_LOG_DEBUG("report data buffer alloc error.\n");
  } else {
    repdat->valid = (ltevalidflag_t)data->enability;
    if (repdat->valid) {
      repdat->phycell_id = ntohl(data->cell_id);
      repdat->earfcn = ntohl(data->earfcn);
      memcpy(repdat->mcc, data->mcc, APICMD_SET_REPCELLINFO_MCC_DIGIT);
      repdat->mnc_digit = data->mnc_digit;
      memcpy(repdat->mnc, data->mnc, data->mnc_digit);
      memcpy(repdat->cgid, data->cgid, APICMD_SET_REPCELLINFO_CGID_DIGIT_MAX + 1);
      repdat->tac = ntohs(data->tac);
      if (APICMD_SET_REPCELLINFO_CELLID_MAX < repdat->phycell_id) {
        DBGIF_LOG1_ERROR("repdat->phycell_id error:%lu\n", repdat->phycell_id);
        repdat->valid = LTE_INVALID;
      } else if (APICMD_SET_REPCELLINFO_EARFCN_MAX < repdat->earfcn) {
        DBGIF_LOG1_ERROR("repdat->earfcn error:%lu\n", repdat->earfcn);
        repdat->valid = LTE_INVALID;
      } else if (!check_arrydigitnum(repdat->mcc, APICMD_SET_REPCELLINFO_MCC_DIGIT)) {
        DBGIF_LOG_ERROR("repdat->mcc error\n");
        repdat->valid = LTE_INVALID;
      } else if (repdat->mnc_digit < APICMD_SET_REPCELLINFO_MNC_DIGIT_MIN ||
                 APICMD_SET_REPCELLINFO_MNC_DIGIT_MAX < repdat->mnc_digit) {
        DBGIF_LOG1_ERROR("repdat->mnc_digit error:%d\n", repdat->mnc_digit);
        repdat->valid = LTE_INVALID;
      } else if (!check_arrydigitnum(repdat->mnc, repdat->mnc_digit)) {
        DBGIF_LOG_ERROR("repdat->mnc error\n");
        repdat->valid = LTE_INVALID;
      }

      repdat->time_diffidx = ntohs(data->time_diffidx);
      repdat->ta = ntohs(data->ta);
      repdat->sfn = ntohs(data->sfn);
      repdat->rsrp = ntohs(data->rsrp);
      repdat->rsrq = ntohs(data->rsrq);
      repdat->neighbor_num = data->neighbor_num;

      int i;

      for (i = 0; i < data->neighbor_num; i++) {
        repdat->neighbor_cell[i].valid = (ltevalidflag_t)data->neighbor_cell[i].valid;
        repdat->neighbor_cell[i].cell_id = ntohl(data->neighbor_cell[i].cell_id);
        repdat->neighbor_cell[i].earfcn = ntohl(data->neighbor_cell[i].earfcn);
        repdat->neighbor_cell[i].sfn = ntohs(data->neighbor_cell[i].sfn);
        repdat->neighbor_cell[i].rsrp = ntohs(data->neighbor_cell[i].rsrp);
        repdat->neighbor_cell[i].rsrq = ntohs(data->neighbor_cell[i].rsrq);
      }
    }

    callback(repdat, cbpriv);
    (void)BUFFPOOL_FREE((FAR void *)repdat);
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
 * Name: apicmdhdlr_repcellinfo
 *
 * Description:
 *   This function is an API command handler for cellinfo report.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_REPORT_CELLINFO,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_repcellinfo(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_REPORT_CELLINFO, repcellinfo_job);
}
