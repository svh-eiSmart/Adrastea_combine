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
#include "buffpoolwrapper.h"
#include "altcom_http.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "apicmd_http_urc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern Http_cmd_event_cb g_httpCmdEvCb[HTTP_PROFILE_MAX];
extern void *g_httpCmdEvCbPriv[HTTP_PROFILE_MAX];
extern Http_cmd_event_cb g_httpPrspEvCb[HTTP_PROFILE_MAX];
extern void *g_httpPrspEvCbPriv[HTTP_PROFILE_MAX];
extern Http_cmd_event_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];
extern void *g_httpSestermEvCbPriv[HTTP_PROFILE_MAX];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_urc_handling_job
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

static void cmd_urc_handling_job(void *arg) {
  Http_cmd_event_cb callback_cmd;
  struct apicmd_httpCmdUrc_s *incoming = NULL;
  Http_event_t *urc_data = NULL;
  void *cbpriv;

  incoming = (struct apicmd_httpCmdUrc_s *)arg;

  DBGIF_LOG_DEBUG("In evt(cmd_urc_handling_job)\n");
  DBGIF_LOG2_DEBUG("profileId (%d) err_code (%d)", incoming->profileId, incoming->err_code);
  DBGIF_LOG3_DEBUG("filesize (%d) event_type (%d) http_status (%d)", incoming->filesize,
                   incoming->event_type, incoming->http_status);

  /* Allocate structure for returned URC */
  urc_data = (Http_event_t *)BUFFPOOL_ALLOC(sizeof(Http_event_t));
  if (!urc_data) {
    DBGIF_LOG_DEBUG("No room for urc_data\n");
    altcom_free_cmd((FAR uint8_t *)arg);
    return;
  }
  memset(urc_data, 0, sizeof(Http_event_t));

  /* Copy the values */
  urc_data->profileId = incoming->profileId;
  urc_data->http_status = ntohl(incoming->http_status);
  urc_data->file_size = ntohl(incoming->filesize);
  urc_data->avail_size = ntohl(incoming->availsize);
  urc_data->event_type = incoming->event_type;
  urc_data->err_code = incoming->err_code;

  if (urc_data->event_type == HTTP_CMD_SESTERM_EV) {
    /* Setup callback */
    ALTCOM_GET_CALLBACK(g_httpSestermEvCb[urc_data->profileId], callback_cmd,
                        g_httpSestermEvCbPriv[urc_data->profileId], cbpriv);
    if (!callback_cmd) {
      DBGIF_LOG_DEBUG("g_httpSestermEvCb is not registered.\n");
      goto release;
    }
  } else if (urc_data->event_type == HTTP_CMD_PRSPRCV_EV) {
    ALTCOM_GET_CALLBACK(g_httpPrspEvCb[urc_data->profileId], callback_cmd,
                        g_httpPrspEvCbPriv[urc_data->profileId], cbpriv);
    if (!callback_cmd) {
      DBGIF_LOG_DEBUG("g_httpCmdEvCb is not registered.\n");
      goto release;
    }
  } else {
    ALTCOM_GET_CALLBACK(g_httpCmdEvCb[urc_data->profileId], callback_cmd,
                        g_httpCmdEvCbPriv[urc_data->profileId], cbpriv);
    if (!callback_cmd) {
      DBGIF_LOG_DEBUG("g_httpCmdEvCb is not registered.\n");
      goto release;
    }
  }

  callback_cmd((void *)urc_data, cbpriv);

release:
  altcom_free_cmd((FAR uint8_t *)arg);
  BUFFPOOL_FREE(urc_data);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_httpUrcEvt
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

enum evthdlrc_e apicmdhdlr_httpUrcEvt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_HTTP_CMDURCS, cmd_urc_handling_job);
}