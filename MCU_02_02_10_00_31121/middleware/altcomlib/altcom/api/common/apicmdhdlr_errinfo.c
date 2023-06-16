/*  --------------------------------------------------------------------------

     (c) copyright 2023 Sony Semiconductor Israel Ltd.
     (formerly known as Altair Semiconductor, Ltd.). All rights reserved.

     This software, in source or object form (the "Software"), is the
     property of Sony Semiconductor Israel Ltd. (the "Company") and/or its
     licensors, which have all right, title and interest therein, You
     may use the Software only in accordance with the terms of written
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

     ----------------------------------------------------------------------- */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "apiutil.h"
#include "evthdl_if.h"
#include "apicmdhdlrbs.h"

#include "apicmd_errinfo.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: errinfo_job
 *
 * Description:
 *   This function is a handler that error information of API command.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void errinfo_job(FAR void *arg) {
  FAR struct apicmd_cmddat_errinfo_s *errinfo;

  errinfo = (FAR struct apicmd_cmddat_errinfo_s *)arg;

  if (errinfo->indicator & APICMD_ERRINFO_INDICATOR_ERRCODE) {
    DBGIF_LOG1_ERROR("err_code: %ld\n", (int32_t)ntohl(errinfo->err_code));
  }

  if (errinfo->indicator & APICMD_ERRINFO_INDICATOR_ERRNO) {
    DBGIF_LOG1_ERROR("err_no: %ld\n", (int32_t)ntohl(errinfo->err_no));
  }

  if (errinfo->indicator & APICMD_ERRINFO_INDICATOR_ERRSTR) {
    DBGIF_LOG1_ERROR("err_str: \"%s\"\n", errinfo->err_str);
  }

  altcom_free_cmd((FAR uint8_t *)errinfo);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_errinfo
 *
 * Description:
 *   This function is an API command handler for error information.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_ERRINFO,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_errinfo(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_ERRINFO, errinfo_job);
}
