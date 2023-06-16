/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "Infra/app_dbg.h"
#include "ReportManager/ReportManager.h"
#include "ReportManager/ReportManagerAPI.h"

int reportMngrBindReadIndApi(int objectId, reportManagerReadIndCB_t reportManagerReadCB) {
  return reportMngrBindReadInd(objectId, reportManagerReadCB);
}

int reportMngrBindWriteIndApi(int objectId, reportManagerWriteIndCB_t reportManagerWriteCB) {
  return reportMngrBindWriteInd(objectId, reportManagerWriteCB);
}

int reportMngrBindExeIndApi(int objectId, reportManagerExeIndCB_t reportManagerExeCB) {
  return reportMngrBindExeInd(objectId, reportManagerExeCB);
}

ReportMngrConfig_result reportManagerConfigApi(report_type_bitmask_t retportTypeBitMask,
                                               int force) {
  return reportManagerConfig(retportTypeBitMask, force);
}

Report_result reportManagerInitApi(reportManagerStartCB_t reportManagerResultCB, int32_t timeout,
                                   report_type_bitmask_t retportTypeBitMask,
                                   bool isRegisterRequired) {
  int ret;

  ret = reportManagerInit(reportManagerResultCB, timeout, retportTypeBitMask, isRegisterRequired);
  if (ret == 0) {
    return REPORT_MNGR_RET_CODE_OK;
  } else {
    return REPORT_MNGR_RET_CODE_FAIL;
  }
}

Report_result reportManagerInitRefreshApi(void) {
  int ret;

  ret = reportManagerInitRefresh();
  if (ret == 0) {
    return REPORT_MNGR_RET_CODE_OK;
  } else {
    return REPORT_MNGR_RET_CODE_FAIL;
  }
}

Report_result reportManagerSendReportsApi(
    repDesc_t *reportToBeSend /*repDesc_t (*reportToBeSend)[]*/) {
  int ret;
  ret = reportManagerSendReports(reportToBeSend);
  if (ret == 0) {
    return REPORT_MNGR_RET_CODE_OK;
  } else {
    return REPORT_MNGR_RET_CODE_FAIL;
  }
}
