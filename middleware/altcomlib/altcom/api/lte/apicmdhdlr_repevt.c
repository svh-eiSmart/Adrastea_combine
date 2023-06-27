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
#include "apicmd_repevt.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern simstate_report_cb_t g_simstat_report_callback;
extern void *g_simstat_report_cbpriv;
extern localtime_report_cb_t g_localtime_report_callback;
extern void *g_localtime_report_cbpriv;
extern regstate_report_cb_t g_regstate_report_callback;
extern void *g_regstate_report_cbpriv;
extern psmstate_report_cb_t g_psmstate_report_callback;
extern void *g_psmstate_report_cbpriv;
extern psm_settings_report_cb_t g_dynpsm_report_callback;
extern void *g_dynpsm_report_cbpriv;
extern edrx_settings_report_cb_t g_dynedrx_report_callback;
extern void *g_dynedrx_report_cbpriv;
extern conphase_report_cb_t g_connphase_report_callback;
extern void *g_connphase_report_cbpriv;
extern antitamper_report_cb_t g_antitamper_report_callback;
extern void *g_antitamper_report_cbpriv;
extern scanresult_report_cb_t g_scanresult_report_callback;
extern void *g_scanresult_report_cbpriv;
extern lwm2m_fw_upgrade_evt_report_cb_t g_lwm2m_fw_upgrade_evt_report_callback;
extern void *g_lwm2m_fw_upgrade_evt_report_cbpriv;
extern fw_upgrade_evt_report_cb_t g_fw_upgrade_evt_report_callback;
extern void *g_fw_upgrade_evt_report_cbpriv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: repevt_ltime_report
 *
 * Description:
 *   This function is an API callback for event report LTIME.
 *
 * Input Parameters:
 *  ltime    Pointer to received LTIME data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_ltime_report(FAR struct apicmd_cmddat_ltime_s *ltime) {
  lte_localtime_t result;
  localtime_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_localtime_report_callback, callback, g_localtime_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_localtime_report_callback is not registered.\n");
    return;
  }

  result.year = ltime->year;
  result.mon = ltime->month;
  result.mday = ltime->day;
  result.hour = ltime->hour;
  result.min = ltime->minutes;
  result.sec = ltime->seconds;
  result.tz_sec = ntohl(ltime->timezone);

  callback(&result, cbpriv);
}

/****************************************************************************
 * Name: repevt_simd_report
 *
 * Description:
 *   This function is an API callback for event report SIMD.
 *
 * Input Parameters:
 *  simd    Pointer to received SIMD data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_simd_report(FAR struct apicmd_cmddat_repevt_simd_s *simd) {
  ltesimstate_e result;
  simstate_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_simstat_report_callback, callback, g_simstat_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_simstat_report_callback is not registered.\n");
    return;
  }

  switch (simd->status) {
    case APICMD_REPORT_EVT_SIMD_REMOVAL: {
      result = LTE_SIMSTAT_REMOVAL;
    } break;
    case APICMD_REPORT_EVT_SIMD_INSERTION: {
      result = LTE_SIMSTAT_INSERTION;
    } break;
    default: {
      DBGIF_LOG1_ERROR("Unsupport SIMD status. state:%lu\n", (uint32_t)simd->status);
      return;
    }
  }

  callback(result, cbpriv);
}

/****************************************************************************
 * Name: repevt_simstate_report
 *
 * Description:
 *   This function is an API callback for event report SIMSTATE.
 *
 * Input Parameters:
 *  simstate    Pointer to received SIMSTATE data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_simstate_report(FAR struct apicmd_cmddat_repevt_simstate_s *simstate) {
  ltesimstate_e state;
  simstate_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_simstat_report_callback, callback, g_simstat_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_simstat_report_callback is not registered.\n");
    return;
  }

  switch (simstate->state) {
    case APICMD_REPORT_EVT_SIMSTATE_SIM_DEACTIVATED: {
      state = LTE_SIMSTAT_DEACTIVATED;
    } break;
    case APICMD_REPORT_EVT_SIMSTATE_SIM_INIT_WAIT_PIN_UNLOCK: {
      state = LTE_SIMSTAT_WAIT_PIN_UNLOCK;
    } break;
    case APICMD_REPORT_EVT_SIMSTATE_PERSONALIZATION_FAILED: {
      state = LTE_SIMSTAT_PERSONAL_FAILED;
    } break;
    case APICMD_REPORT_EVT_SIMSTATE_ACTIVATION_COMPLETED: {
      state = LTE_SIMSTAT_ACTIVATE;
    } break;
    default: {
      DBGIF_LOG1_ERROR("Unsupport SIM state. state:%lu\n", (uint32_t)simstate->state);
      return;
    }
  }

  callback(state, cbpriv);
}

/****************************************************************************
 * Name: repevt_regstate_report
 *
 * Description:
 *   This function is an API callback for event report registration state.
 *
 * Input Parameters:
 *  regstate    Pointer to received registration state data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_regstate_report(FAR struct apicmd_cmddat_repregstate_s *regstate) {
  lteregstate_e state;
  regstate_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_regstate_report_callback, callback, g_regstate_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_regstate_report_callback is not registered.\n");
    return;
  }

  switch (regstate->state) {
    case APICMD_REP_REGST_NOTATCH_NOTSRCH: {
      state = LTE_REGSTAT_NOT_REGISTERED_NOT_SEARCHING;
    } break;
    case APICMD_REP_REGST_REGHOME: {
      state = LTE_REGSTAT_REGISTERED_HOME;
    } break;
    case APICMD_REP_REGST_NOTATCH_SRCH: {
      state = LTE_REGSTAT_NOT_REGISTERED_SEARCHING;
    } break;
    case APICMD_REP_REGST_REGDENIED: {
      state = LTE_REGSTAT_REGISTRATION_DENIED;
    } break;
    case APICMD_REP_REGST_UNKNOWN: {
      state = LTE_REGSTAT_UNKNOWN;
    } break;
    case APICMD_REP_REGST_REGROAMING: {
      state = LTE_REGSTAT_REGISTERED_ROAMING;
    } break;
    default: {
      DBGIF_LOG1_ERROR("Unsupport registration state. state:%lu\n", (uint32_t)regstate->state);
      return;
    }
  }

  callback(state, cbpriv);
}
/****************************************************************************
 * Name: repevt_antitamper_report
 *
 * Description:
 *   This function is an API callback for event report ANTITAMPER.
 *
 * Input Parameters:
 *  none
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_antitamper_report(FAR struct apicmd_cmddat_repantitamper_s *antitamper) {
  antitamper_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_antitamper_report_callback, callback, g_antitamper_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_antitamper_report_callback is not registered.\n");
    return;
  }

  callback(antitamper->data, cbpriv);
}
/****************************************************************************
 * Name: repevt_psmstate_report
 *
 * Description:
 *   This function is an API callback for event report psm state.
 *
 * Input Parameters:
 *  psmstate    Pointer to received psm state data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_psmstate_report(FAR struct apicmd_cmddat_reppsmstate_s *psmstate) {
  ltepsmstate_e state;
  psmstate_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_psmstate_report_callback, callback, g_psmstate_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_psmstate_report_callback is not registered.\n");
    return;
  }

  switch (psmstate->state) {
    case APICMD_REP_PSMST_NOT_ACTIVE: {
      state = LTE_PSMSTAT_NOT_ACTIVE;
    } break;
    case APICMD_REP_PSMST_ACTIVE: {
      state = LTE_PSMSTAT_ACTIVE;
    } break;
    case APICMD_REP_PSMST_ACTIVE_AND_CAMPED: {
      state = LTE_PSMSTAT_ACTIVE_AND_CAMPED;
    } break;
    case APICMD_REP_PSMST_CAMP_INTERRUPTED: {
      state = LTE_PSMSTAT_CAMP_INTERRUPTED;
    } break;
    default: {
      DBGIF_LOG1_ERROR("Unsupport psm state. state:%lu\n", (uint32_t)psmstate->state);
      return;
    }
  }

  callback(state, cbpriv);
}

/****************************************************************************
 * Name: repevt_dynpsm_report
 *
 * Description:
 *   This function is an API callback for event report dynamic psm.
 *
 * Input Parameters:
 *  dynPsmSetting    Pointer to received dynamic psm data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_dynpsm_report(FAR struct apicmd_cmddat_repdynpsm_s *dynPsmSetting) {
  lte_psm_setting_t setting;
  psm_settings_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_dynpsm_report_callback, callback, g_dynpsm_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_dynpsm_report_callback is not registered.\n");
    return;
  }

  setting.enable = LTE_ENABLE;
  setting.active_time.unit = dynPsmSetting->at_val.unit;
  setting.active_time.time_val = dynPsmSetting->at_val.time_val;
  setting.ext_periodic_tau_time.unit = dynPsmSetting->tau_val.unit;
  setting.ext_periodic_tau_time.time_val = dynPsmSetting->tau_val.time_val;
  callback(&setting, cbpriv);
}

/****************************************************************************
 * Name: repevt_dynedrx_report
 *
 * Description:
 *   This function is an API callback for event report dynamic eDRX.
 *
 * Input Parameters:
 *  dynEdrxSetting    Pointer to received dynamic eDRX data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_dynedrx_report(FAR struct apicmd_cmddat_repdynedrx_s *dynEdrxSetting) {
  lte_edrx_setting_t setting;
  edrx_settings_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_dynedrx_report_callback, callback, g_dynedrx_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_dynedrx_report_callback is not registered.\n");
    return;
  }

  setting.enable = LTE_ENABLE;
  setting.act_type = (lteedrxtype_e)dynEdrxSetting->acttype;
  setting.edrx_cycle = (lteedrxcyc_e)dynEdrxSetting->edrx_cycle;
  setting.ptw_val = (lteedrxptw_e)dynEdrxSetting->ptw_val;
  callback(&setting, cbpriv);
}

/****************************************************************************
 * Name: repevt_connphase_report
 *
 * Description:
 *   This function is an API callback for event report connectivity phase.
 *
 * Input Parameters:
 *  connphase    Pointer to received connectivity phase data.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_connphase_report(FAR struct apicmd_cmddat_repconnphase_s *connphase) {
  lteconphase_e state;
  lteconphase_rat_e rat;
  lteconfphase_scan_type_e scantype;
  lteconfphase_scan_reason_e scanreason;
  conphase_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_connphase_report_callback, callback, g_connphase_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_connphase_report_callback is not registered.\n");
    return;
  }

  switch (connphase->state) {
    case APICMD_REP_CONNPHASE_START_SCAN: {
      state = LTE_CONPHASE_START_SCAN;
    } break;
    case APICMD_REP_CONNPHASE_FAIL_SCAN: {
      state = LTE_CONPHASE_FAIL_SCAN;
    } break;
    case APICMD_REP_CONNPHASE_ENTER_CAMPED: {
      state = LTE_CONPHASE_ENTER_CAMPED;
    } break;
    case APICMD_REP_CONNPHASE_CONNECTION_ESTABLISHMENT: {
      state = LTE_CONPHASE_CONNECTION_ESTABLISHMENT;
    } break;
    case APICMD_REP_CONNPHASE_START_RESCAN: {
      state = LTE_CONPHASE_START_RESCAN;
    } break;
    case APICMD_REP_CONNPHASE_ENTER_CONNECTED: {
      state = LTE_CONPHASE_CONNECTED;
    } break;
    case APICMD_REP_CONNPHASE_NO_SUITABLE_CELL: {
      state = LTE_CONPHASE_NO_SUITABLE_CELL;
    } break;
    case APICMD_REP_CONNPHASE_REG_ATTEMPT_FAILED: {
      state = LTE_CONPHASE_REG_ATTEMPT_FAILED;
    } break;
    default: {
      DBGIF_LOG1_DEBUG("Unknown connectivity phase. state: %lu\n", (uint32_t)connphase->state);
      state = LTE_CONPHASE_NOT_AVAIL;
    } break;
  }

  switch (connphase->rat) {
    case APICMD_REP_CONPHASE_RAT_CATM: {
      rat = LTE_CONPHASE_RAT_CATM;
    } break;
    case APICMD_REP_CONPHASE_RAT_NBIOT: {
      rat = LTE_CONPHASE_RAT_NBIOT;
    } break;
    case APICMD_REP_CONPHASE_RAT_GSM: {
      rat = LTE_CONPHASE_RAT_GSM;
    } break;
    default: {
      DBGIF_LOG1_DEBUG("Unknown RAT type. rat: %lu\n", (uint32_t)connphase->rat);
      rat = LTE_CONPHASE_RAT_NOT_AVAIL;
    } break;
  }

  switch (connphase->scantype) {
    case APICMD_REP_CONPHASE_SCAN_MRU_ONLY: {
      scantype = LTE_CONPHASE_SCAN_MRU_ONLY;
    } break;
    case APICMD_REP_CONPHASE_SCAN_MRU_AND_FULL_SCAN: {
      scantype = LTE_CONPHASE_SCAN_MRU_AND_FULL_SCAN;
    } break;
    case APICMD_REP_CONPHASE_SCAN_MRU_AND_COUNTRY_SCAN: {
      scantype = LTE_CONPHASE_SCAN_MRU_AND_COUNTRY_SCAN;
    } break;
    case APICMD_REP_CONPHASE_SCAN_MRU_AND_LS: {
      scantype = LTE_CONPHASE_SCAN_MRU_AND_LS;
    } break;
    default: {
      DBGIF_LOG1_DEBUG("Unknown scan type. scantype: %lu\n", (uint32_t)connphase->scantype);
      scantype = LTE_CONPHASE_SCAN_NOT_AVAIL;
    } break;
  }

  switch (connphase->scanreason) {
    case APICMD_REP_CONPHASE_SCAN_REASON_INIT_SCAN: {
      scanreason = LTE_CONPHASE_SCAN_REASON_INIT_SCAN;
    } break;
    case APICMD_REP_CONPHASE_SCAN_REASON_OUT_OF_COVERAGE: {
      scanreason = LTE_CONPHASE_SCAN_REASON_OUT_OF_COVERAGE;
    } break;
    case APICMD_REP_CONPHASE_SCAN_REASON_HIGH_PRIORITY: {
      scanreason = LTE_CONPHASE_SCAN_REASON_HIGH_PRIORITY;
    } break;
    case APICMD_REP_CONPHASE_SCAN_REASON_LIMITED_SERVICE: {
      scanreason = LTE_CONPHASE_SCAN_REASON_LIMITED_SERVICE;
    } break;
    case APICMD_REP_CONPHASE_SCAN_REASON_COPS: {
      scanreason = LTE_CONPHASE_SCAN_REASON_COPS;
    } break;
    case APICMD_REP_CONPHASE_SCAN_REASON_OTHER: {
      scanreason = LTE_CONPHASE_SCAN_REASON_OTHER;
    } break;
    default: {
      DBGIF_LOG1_DEBUG("Unknown scan reason. scanreason: %lu\n", (uint32_t)connphase->scanreason);
      scanreason = LTE_CONPHASE_SCAN_REASON_NOT_AVAIL;
    } break;
  }

  callback(state, rat, scantype, scanreason, cbpriv);
}

/****************************************************************************
 * Name: repevt_scanresult_report
 *
 * Description:
 *   This function is an API callback for event report scanning result.
 *
 * Input Parameters:
 *  scanresult    Pointer to received scanning result.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_scanresult_report(FAR struct apicmd_cmddat_repscanresult_s *scanresult) {
  lte_scan_result_t result;
  scanresult_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_scanresult_report_callback, callback, g_scanresult_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_scanresult_report_callback is not registered.\n");
    return;
  }

  result.resultcode = (lte_scan_resultcode_t)scanresult->resultcode;
  result.info = (lte_scan_info_t)scanresult->info;
  callback(&result, cbpriv);
}

/****************************************************************************
 * Name: repevt_lwm2m_fw_upgrade_evt_report
 *
 * Description:
 *   This function is an API callback for event report lwm2m fw upgrade event.
 *
 * Input Parameters:
 *  lwm2m_fw_upgrade_evt    Pointer to received lwm2m fw upgrade event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_lwm2m_fw_upgrade_evt_report(
    FAR struct apicmd_cmddat_replwm2mfwupgradeevt_s *lwm2m_fw_upgrade_evt) {
  lte_lwm2m_fw_upgrade_evt_t result;
  lwm2m_fw_upgrade_evt_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_lwm2m_fw_upgrade_evt_report_callback, callback,
                      g_lwm2m_fw_upgrade_evt_report_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_lwm2m_fw_upgrade_evt_report_callback is not registered.\n");
    return;
  }

  result.eventtype = (lte_lwm2m_fw_upgrade_evt_type_t)lwm2m_fw_upgrade_evt->eventtype;
  result.package_size = ntohl(lwm2m_fw_upgrade_evt->package_size);
  result.error_type = (lte_lwm2m_fw_upgrade_evt_error_type_t)lwm2m_fw_upgrade_evt->error_type;
  strncpy((char *)result.package_name, (const char *)lwm2m_fw_upgrade_evt->package_name,
          LTE_LWM2M_FW_UPGRADE_PACKAGE_NAME_LEN);
  result.package_name[LTE_LWM2M_FW_UPGRADE_PACKAGE_NAME_LEN - 1] = 0;
  callback(&result, cbpriv);
}

/****************************************************************************
 * Name: repevt_fw_upgrade_evt_report
 *
 * Description:
 *   This function is an API callback for event report fw upgrade event.
 *
 * Input Parameters:
 *  fw_upgrade_evt    Pointer to received fw upgrade event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void repevt_fw_upgrade_evt_report(
    FAR struct apicmd_cmddat_repfwupgradeevt_s *fw_upgrade_evt) {
  lte_fw_upgrade_evt_t result;
  fw_upgrade_evt_report_cb_t callback;
  void *cbpriv;

  ALTCOM_GET_CALLBACK(g_fw_upgrade_evt_report_callback, callback, g_fw_upgrade_evt_report_cbpriv,
                      cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_fw_upgrade_evt_report_callback is not registered.\n");
    return;
  }

  result.eventtype = (lte_fw_upgrade_evt_type_t)fw_upgrade_evt->eventtype;
  result.imagetype = (lte_fw_upgrade_image_type_t)fw_upgrade_evt->imagetype;
  result.failurecause = (lte_fw_upgrade_failure_cause_t)fw_upgrade_evt->failurecause;
  callback(&result, cbpriv);
}

/****************************************************************************
 * Name: repevt_job
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

static void repevt_job(FAR void *arg) {
  FAR struct apicmd_cmddat_repevt_s *data;

  data = (FAR struct apicmd_cmddat_repevt_s *)arg;

  switch (ntohs(data->type)) {
    case APICMD_REPORT_EVT_TYPE_LTIME: {
      repevt_ltime_report(&data->u.ltime);
    } break;
    case APICMD_REPORT_EVT_TYPE_SIMD: {
      repevt_simd_report(&data->u.simd);
    } break;
    case APICMD_REPORT_EVT_TYPE_SIMSTATE: {
      repevt_simstate_report(&data->u.simstate);
    } break;
    case APICMD_REPORT_EVT_TYPE_REGSTATE: {
      repevt_regstate_report(&data->u.regstate);
    } break;
    case APICMD_REPORT_EVT_TYPE_PSMSTATE: {
      repevt_psmstate_report(&data->u.psmstate);
    } break;
    case APICMD_REPORT_EVT_TYPE_DYNPSM: {
      repevt_dynpsm_report(&data->u.dynpsm);
    } break;
    case APICMD_REPORT_EVT_TYPE_DYNEDRX: {
      repevt_dynedrx_report(&data->u.dynedrx);
    } break;
    case APICMD_REPORT_EVT_TYPE_CONNPHASE: {
      repevt_connphase_report(&data->u.connphase);
    } break;
    case APICMD_REPORT_EVT_TYPE_ANTITAMPER: {
      repevt_antitamper_report(&data->u.antitamper);
    } break;
    case APICMD_REPORT_EVT_TYPE_SCANRESULT: {
      repevt_scanresult_report(&data->u.scanresult);
    } break;
    case APICMD_REPORT_EVT_TYPE_LWM2M_FW_UPGRADE_EVT: {
      repevt_lwm2m_fw_upgrade_evt_report(&data->u.lwm2mfwupgevt);
    } break;
    case APICMD_REPORT_EVT_TYPE_FW_UPGRADE_EVT: {
      repevt_fw_upgrade_evt_report(&data->u.fwupgevt);
    } break;
    default: {
      DBGIF_LOG1_ERROR("Unsupported event type. type:%lu\n", (uint32_t)ntohs(data->type));
    } break;
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
 * Name: apicmdhdlr_repevt
 *
 * Description:
 *   This function is an API command handler for event report.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_REPORT_EVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_repevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_REPORT_EVT, repevt_job);
}
