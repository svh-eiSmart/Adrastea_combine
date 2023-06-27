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
#include <stdbool.h>

#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_atcmd.h"
#include "apicmd_atcmdRegUnsol.h"
#include "apicmdhdlr_atcmdUrcEvt.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define ATCMDREGUNSOL_REQ_DATALEN (sizeof(struct apicmd_atcmdregunsol_s))
#define ATCMDREGUNSOL_RES_DATALEN (sizeof(struct apicmd_atcmdregunsolres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: atcmdRegUnsol_request
 *
 * Description:
 *   Send APICMDID_ATCMD_REGURC.
 *
 ****************************************************************************/

static ATCMDError_e atcmdRegUnsol_request(bool reg1dereg0, const char *urcStr) {
  int32_t ret;
  uint16_t sendLen, resLen;
  struct apicmd_atcmdregunsol_s *cmd = NULL;
  struct apicmd_atcmdregunsolres_s *res = NULL;

  /* Allocate send and response command buffer */
  sendLen = ATCMDREGUNSOL_REQ_DATALEN - ATCMD_MAX_URCLEN_REQUEST + strlen(urcStr) + 1;
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_ATCMDCONN_REGURC, sendLen,
                                          (void **)&res, ATCMDREGUNSOL_RES_DATALEN)) {
    return ATCMD_FAILURE;
  }

  /* Fill the data */
  cmd->reg1dereg0 = (uint8_t)reg1dereg0;
  memcpy(cmd->regData, urcStr, sendLen);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, ATCMDREGUNSOL_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = ATCMD_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != ATCMDREGUNSOL_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = ATCMD_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != ATCMD_SUCCESS) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[atcmdRegUnsol-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (ATCMDError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_atcmdRegUnsol() Register URCs and corresponding callback.
 *
 * @param [in] urcStr: The URC string to be registered.
 * @param [in] urcCb: A corresponding callback to URC; NULL value implies to deregister URCs.
 * @param [in] cbParam: An user parameter to the urcCb.
 *
 * @return ATCMD_SUCCESS on success;  ATCMD_FAILURE on failure.
 */

ATCMDError_e altcom_atcmdRegUnsol(const char *urcStr, AtcmdUrcCbFunc_t urcCb, void *cbParam) {
  ATCMDError_e ret;

  /* Check parameters */
  if (NULL == urcStr || ATCMD_MAX_URCLEN_REQUEST - 1 < strlen(urcStr)) {
    DBGIF_LOG_ERROR("Invalid urcStr\n");
    return ATCMD_FAILURE;
  }

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return ATCMD_FAILURE;
  }

  /* Check if callback registering only */
  if (altcom_isCbRegOnly()) {
    return atcmdHelper_RegisterCallback(urcStr, urcCb, cbParam);
  }

  /* Send request */
  ret = atcmdRegUnsol_request(urcCb != NULL ? true : false, urcStr);
  if (ret != ATCMD_SUCCESS) {
    return ret;
  }

  /* Register callback for these URCs */
  return atcmdHelper_RegisterCallback(urcStr, urcCb, cbParam);
}
