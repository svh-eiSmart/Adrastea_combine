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
#include "apicmd_httpAbort.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_ABORT_RES_DATALEN (sizeof(struct apicmd_httpAbort_res_s))
#define HTTP_ABORT_REQ_DATALEN (sizeof(struct apicmd_httpAbort_s))
#define ISPROFILE_VALID(p) (p > 0 && p <= 5)
#define CMD_TIMEOUT 10000 /* 10 secs */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
Http_err_code_e altcom_http_abort_profile(Http_profile_id_e profile_id) {
  struct apicmd_httpAbort_res_s *res = NULL;
  struct apicmd_httpAbort_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE;
  uint16_t resLen = 0;

  DBGIF_LOG_DEBUG("Into altcom_http_abort_profile()");

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDABORT,
                                          HTTP_ABORT_REQ_DATALEN, (FAR void **)&res,
                                          HTTP_ABORT_RES_DATALEN)) {
    return ret;
  }

  /* Fill-in data */

  cmd->profileId = profile_id;
  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", profile_id);
    goto sign_out;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_ABORT_RES_DATALEN, &resLen, CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_ABORT_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_httpAbort-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
