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
#include "apicmd_cmd_abort.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COAP_ABORT_RES_DATALEN (sizeof(struct apicmd_coapAbort_res_s))
#define COAP_ABORT_REQ_DATALEN (sizeof(struct apicmd_coapAbort_s))
#define ISPROFILE_VALID(p) (p > 0 && p <= 5)
#define CMD_TIMEOUT 10000 /* 10 secs */
#define COAPEV_RET_SUCCESS 0

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 @brief altcom_coap_cmd():          Commands to communicate with COAP .

 @param [in]  profileId:            Assigned profile between 1 and 5
 @param [in]  CmdParams:            Communication parameters. See @ref CoapCmdData_t.
 @param [out] Token:                Automatically generated token.

 @return:                           COAP_SUCCESS or COAP_FAILURE.
 */

Coap_err_code altcom_coap_abort_profile(uint8_t profileId) {
  struct apicmd_coapAbort_res_s *res = NULL;
  struct apicmd_coapAbort_s *cmd = NULL;
  int32_t ret = 0;
  uint16_t resLen = 0;
  Coap_err_code err = COAP_FAILURE;

  DBGIF_LOG_DEBUG("Into altcom_coap_abort_profile()");

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_COAP_CMDABORT,
                                          COAP_ABORT_REQ_DATALEN, (FAR void **)&res,
                                          COAP_ABORT_RES_DATALEN)) {
    goto end;
  }

  /* Fill the data */
  cmd->profileId = profileId;
  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", profileId);
    goto sign_out;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, COAP_ABORT_RES_DATALEN, &resLen, CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != COAP_ABORT_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_coapAbort-res]ret: %d\n", ret);

  if (ret == (int32_t)COAPEV_RET_SUCCESS) err = COAP_SUCCESS;

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

end:
  return err;
}
