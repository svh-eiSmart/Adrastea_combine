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
extern Http_read_event_cb g_httpGetDataEvCb[HTTP_PROFILE_MAX];
extern Http_sesterm_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];

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
static void cmd_urc_get_data_handling_job(void *arg) {
  Http_read_event_cb callback;
  struct apicmd_httpGetDataUrc_s *incoming = NULL;
  Http_cb_read_data_t *urc_data = NULL;
  void *cbpriv;

  DBGIF_LOG_DEBUG("cmd_urc_get_data_handling_job");
  incoming = (struct apicmd_httpGetDataUrc_s *)arg;

  DBGIF_LOG2_DEBUG("ProfileId (%d) chunklen (%d)", incoming->profileId, ntohs(incoming->chunkLen));
  DBGIF_LOG1_DEBUG("pending (%d)", ntohl(incoming->pending));

  /* Allocate structure for returned URC */
  urc_data = (Http_cb_read_data_t *)BUFFPOOL_ALLOC(sizeof(Http_cb_read_data_t));
  if (urc_data == NULL) {
    DBGIF_LOG_DEBUG("No room for urc_data\n");
    altcom_free_cmd((FAR uint8_t *)arg);
    return;
  }
  memset(urc_data, 0, sizeof(Http_cb_read_data_t));

  /* Copy the values */
  urc_data->profile_id = incoming->profileId;
  urc_data->chunkLen = ntohs(incoming->chunkLen);
  urc_data->pending = ntohl(incoming->pending);

  /* Allocate payload */
  urc_data->data = (uint8_t *)BUFFPOOL_ALLOC(urc_data->chunkLen);
  if (urc_data->data == NULL) {
    DBGIF_LOG_DEBUG("No room for urc_data->data\n");
    altcom_free_cmd((FAR uint8_t *)arg);
    return;
  }
  /* copy payload */
  memcpy(urc_data->data, incoming->data, urc_data->chunkLen);

  /* Setup callback */
  ALTCOM_GET_CALLBACK(g_httpGetDataEvCb[urc_data->profile_id], callback, urc_data, cbpriv);
  if (!callback) {
    DBGIF_LOG_DEBUG("g_httpGetDataEvCb is not registered.\n");
    goto release;
  }

  callback(cbpriv);

release:
  BUFFPOOL_FREE(urc_data->data);
  BUFFPOOL_FREE(urc_data);
  altcom_free_cmd((FAR uint8_t *)arg);
}

static void cmd_urc_handling_job(void *arg) {
  Http_cmd_event_cb callback_cmd;
  Http_sesterm_cb callback_sesterm;
  struct apicmd_httpCmdUrc_s *incoming = NULL;
  HttpUrcIe_t *urc_data = NULL;
  void *cbpriv;

  incoming = (struct apicmd_httpCmdUrc_s *)arg;

  DBGIF_LOG_DEBUG("In evt(cmd_urc_handling_job)\n");
  DBGIF_LOG2_DEBUG("profileId (%d) err_code (%d)", incoming->profileId, incoming->err_code);
  DBGIF_LOG3_DEBUG("filesize (%d) httpEveNum (%d) http_status (%d)", incoming->filesize,
                   incoming->httpEveNum, incoming->http_status);

  /* Allocate structure for returned URC */
  urc_data = (HttpUrcIe_t *)BUFFPOOL_ALLOC(sizeof(HttpUrcIe_t));
  if (!urc_data) {
    DBGIF_LOG_DEBUG("No room for urc_data\n");
    altcom_free_cmd((FAR uint8_t *)arg);
    return;
  }
  memset(urc_data, 0, sizeof(HttpUrcIe_t));

  /* Copy the values */
  urc_data->profileId = incoming->profileId;
  urc_data->http_status = ntohl(incoming->http_status);
  urc_data->filesize = ntohl(incoming->filesize);
  urc_data->httpEveNum = incoming->httpEveNum;
  urc_data->err_code = incoming->err_code;

  if (urc_data->httpEveNum == HTTP_CMD_SESTERM_EV) {
    /* Setup callback */
    ALTCOM_GET_CALLBACK(g_httpSestermEvCb[urc_data->profileId], callback_sesterm, urc_data, cbpriv);
    if (!callback_sesterm) {
      DBGIF_LOG_DEBUG("g_httpSestermEvCb is not registered.\n");
      goto release;
    }
    callback_sesterm(cbpriv);
  } else {
    ALTCOM_GET_CALLBACK(g_httpCmdEvCb[urc_data->profileId], callback_cmd, urc_data, cbpriv);
    if (!callback_cmd) {
      DBGIF_LOG_DEBUG("g_httpCmdEvCb is not registered.\n");
      goto release;
    }
    callback_cmd(cbpriv);
  }

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
enum evthdlrc_e apicmdhdlr_httpGetDataUrcEvt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_HTTP_GETDATAURCS, cmd_urc_get_data_handling_job);
}
