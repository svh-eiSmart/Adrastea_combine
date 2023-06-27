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

#include <stdlib.h>
#include "lte/lte_api.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

attach_net_cb_t g_attach_net_callback = NULL;
detach_net_cb_t g_detach_net_callback = NULL;
data_on_cb_t g_dataon_callback = NULL;
data_off_cb_t g_dataoff_callback = NULL;
netstate_report_cb_t g_netstat_report_callback = NULL;
simstate_report_cb_t g_simstat_report_callback = NULL;
localtime_report_cb_t g_localtime_report_callback = NULL;
quality_report_cb_t g_quality_callback = NULL;
cellinfo_report_cb_t g_cellinfo_callback = NULL;
regstate_report_cb_t g_regstate_report_callback = NULL;
psmstate_report_cb_t g_psmstate_report_callback = NULL;
psm_settings_report_cb_t g_dynpsm_report_callback = NULL;
edrx_settings_report_cb_t g_dynedrx_report_callback = NULL;
conphase_report_cb_t g_connphase_report_callback = NULL;
antitamper_report_cb_t g_antitamper_report_callback = NULL;
scanresult_report_cb_t g_scanresult_report_callback = NULL;
timerevt_report_cb_t g_timerevt_report_callback = NULL;
lwm2m_fw_upgrade_evt_report_cb_t g_lwm2m_fw_upgrade_evt_report_callback = NULL;
fw_upgrade_evt_report_cb_t g_fw_upgrade_evt_report_callback = NULL;

void *g_attach_net_cbpriv = NULL;
void *g_detach_net_cbpriv = NULL;
void *g_dataon_cbpriv = NULL;
void *g_dataoff_cbpriv = NULL;
void *g_netstat_report_cbpriv = NULL;
void *g_simstat_report_cbpriv = NULL;
void *g_localtime_report_cbpriv = NULL;
void *g_quality_cbpriv = NULL;
void *g_cellinfo_cbpriv = NULL;
void *g_regstate_report_cbpriv = NULL;
void *g_psmstate_report_cbpriv = NULL;
void *g_dynpsm_report_cbpriv = NULL;
void *g_dynedrx_report_cbpriv = NULL;
void *g_connphase_report_cbpriv = NULL;
void *g_antitamper_report_cbpriv = NULL;
void *g_scanresult_report_cbpriv = NULL;
void *g_timerevt_report_cbpriv = NULL;
void *g_lwm2m_fw_upgrade_evt_report_cbpriv = NULL;
void *g_fw_upgrade_evt_report_cbpriv = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_callback_init
 *
 * Description:
 *   Delete the registered callback and initialize it.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void lte_callback_init(void) {
  g_attach_net_callback = NULL;
  g_detach_net_callback = NULL;
  g_dataon_callback = NULL;
  g_dataoff_callback = NULL;
  g_netstat_report_callback = NULL;
  g_simstat_report_callback = NULL;
  g_localtime_report_callback = NULL;
  g_quality_callback = NULL;
  g_cellinfo_callback = NULL;
  g_regstate_report_callback = NULL;
  g_psmstate_report_callback = NULL;
  g_dynpsm_report_callback = NULL;
  g_dynedrx_report_callback = NULL;
  g_connphase_report_callback = NULL;
  g_antitamper_report_callback = NULL;
  g_scanresult_report_callback = NULL;
  g_timerevt_report_callback = NULL;

  g_attach_net_cbpriv = NULL;
  g_detach_net_cbpriv = NULL;
  g_dataon_cbpriv = NULL;
  g_dataoff_cbpriv = NULL;
  g_netstat_report_cbpriv = NULL;
  g_simstat_report_cbpriv = NULL;
  g_localtime_report_cbpriv = NULL;
  g_quality_cbpriv = NULL;
  g_cellinfo_cbpriv = NULL;
  g_regstate_report_cbpriv = NULL;
  g_psmstate_report_cbpriv = NULL;
  g_dynpsm_report_cbpriv = NULL;
  g_dynedrx_report_cbpriv = NULL;
  g_connphase_report_cbpriv = NULL;
  g_antitamper_report_cbpriv = NULL;
  g_scanresult_report_cbpriv = NULL;
  g_timerevt_report_cbpriv = NULL;
}
