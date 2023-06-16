/****************************************************************************
 *
 *  (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.
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
#include <stdbool.h>
#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_http.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern Http_cmd_event_cb g_httpCmdEvCb[HTTP_PROFILE_MAX];
extern Http_read_event_cb g_httpGetDataEvCb[HTTP_PROFILE_MAX];
extern Http_sesterm_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 @brief http_EventRegister():  		Register URC callbacks.

 @param [in]  callback:             Callback
 @param [in]  profile_id            Selected profile
 @param [in]  CmdParams:            Selected event.

 */

void http_EventRegister(void *callback, Http_profile_id_e profile_id,
                        eHttpEveEnum_e event_callback) {
  switch (event_callback) {
    case HTTP_CMD_PUTCONF_EV:
    case HTTP_CMD_GETRCV_EV:
    case HTTP_CMD_POSTCONF_EV:
    case HTTP_CMD_DELCONF_EV:
      g_httpCmdEvCb[profile_id] = NULL;
      if (callback) g_httpCmdEvCb[profile_id] = (Http_cmd_event_cb)callback;
      break;
    case HTTP_CMD_READCONF_EV:
      if (callback) g_httpGetDataEvCb[profile_id] = (Http_read_event_cb)callback;
      break;
    case HTTP_CMD_SESTERM_EV:
      if (callback) g_httpSestermEvCb[profile_id] = (Http_sesterm_cb)callback;
      break;
    default:
      break;
  }
}
