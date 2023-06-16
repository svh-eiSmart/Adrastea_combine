/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
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

   ------------------------------------------------------------------------- */

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
extern void *g_coapCmdEvCbPriv;
extern coapRstEvCb g_coapRstEvCb;
extern void *g_coapRstEvCbPriv;
extern coapTermEvCb g_coapTermEvCb;
extern void *g_coapTermEvCbPriv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
   @brief coap_EventRegister() 		Callback registration

   @param [in] evt: 				Selected type of callback see @ref Coap_Ev_type_e.
   @param [in] cb:					Callback pointer, see @ref coapCmdEvCb, @ref
   coapRstEvCb and @ref coapermEvCb
   @param [in] priv: 				User provided private data.

   @return COAP_SUCCESS or COAP_FAILURE.
*/

Coap_err_code coap_EventRegister(Coap_Ev_type_e evt, void *cb, void *priv) {
  int32_t ret = 0;

  switch (evt) {
    case COAP_CB_CMDS:
      if (cb) {
        ALTCOM_REG_CALLBACK(ret, g_coapCmdEvCb, cb, g_coapCmdEvCbPriv, priv);
      } else {
        ALTCOM_CLR_CALLBACK(g_coapCmdEvCb, g_coapCmdEvCbPriv);
      }

      break;

    case COAP_CB_CMDRST:
      if (cb) {
        ALTCOM_REG_CALLBACK(ret, g_coapRstEvCb, cb, g_coapRstEvCbPriv, priv);
      } else {
        ALTCOM_CLR_CALLBACK(g_coapRstEvCb, g_coapRstEvCbPriv);
      }

      break;

    case COAP_CB_CMDTERM:
      if (cb) {
        ALTCOM_REG_CALLBACK(ret, g_coapTermEvCb, cb, g_coapTermEvCbPriv, priv);
      } else {
        ALTCOM_CLR_CALLBACK(g_coapTermEvCb, g_coapTermEvCbPriv);
      }

      break;

    default:
      DBGIF_LOG1_ERROR("Invalid event :%ld.\n", (int32_t)evt);
      return COAP_FAILURE;
  }

  if (ret != 0) {
    DBGIF_LOG1_ERROR("coap_EventRegister fail, ret:%ld.\n", (int32_t)evt);
  }

  return ret == 0 ? COAP_SUCCESS : COAP_FAILURE;
}
