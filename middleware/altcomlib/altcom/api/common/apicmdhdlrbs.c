/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

#include "dbg_if.h"
#include "wrkrid.h"
#include "apiutil.h"
#include "evthdlbs.h"
#include "apicmdgw.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Inline functions
 ****************************************************************************/

enum evthdlrc_e apicmdhdlrbs_do_runjob(FAR uint8_t *evt, uint16_t cmdid,
                                       FAR apicmdhdlrbs_cb_job job) {
  if (!evt) {
    DBGIF_LOG_ERROR("NULL parameter");
    return EVTHDLRC_INTERNALERROR;
  }

  if (!apicmdgw_cmdid_compare(evt, cmdid)) {
    return EVTHDLRC_UNSUPPORTEDEVENT;
  }

  if (0 >
      evthdlbs_runjob(WRKRID_API_CALLBACK_THREAD, (CODE thrdpool_jobif_t)job, (FAR void *)evt)) {
    altcom_free_cmd((FAR uint8_t *)evt);
    return EVTHDLRC_INTERNALERROR;
  }

  return EVTHDLRC_STARTHANDLE;
}