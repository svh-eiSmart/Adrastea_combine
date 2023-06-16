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

#include <stdlib.h>
#include <string.h>
#include "http/altcom_http.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
Http_cmd_event_cb g_httpCmdEvCb[HTTP_PROFILE_MAX];
void *g_httpCmdEvCbPriv[HTTP_PROFILE_MAX];
Http_cmd_event_cb g_httpPrspEvCb[HTTP_PROFILE_MAX];
void *g_httpPrspEvCbPriv[HTTP_PROFILE_MAX];
Http_cmd_event_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];
void *g_httpSestermEvCbPriv[HTTP_PROFILE_MAX];

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void http_callback_init(void) {
  memset(g_httpCmdEvCb, 0, sizeof(g_httpCmdEvCb));
  memset(g_httpCmdEvCbPriv, 0, sizeof(g_httpCmdEvCbPriv));
  memset(g_httpPrspEvCb, 0, sizeof(g_httpPrspEvCb));
  memset(g_httpPrspEvCbPriv, 0, sizeof(g_httpPrspEvCbPriv));
  memset(g_httpSestermEvCb, 0, sizeof(g_httpSestermEvCb));
  memset(g_httpSestermEvCbPriv, 0, sizeof(g_httpSestermEvCbPriv));
}
