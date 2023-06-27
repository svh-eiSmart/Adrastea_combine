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

#include <string.h>
#include "altcom_coap.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "apicmd_cmd_urc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern coapRstEvCb g_coapRstEvCb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rst_urc_handling_job
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

static void rst_urc_handling_job(void *arg) {
  coapRstEvCb callback;
  struct apicmd_coapRstUrc_s *incoming;
  uint8_t profile;
  char token[COAP_TOKEN_MAX_SIZE];

  incoming = (struct apicmd_coapRstUrc_s *)arg;
  profile = incoming->profileId;
  token[0] = 0;
  if (incoming->Token[0] != '\0') snprintf(token, COAP_TOKEN_MAX_SIZE, incoming->Token);

  callback = g_coapRstEvCb;
  if (!callback) {
    DBGIF_LOG_DEBUG("g_coapRstEvCb is not registered.\n");
    return;
  }

  callback(profile, token);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apirsthdlr_cmdevt
 *
 * Description:
 *   This function is an API command handler for COAP URC handling.
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

enum evthdlrc_e apirsthdlr_cmdevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_COAP_RST_URC, rst_urc_handling_job);
}
