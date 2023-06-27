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
#include "apicmd_GetCredList.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define GETCREDLIST_DATA_LEN (sizeof(struct apicmd_getcredlist_s))
#define GETCREDLIST_RESP_LEN (sizeof(struct apicmd_getcredlistres_s))

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
 * @brief Get the credential list in trusted folder stored on MAP.
 *
 * @param [in] caPath: Specify which trusted folder to list. See @ref CredentialType_e.
 * @param [inout] credList: Caller provided buffer to store the desired list of credential. The
 * output is a NULL-Terminated string.
 * @param [in] listBufLen: Buffer length of credList.
 * @param [inout] listLen: Real length of credential list, this parameter could be used to check if
 * the size of provided buffer sufficient or not. Only valid on @ref CERTMGMT_SUCCESS / @ref
 * CERTMGMT_INSUFFICIENT_BUFFER returned.
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_INSUFFICIENT_BUFFER  | Insufficient buffer size
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_GetCredentialList(TrustedCaPath_e caPath, uint8_t *credList,
                                          uint16_t listBufLen, uint16_t *listLen) {
  /* Return error if parameter is invalid */
  if (!credList || !listLen) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return CERTMGMT_FAILURE;
  }

  if (CAPATH_USER < caPath) {
    DBGIF_LOG1_ERROR("Invalid caPath type %lu\n", (uint32_t)caPath);
    return CERTMGMT_FAILURE;
  }

  if (0 == listBufLen) {
    DBGIF_LOG1_ERROR("Invalid listBufLen length %hu\n", listBufLen);
    return CERTMGMT_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return CERTMGMT_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_getcredlist_s *cmd = NULL;
  FAR struct apicmd_getcredlistres_s *res = NULL;
  uint16_t remainLen = listBufLen, readLen = 0, accLen = 0;
  uint32_t handle = 0;
  CertMgmt_Error_e result = CERTMGMT_SUCCESS;

  do {
    altcom_generic_free_cmdandresbuff(cmd, res);
    /* Allocate API command buffer to send */
    if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_CREDLIST,
                                           GETCREDLIST_DATA_LEN, (FAR void **)&res,
                                           GETCREDLIST_RESP_LEN)) {
      cmd->handle = htonl(handle);
      cmd->reqLen = htons(remainLen);  // The first remainLen used to confirm the buffer sufficiency
      cmd->creddir = (uint8_t)caPath;

      /* Send API command to modem */
      ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETCREDLIST_RESP_LEN, &resLen,
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

    if (GETCREDLIST_RESP_LEN != resLen) {
      DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
      result = CERTMGMT_FAILURE;
      goto errout_with_cmdfree;
    }

    /* Check API return code */
    result = (CertMgmt_Error_e)res->result;
    if (CERTMGMT_SUCCESS == result) {
      handle = ntohl(res->handle);
      readLen = ntohs(res->readLen);
      DBGIF_ASSERT(CERTMGMT_MAX_LISTCHUNK_LEN >= readLen, "Impossible case, we assert here!");
      if (0 == readLen) {
        if (handle) {
          remainLen = 0;
          DBGIF_LOG_DEBUG("Crednetial list read finish, close remote handle\n");
        } else {
          *listLen = accLen;
          DBGIF_LOG1_DEBUG("Crednetial list length: %hu bytes\n", accLen);
          break;
        }

      } else {
        memcpy(credList + accLen, res->listData, (size_t)readLen);
        remainLen -= readLen;
        accLen += readLen;
      }
    } else if (CERTMGMT_INSUFFICIENT_BUFFER == result) {
      *listLen = (uint32_t)ntohs(res->readLen);
      DBGIF_LOG2_ERROR("credBuf too small: %hu, needs: %hu\n", listBufLen, *listLen);
      goto errout_with_cmdfree;
    } else {
      DBGIF_LOG1_ERROR("Credential list read failed: %lu\n", (uint32_t)result);
      goto errout_with_cmdfree;
    }
  } while (true);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
