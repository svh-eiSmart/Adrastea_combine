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

#include "buffpoolwrapper.h"
#include "apicmd_errind.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: errindication_job
 *
 * Description:
 *   This function is a handler that error indication of API command.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void errindication_job(FAR void *arg) {
  FAR struct apicmd_cmddat_errind_s *data;

  data = (FAR struct apicmd_cmddat_errind_s *)arg;

  DBGIF_LOG_ERROR("Receive err indication.\n");
  DBGIF_LOG1_ERROR("version       :%02x\n", data->ver);
  DBGIF_LOG1_ERROR("sequence id   :%02x\n", data->seqid);
  DBGIF_LOG1_ERROR("command id    :%04x\n", (unsigned int)ntohs(data->cmdid));
  DBGIF_LOG1_ERROR("transaction id:%04x\n", (unsigned int)ntohs(data->transid));
  DBGIF_LOG1_ERROR("data length   :%04x\n", (unsigned int)ntohs(data->dtlen));

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((FAR uint8_t *)data);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_errindication
 *
 * Description:
 *   This function is an API command handler for error indication.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_ERRIND,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_errindication(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_ERRIND, errindication_job);
}
