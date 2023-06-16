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
#include "apicmd_httpClear.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_CLEAR_RES_DATALEN (sizeof(struct apicmd_httpClear_res_s))
#define HTTP_CLEAR_REQ_DATALEN (sizeof(struct apicmd_httpClear_s))
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
Http_err_code_e altcom_http_clear_profile(Http_profile_id_e profile_id) {
  int32_t ret = HTTP_FAILURE;
  uint16_t resLen = 0;

  FAR struct apicmd_httpClear_s *cmd = NULL;
  FAR struct apicmd_httpClear_res_s *res = NULL;

  DBGIF_LOG_DEBUG("Into altcom_http_clear_profile()");

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_HTTP_CMDCLEAR,
                                          HTTP_CLEAR_REQ_DATALEN, (FAR void **)&res,
                                          HTTP_CLEAR_RES_DATALEN)) {
    return ret;
  }

  /* Fill the data */
  cmd->profileId = profile_id;
  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", profile_id);
    goto sign_out;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, HTTP_CLEAR_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != HTTP_CLEAR_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);

  DBGIF_LOG1_DEBUG("[altcom_httpClear-res]ret: %d\n", ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
