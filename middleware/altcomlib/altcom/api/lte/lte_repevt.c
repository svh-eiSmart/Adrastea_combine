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
#include "apicmd_repevt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REPEVT_DATA_LEN (sizeof(struct apicmd_cmddat_setrepevt_s))
#define REPSETRES_DATA_LEN (sizeof(struct apicmd_cmddat_setrepevtres_s))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint16_t g_set_repevt = 0;
static bool g_lte_setrepsimstat_isproc = false;
static bool g_lte_setrepltime_isproc = false;
static bool g_lte_setregstat_isproc = false;
static bool g_lte_setpsmstat_isproc = false;
static bool g_lte_setdynpsm_isproc = false;
static bool g_lte_setdynedrx_isproc = false;
static bool g_lte_setconnphase_isproc = false;
static bool g_lte_setalert_isproc = false;
static bool g_lte_setscanresult_isproc = false;
static bool g_lte_setlwm2m_fw_upgrade_evt_isproc = false;
static bool g_lte_setfw_upgrade_evt_isproc = false;

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
int32_t lte_set_report_event(uint16_t event, bool *glbProcFlag, void **glbCb, void **glbPriv,
                             void *userCb, void *userPriv) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_setrepevt_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_setrepevtres_s *resbuff = NULL;
  uint16_t resbufflen = REPSETRES_DATA_LEN;
  uint16_t reslen = 0;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EPERM;
  }

  /* Check if callback registering only */

  if (altcom_isCbRegOnly()) {
    ALTCOM_CLR_CALLBACK(*glbCb, *glbPriv);
    if (userCb) {
      ALTCOM_REG_CALLBACK(ret, *glbCb, userCb, *glbPriv, userPriv);
    }

    return ret;
  }

  /* Check this process runnning. */
  if (*glbProcFlag) {
    return -EBUSY;
  }

  *glbProcFlag = true;

  /* Allocate API command buffer to send */
  cmdbuff = (FAR struct apicmd_cmddat_setrepevt_s *)apicmdgw_cmd_allocbuff(APICMDID_SET_REP_EVT,
                                                                           REPEVT_DATA_LEN);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    *glbProcFlag = false;
    return -ENOMEM;
  } else {
    /* Set event field */
    resbuff = (FAR struct apicmd_cmddat_setrepevtres_s *)BUFFPOOL_ALLOC(resbufflen);
    if (!resbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      altcom_free_cmd((FAR uint8_t *)cmdbuff);
      *glbProcFlag = false;
      return -ENOMEM;
    }

    if (userCb) {
      g_set_repevt |= event;
      cmdbuff->enability = 1;
    } else {
      g_set_repevt &= ~event;
      cmdbuff->enability = 0;
    }

    cmdbuff->event = event;
    cmdbuff->event = htons(cmdbuff->event);

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, resbufflen, &reslen,
                        ALTCOM_SYS_TIMEO_FEVR);
  }

  if (0 <= ret && resbufflen == reslen) {
    if (APICMD_SET_REP_EVT_RES_OK == resbuff->result) {
      /* Register API callback */
      ALTCOM_CLR_CALLBACK(*glbCb, *glbPriv);
      if (userCb) {
        ALTCOM_REG_CALLBACK(ret, *glbCb, userCb, *glbPriv, userPriv);
      }
    } else {
      DBGIF_LOG1_ERROR("API command response is err, event is 0x%X.\n",
                       (unsigned int)ntohs(resbuff->event));
      ret = -EIO;
    }
  }

  if (0 <= ret) {
    ret = 0;
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  (void)BUFFPOOL_FREE(resbuff);
  *glbProcFlag = false;

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_set_report_simstate() Change the report setting of the SIM
 * state. The default report setting is disable.
 *
 * @param [in] simstate_callback: Callback function to notify that
 * SIM state. If NULL is set, the report setting is disabled.
 * @param[in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_simstate(simstate_report_cb_t simstate_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_SIMD | APICMD_SET_REP_EVT_SIMSTATE,
                              &g_lte_setrepsimstat_isproc, (void **)&g_simstat_report_callback,
                              (void **)&g_simstat_report_cbpriv, (void *)simstate_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_localtime() Change the report setting of the local
 * time. The default report setting is disable.
 *
 * @param [in] localtime_callback: Callback function to notify that
 * local time. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_localtime(localtime_report_cb_t localtime_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_LTIME, (void *)&g_lte_setrepltime_isproc,
                              (void **)&g_localtime_report_callback,
                              (void **)&g_localtime_report_cbpriv, (void *)localtime_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_regstate() Change the report setting of the LTE
 * registration state
 * The default report setting is disable.
 *
 * @param [in] regstate_callback: Callback function to notify that
 * LTE registration  state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_regstate(regstate_report_cb_t regstate_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_REGSTATE, (void *)&g_lte_setregstat_isproc,
                              (void **)&g_regstate_report_callback,
                              (void **)&g_regstate_report_cbpriv, (void *)regstate_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_active_psm() Change the report setting of the LTE
 * active PSM state.
 * The default report setting is disable.
 *
 * @param [in] psmstate_callback: Callback function to notify that
 * LTE PSM state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_active_psm(psmstate_report_cb_t psmstate_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_PSMSTATE, (void *)&g_lte_setpsmstat_isproc,
                              (void **)&g_psmstate_report_callback,
                              (void **)&g_psmstate_report_cbpriv, (void *)psmstate_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_dynamic_psm() Change the report setting of the LTE
 * network given dynamic psm settings.
 * The default report setting is disable.
 *
 * @param [in] psm_settings_report_callback: Callback function to notify that
 * LTE network given psm setting has changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_dynamic_psm(psm_settings_report_cb_t psm_settings_report_callback,
                                   void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_DYNPSM, (void *)&g_lte_setdynpsm_isproc,
                              (void **)&g_dynpsm_report_callback, (void **)&g_dynpsm_report_cbpriv,
                              (void *)psm_settings_report_callback, (void *)userPriv);
}

/**
 * @brief lte_set_report_dynamic_psm() Change the report setting of the LTE
 * network given dynamic edrx settings.
 * The default report setting is disable.
 *
 * @param [in] edrx_settings_report_callback: Callback function to notify that
 * LTE network given edrx setting has changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_dynamic_edrx(edrx_settings_report_cb_t edrx_settings_report_callback,
                                    void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_DYNEDRX, (void *)&g_lte_setdynedrx_isproc,
                              (void **)&g_dynedrx_report_callback,
                              (void **)&g_dynedrx_report_cbpriv,
                              (void *)edrx_settings_report_callback, (void *)userPriv);
}

/**
 * @brief lte_set_report_connectivity_phase() Change the report setting of the LTE
 * connectivity phase
 * The default report setting is disable.
 *
 * @param [in] conphase_report_cb_t: Callback function to notify that
 * LTE connectivity phase changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_connectivity_phase(conphase_report_cb_t conphase_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_CONNPHASE, (void *)&g_lte_setconnphase_isproc,
                              (void **)&g_connphase_report_callback,
                              (void **)&g_connphase_report_cbpriv, (void *)conphase_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_antitamper() Change the report setting of the LTE
 * anti tamper event
 * The default report setting is disable.
 *
 * @param [in] regstate_callback: Callback function to notify that
 * ANTI TAMPER event. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_antitamper(antitamper_report_cb_t antitamper_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_ANTITAMPER, (void *)&g_lte_setalert_isproc,
                              (void **)&g_antitamper_report_callback,
                              (void **)&g_antitamper_report_cbpriv, (void *)antitamper_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_scanresult() Change the report setting of the LTE
 * scanning result event
 * The default report setting is disable.
 *
 * @param [in] scanresult_callback: Callback of scanning result. If NULL is set, the report setting
 * is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_scanresult(scanresult_report_cb_t scanresult_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_SCANRESULT, (void *)&g_lte_setscanresult_isproc,
                              (void **)&g_scanresult_report_callback,
                              (void **)&g_scanresult_report_cbpriv, (void *)scanresult_callback,
                              (void *)userPriv);
}

/**
 * @brief lte_set_report_lwm2m_fw_upgrade_evt() Change the report setting of the LTE LWM2M
 * FW upgrade event
 * The default report setting is disable.
 *
 * @param [in] lwm2m_fw_upgrade_evt_callback: Callback of lwm2m fw upgrade event. If NULL is set,
 * the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_lwm2m_fw_upgrade_evt(
    lwm2m_fw_upgrade_evt_report_cb_t lwm2m_fw_upgrade_evt_callback, void *userPriv) {
  return lte_set_report_event(APICMD_SET_REP_EVT_LWM2M_FW_UPGRADE_EVT,
                              (void *)&g_lte_setlwm2m_fw_upgrade_evt_isproc,
                              (void **)&g_lwm2m_fw_upgrade_evt_report_callback,
                              (void **)&g_lwm2m_fw_upgrade_evt_report_cbpriv,
                              (void *)lwm2m_fw_upgrade_evt_callback, (void *)userPriv);
}

/**
 * @brief lte_set_report_fw_upgrade_evt() Change the report setting of the LTE FW upgrade event
 * The default report setting is disable.
 *
 * @param [in] fw_upgrade_evt_callback: Callback of fw upgrade event. If NULL is set,
 * the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_fw_upgrade_evt(fw_upgrade_evt_report_cb_t fw_upgrade_evt_callback,
                                      void *userPriv) {
  return lte_set_report_event(
      APICMD_SET_REP_EVT_FW_UPGRADE_EVT, (void *)&g_lte_setfw_upgrade_evt_isproc,
      (void **)&g_fw_upgrade_evt_report_callback, (void **)&g_fw_upgrade_evt_report_cbpriv,
      (void *)fw_upgrade_evt_callback, (void *)userPriv);
}
