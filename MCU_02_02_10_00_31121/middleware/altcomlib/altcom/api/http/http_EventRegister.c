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
extern void *g_httpCmdEvCbPriv[HTTP_PROFILE_MAX];
extern Http_cmd_event_cb g_httpPrspEvCb[HTTP_PROFILE_MAX];
extern void *g_httpPrspEvCbPriv[HTTP_PROFILE_MAX];
extern Http_cmd_event_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];
extern void *g_httpSestermEvCbPriv[HTTP_PROFILE_MAX];

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void altcom_http_event_register(Http_profile_id_e profile_id, Http_event_type_e event_type,
                                Http_cmd_event_cb callback, void *user_priv) {
  Http_cmd_event_cb *dst_cb;
  void **dst_priv;
  int32_t ret = 0;

  switch (event_type) {
    case HTTP_CMD_PUTCONF_EV:
    case HTTP_CMD_GETRCV_EV:
    case HTTP_CMD_POSTCONF_EV:
    case HTTP_CMD_DELCONF_EV:
      dst_cb = &g_httpCmdEvCb[profile_id];
      dst_priv = &g_httpCmdEvCbPriv[profile_id];
      break;
    case HTTP_CMD_PRSPRCV_EV:
      dst_cb = &g_httpPrspEvCb[profile_id];
      dst_priv = &g_httpPrspEvCbPriv[profile_id];
      break;
    case HTTP_CMD_SESTERM_EV:
      dst_cb = &g_httpSestermEvCb[profile_id];
      dst_priv = &g_httpSestermEvCbPriv[profile_id];
      break;

    default:
      return;
  }

  if (callback) {
    ALTCOM_REG_CALLBACK(ret, *dst_cb, callback, *dst_priv, user_priv);
    if (ret != 0) {
      DBGIF_LOG1_ERROR("Fail to register HTTP event callback, ret = %ld", ret);
    }
  } else {
    ALTCOM_CLR_CALLBACK(*dst_cb, *dst_priv);
  }
}
