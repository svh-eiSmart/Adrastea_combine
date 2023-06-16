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
#include "apicmd_httpReadData.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_READDATA_RES_DATALEN (sizeof(struct apicmd_http_readData_res_s))
#define HTTP_READDATA_REQ_DATALEN (sizeof(struct apicmd_http_readData_s))
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
Http_err_code_e altcom_http_read_data(Http_profile_id_e profile_id, uint16_t chunk_len, void *buf,
                                      uint16_t *read_len, uint32_t *pend_len) {
  struct apicmd_http_readData_res_s *res = NULL;
  struct apicmd_http_readData_s *cmd = NULL;
  int32_t ret = HTTP_FAILURE;
  uint16_t resLen = 0;

  DBGIF_LOG_DEBUG("altcom_http_read_data()");

  if (chunk_len == 0) {
    DBGIF_LOG_ERROR("Chunk length parameter is missing \n");
    return ret;
  }

  if (chunk_len > HTTP_MAX_DATA_LENGTH) {
    DBGIF_LOG_ERROR("Chunk length exceeds maximum \n");
    return ret;
  }

  if (read_len == NULL || pend_len == NULL) {
    DBGIF_LOG2_ERROR("Invalid parameter readlen(%p) or pendlen(%p)\n", read_len, pend_len);
    return ret;
  }

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_HTTP_CMDREADDATA,
                                          HTTP_READDATA_REQ_DATALEN, (FAR void **)&res,
                                          HTTP_READDATA_RES_DATALEN)) {
    return ret;
  }

  /* Validate parameters */

  if (!ISPROFILE_VALID(profile_id)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", cmd->profileId);
    goto sign_out;
  }

  cmd->profileId = profile_id;

  /* Chunk length */
  DBGIF_LOG1_DEBUG("Chunk length (%d)", chunk_len);
  cmd->chunkLen = htons(chunk_len);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, HTTP_READDATA_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  *read_len = ntohs(res->readLen);
  if (*read_len != 0) {
    memcpy(buf, res->data, (size_t)*read_len);
  }

  *pend_len = ntohl(res->pendLen);
  DBGIF_LOG3_DEBUG("[altcom_http_read_data-res]read_len: %hu, pend_len: %hu, ret: %d\n", *read_len,
                   *pend_len, ret);

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return (Http_err_code_e)ret;
}
