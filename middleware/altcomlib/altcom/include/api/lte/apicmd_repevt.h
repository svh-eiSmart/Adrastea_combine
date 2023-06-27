/****************************************************************************
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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPEVT_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPEVT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"
#include "apicmd_ltime.h"
#include "apicmd_getpsm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_SET_REP_EVT_LTIME (1 << 0)
#define APICMD_SET_REP_EVT_SIMD (1 << 1)
#define APICMD_SET_REP_EVT_SIMSTATE (1 << 2)
#define APICMD_SET_REP_EVT_REGSTATE (1 << 3)
#define APICMD_SET_REP_EVT_PSMSTATE (1 << 4)
#define APICMD_SET_REP_EVT_DYNPSM (1 << 5)
#define APICMD_SET_REP_EVT_DYNEDRX (1 << 6)
#define APICMD_SET_REP_EVT_CONNPHASE (1 << 7)
#define APICMD_SET_REP_EVT_ANTITAMPER (1 << 8)
#define APICMD_SET_REP_EVT_SCANRESULT (1 << 9)
#define APICMD_SET_REP_EVT_LWM2M_FW_UPGRADE_EVT (1 << 10)
#define APICMD_SET_REP_EVT_FW_UPGRADE_EVT (1 << 11)
#define APICMD_SET_REP_EVT_MAX (1 << 15)

#define APICMD_SET_REP_EVT_RES_OK (0)
#define APICMD_SET_REP_EVT_RES_ERR (1)

#define APICMD_REPORT_EVT_TYPE_LTIME (0)
#define APICMD_REPORT_EVT_TYPE_SIMD (1)
#define APICMD_REPORT_EVT_TYPE_SIMSTATE (2)
#define APICMD_REPORT_EVT_TYPE_REGSTATE (3)
#define APICMD_REPORT_EVT_TYPE_PSMSTATE (4)
#define APICMD_REPORT_EVT_TYPE_DYNPSM (5)
#define APICMD_REPORT_EVT_TYPE_DYNEDRX (6)
#define APICMD_REPORT_EVT_TYPE_CONNPHASE (7)
#define APICMD_REPORT_EVT_TYPE_ANTITAMPER (8)
#define APICMD_REPORT_EVT_TYPE_SCANRESULT (9)
#define APICMD_REPORT_EVT_TYPE_LWM2M_FW_UPGRADE_EVT (10)
#define APICMD_REPORT_EVT_TYPE_FW_UPGRADE_EVT (11)

#define APICMD_REPORT_EVT_SIMD_REMOVAL (0)
#define APICMD_REPORT_EVT_SIMD_INSERTION (1)

#define APICMD_REPORT_EVT_ALERTU_TEMP (0)
#define APICMD_REPORT_EVT_ALERTU_ANTIAMPER (1)

#define APICMD_REPORT_EVT_SIMSTATE_SIM_DEACTIVATED (0)
#define APICMD_REPORT_EVT_SIMSTATE_SIM_INIT_WAIT_PIN_UNLOCK (1)
#define APICMD_REPORT_EVT_SIMSTATE_PERSONALIZATION_FAILED (2)
#define APICMD_REPORT_EVT_SIMSTATE_ACTIVATION_COMPLETED (3)

#define APICMD_REP_REGST_NOTATCH_NOTSRCH (0)
#define APICMD_REP_REGST_REGHOME (1)
#define APICMD_REP_REGST_NOTATCH_SRCH (2)
#define APICMD_REP_REGST_REGDENIED (3)
#define APICMD_REP_REGST_UNKNOWN (4)
#define APICMD_REP_REGST_REGROAMING (5)

#define APICMD_REP_PSMST_NOT_ACTIVE (0)
#define APICMD_REP_PSMST_ACTIVE (1)
#define APICMD_REP_PSMST_ACTIVE_AND_CAMPED (2)
#define APICMD_REP_PSMST_CAMP_INTERRUPTED (3)

#define APICMD_REP_CONNPHASE_START_SCAN (0)
#define APICMD_REP_CONNPHASE_FAIL_SCAN (1)
#define APICMD_REP_CONNPHASE_ENTER_CAMPED (2)
#define APICMD_REP_CONNPHASE_CONNECTION_ESTABLISHMENT (3)
#define APICMD_REP_CONNPHASE_START_RESCAN (4)
#define APICMD_REP_CONNPHASE_ENTER_CONNECTED (5)
#define APICMD_REP_CONNPHASE_NO_SUITABLE_CELL (6)
#define APICMD_REP_CONNPHASE_REG_ATTEMPT_FAILED (7)
#define APICMD_REP_CONNPHASE_NOT_AVAIL (99)

#define APICMD_REP_CONPHASE_RAT_CATM (0)
#define APICMD_REP_CONPHASE_RAT_NBIOT (1)
#define APICMD_REP_CONPHASE_RAT_GSM (3)
#define APICMD_REP_CONPHASE_RAT_NOT_AVAIL (99)

#define APICMD_REP_CONPHASE_SCAN_MRU_ONLY (0)
#define APICMD_REP_CONPHASE_SCAN_MRU_AND_FULL_SCAN (1)
#define APICMD_REP_CONPHASE_SCAN_MRU_AND_COUNTRY_SCAN (2)
#define APICMD_REP_CONPHASE_SCAN_MRU_AND_LS (3)
#define APICMD_REP_CONPHASE_SCAN_MRU_NOT_AVAIL (99)

#define APICMD_REP_CONPHASE_SCAN_REASON_INIT_SCAN (0)
#define APICMD_REP_CONPHASE_SCAN_REASON_OUT_OF_COVERAGE (1)
#define APICMD_REP_CONPHASE_SCAN_REASON_HIGH_PRIORITY (2)
#define APICMD_REP_CONPHASE_SCAN_REASON_LIMITED_SERVICE (3)
#define APICMD_REP_CONPHASE_SCAN_REASON_COPS (4)
#define APICMD_REP_CONPHASE_SCAN_REASON_OTHER (5)
#define APICMD_REP_CONPHASE_SCAN_REASON_NOT_AVAIL (99)

#define APICMD_REP_SCANRESULT_SUCCESS (0)
#define APICMD_REP_SCANRESULT_FAILURE (1)

#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_PACKAGE_NAME_LEN 30

#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_PENDING_DOWNLOAD (0)
#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_PENDING_UPDATE (1)
#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_COMPLETED (2)
#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED (3)
#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_FOTA_CANCELED_BY_LWM2M_SERVER (4)

#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED_NON_FATAL (0)
#define APICMD_REP_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED_FATAL (1)

#define APICMD_REP_FW_UPGRADE_EVT_DLPENDING (0)
#define APICMD_REP_FW_UPGRADE_EVT_DLDONE (1)
#define APICMD_REP_FW_UPGRADE_EVT_UPPENDING (2)
#define APICMD_REP_FW_UPGRADE_EVT_REBOOTNEEDED (3)
#define APICMD_REP_FW_UPGRADE_EVT_FAILURE (4)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */
begin_packed_struct struct apicmd_cmddat_setrepevt_s {
  uint16_t event;
  uint8_t enability;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_setrepevtres_s {
  uint8_t result;
  uint16_t event;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repevt_simd_s { uint8_t status; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repevt_simstate_s { uint8_t state; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repregstate_s { uint8_t state; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_reppsmstate_s { uint8_t state; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repantitamper_s { uint8_t data; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repdynpsm_s {
  struct apicmd_cmddat_getpsm_timeval_s at_val;
  struct apicmd_cmddat_getpsm_timeval_s tau_val;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repdynedrx_s {
  uint8_t acttype;
  uint8_t edrx_cycle;
  uint8_t ptw_val;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repconnphase_s {
  uint8_t state;
  uint8_t rat;
  uint8_t scantype;
  uint8_t scanreason;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repscanresult_s {
  uint8_t resultcode;
  uint8_t info;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_replwm2mfwupgradeevt_s {
  uint8_t eventtype;
  uint32_t package_size;
  uint8_t package_name[APICMD_REP_LWM2M_FW_UPGRADE_EVT_PACKAGE_NAME_LEN];
  uint8_t error_type;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repfwupgradeevt_s {
  uint8_t eventtype;
  uint8_t imagetype;
  uint8_t failurecause;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repevt_s {
  uint16_t type;
  begin_packed_struct union {
    struct apicmd_cmddat_ltime_s ltime;
    struct apicmd_cmddat_repevt_simd_s simd;
    struct apicmd_cmddat_repevt_simstate_s simstate;
    struct apicmd_cmddat_repregstate_s regstate;
    struct apicmd_cmddat_reppsmstate_s psmstate;
    struct apicmd_cmddat_repdynpsm_s dynpsm;
    struct apicmd_cmddat_repdynedrx_s dynedrx;
    struct apicmd_cmddat_repconnphase_s connphase;
    struct apicmd_cmddat_repantitamper_s antitamper;
    struct apicmd_cmddat_repscanresult_s scanresult;
    struct apicmd_cmddat_replwm2mfwupgradeevt_s lwm2mfwupgevt;
    struct apicmd_cmddat_repfwupgradeevt_s fwupgevt;
  } end_packed_struct u;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPEVT_H */
