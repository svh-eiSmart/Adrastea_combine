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
#include <stdbool.h>
#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_coap.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern coapCmdEvCb g_coapCmdEvCb;
extern coapRstEvCb g_coapRstEvCb;
extern coapTermEvCb g_coapTermEvCb;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 @brief coap_EventRegister():  		Register URC callbacks.

 @param [in]  *CB:            		Callback
 @param [in]  CmdParams:            Selected callback.

 @return:                           COAP_SUCCESS or COAP_FAILURE.
 */

void coap_EventRegister(void *CB, Coap_Ev_type_e evCb) {
  switch (evCb) {
    case COAP_CB_CMDS:
      g_coapCmdEvCb = NULL;
      if (CB) g_coapCmdEvCb = (coapCmdEvCb)CB;
      break;
    case COAP_CB_CMDRST:
      g_coapRstEvCb = NULL;
      if (CB) g_coapRstEvCb = (coapRstEvCb)CB;
      break;
    case COAP_CB_CMDTERM:
      g_coapTermEvCb = NULL;
      if (CB) g_coapTermEvCb = (coapTermEvCb)CB;
      break;
  }
}
