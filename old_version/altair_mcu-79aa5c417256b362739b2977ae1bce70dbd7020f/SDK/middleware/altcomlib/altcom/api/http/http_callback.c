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
#include "http/altcom_http.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
Http_cmd_event_cb g_httpCmdEvCb[HTTP_PROFILE_MAX];
Http_read_event_cb g_httpGetDataEvCb[HTTP_PROFILE_MAX];
Http_sesterm_cb g_httpSestermEvCb[HTTP_PROFILE_MAX];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: http_callback_init
 *
 * Description:
 *   Delete the registered callback and initialize it.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void http_callback_init(void) {
  for (int i = 0; i < HTTP_PROFILE_MAX; i++) {
    g_httpCmdEvCb[i] = NULL;
    g_httpGetDataEvCb[i] = NULL;
    g_httpSestermEvCb[i] = NULL;
  }
}
