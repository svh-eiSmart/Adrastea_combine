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
#include "altcom_certmgmt.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"
#include "apicmd_ReadCred.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define READCRED_DATA_LEN (sizeof(struct apicmd_readcred_s))
#define READCRED_RESP_LEN (sizeof(struct apicmd_readcredres_s))

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
 * @brief Read credential file from MAP storage.
 *
 * @param [in] credName: Name of target certificate. It must be a NULL-Terminated string, and length
 * should less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] credBuf: Caller provided buffer to store the content of target certificate.
 * @param [in] credBufLen: Buffer length of certBuf.
 * @param [inout] credLen: Real length of certificate, this parameter could be used to check if the
 * size of provided buffer sufficient or not. Only valid on @ref CERTMGMT_SUCCESS / @ref
 * CERTMGMT_INSUFFICIENT_BUFFER returned.
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_INSUFFICIENT_BUFFER  | Insufficient buffer size
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_ReadCredential(const char *credName, uint8_t *credBuf, uint16_t credBufLen,
                                       uint16_t *credLen) {
  uint16_t credNameLen;

  /* Return error if parameter is invalid */
  if (!credName || !credBuf || !credLen) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return CERTMGMT_FAILURE;
  }

  credNameLen = strlen(credName);
  if (0 == credNameLen || CERTMGMT_MAX_FILENAME_LEN < credNameLen) {
    DBGIF_LOG1_ERROR("Invalid credName length %hu\n", credNameLen);
    return CERTMGMT_FAILURE;
  }

  if (0 == credBufLen) {
    DBGIF_LOG1_ERROR("Invalid credBufLen length %hu\n", credBufLen);
    return CERTMGMT_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return CERTMGMT_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_readcred_s *cmd = NULL;
  FAR struct apicmd_readcredres_s *res = NULL;
  uint16_t remainLen = credBufLen, readLen = 0, accLen = 0;
  uint32_t handle = 0;
  CertMgmt_Error_e result = CERTMGMT_SUCCESS;

  do {
    altcom_generic_free_cmdandresbuff(cmd, res);
    /* Allocate API command buffer to send */
    if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_READ_CREDENTIAL,
                                           READCRED_DATA_LEN, (FAR void **)&res,
                                           READCRED_RESP_LEN)) {
      cmd->handle = htonl(handle);
      cmd->reqLen = htons(remainLen);  // The first remainLen used to confirm the buffer sufficiency
      memcpy(cmd->credName, credName, (size_t)credNameLen);
      cmd->credName[credNameLen] = 0;

      /* Send API command to modem */
      ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, READCRED_RESP_LEN, &resLen,
                          ALTCOM_SYS_TIMEO_FEVR);
    } else {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      return CERTMGMT_FAILURE;
    }

    if (0 > ret) {
      DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
      result = CERTMGMT_FAILURE;
      goto errout_with_cmdfree;
    }

    if (READCRED_RESP_LEN != resLen) {
      DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
      result = CERTMGMT_FAILURE;
      goto errout_with_cmdfree;
    }

    /* Check API return code */
    result = (CertMgmt_Error_e)res->result;
    if (CERTMGMT_SUCCESS == result) {
      handle = ntohl(res->handle);
      readLen = ntohs(res->readLen);
      DBGIF_ASSERT(CERTMGMT_MAX_READCHUNK_LEN >= readLen, "Impossible case, we assert here!");
      if (0 == readLen) {
        if (handle) {
          remainLen = 0;
          DBGIF_LOG_DEBUG("Crednetial read finish, close remote handle\n");
        } else {
          *credLen = accLen;
          DBGIF_LOG1_DEBUG("Crednetial length: %hu bytes\n", accLen);
          break;
        }
      } else {
        memcpy(credBuf + accLen, res->credData, (size_t)readLen);
        remainLen -= readLen;
        accLen += readLen;
      }
    } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
      *credLen = (uint32_t)ntohs(res->readLen);
      DBGIF_LOG2_ERROR("credBuf too small: %hu, needs: %hu\n", credBufLen, *credLen);
      goto errout_with_cmdfree;
    } else {
      DBGIF_LOG1_ERROR("Credential read failed: %lu\n", (uint32_t)result);
      goto errout_with_cmdfree;
    }
  } while (true);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
