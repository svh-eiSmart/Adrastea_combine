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
#include "apicmd_WriteCred.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define WRITECRED_DATA_LEN (sizeof(struct apicmd_writecred_s))
#define WRITECRED_RESP_LEN (sizeof(struct apicmd_writecredres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int hex2ascstr(const uint8_t *src, char *dst, uint16_t srclen, uint16_t dstlen) {
  int srcidx, dstidx;

  if (dstlen < (srclen * 2 + 1)) {
    DBGIF_LOG2_ERROR("Insufficient dstlen %u, srclen %u\n", dstlen, srclen);
    return -1;
  }

  dst[dstlen - 1] = 0;
  srcidx = dstidx = 0;
  while (srclen) {
    snprintf((char *)(dst + dstidx), 3, "%02X", (unsigned int)src[srcidx]);
    srcidx++;
    dstidx += 2;
    srclen--;
  }

  DBGIF_LOG1_ERROR("len after conversion %d\n", dstidx + 1);
  return dstidx + 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Write credential data(certificate/private key) to MAP storage.
 *
 * @param [in] credType: Type of credential. See @ref CredentialType_e.
 * @param [in] credName: Name of the target credential to write. It must to be a NULL-Terminated
 * string, and length should less than @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [in] credData: Data buffer of credential to write. It must to be a NULL-Terminated string.
 * @param [in] credLen: Data length of credData.
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_WriteCredential(CredentialType_e credType, const char *credName,
                                        uint8_t *credData, uint16_t credLen) {
  uint16_t credNameLen;

  /* Return error if parameter is invalid */
  if (!credName || !credData) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return CERTMGMT_FAILURE;
  }

  credNameLen = strlen(credName);
  if (0 == credNameLen || CERTMGMT_MAX_FILENAME_LEN < credNameLen) {
    DBGIF_LOG1_ERROR("Invalid credName length %hu\n", credNameLen);
    return CERTMGMT_FAILURE;
  }

  if (0 == credLen) {
    DBGIF_LOG1_ERROR("Invalid credLen %hu\n", credLen);
    return CERTMGMT_FAILURE;
  }

  if (CREDTYPE_MAX <= credType) {
    DBGIF_LOG1_ERROR("Invalid credType %lu\n", (uint32_t)credType);
    return CERTMGMT_FAILURE;
  }

  if ((CREDTYPE_PSKID == credType) && credLen > CERTMGMT_MAX_PSKID_LEN) {
    DBGIF_LOG1_ERROR("Invalid length of PSKID %hu\n", credLen);
    return CERTMGMT_FAILURE;
  }

  if ((CREDTYPE_PSK == credType) &&
      (credLen > CERTMGMT_MAX_PSK_LEN || credLen < CERTMGMT_MIN_PSK_LEN)) {
    DBGIF_LOG1_ERROR("Invalid length of PSK %hu\n", credLen);
    return CERTMGMT_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return CERTMGMT_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_writecred_s *cmd = NULL;
  FAR struct apicmd_writecredres_s *res = NULL;
  uint16_t remainLen = credLen, writeLen = 0, accLen = 0;
  uint32_t handle = 0;
  CertMgmt_Error_e result = CERTMGMT_SUCCESS;

  do {
    altcom_generic_free_cmdandresbuff(cmd, res);
    /* Allocate API command buffer to send */
    if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_WRITE_CREDENTIAL,
                                           WRITECRED_DATA_LEN, (FAR void **)&res,
                                           WRITECRED_RESP_LEN)) {
      uint16_t chunk =
          CERTMGMT_MAX_WRITECHUNK_LEN <= remainLen ? CERTMGMT_MAX_WRITECHUNK_LEN : remainLen;

      if (CREDTYPE_PSKID == credType || CREDTYPE_PSK == credType) {
        remainLen =
            hex2ascstr(credData, (char *)cmd->credData, remainLen, (uint16_t)sizeof(cmd->credData));
        DBGIF_ASSERT(remainLen > 0, "hex2ascstr failed\r\n");

        cmd->reqLen = --remainLen;
      } else {
        cmd->reqLen = remainLen;
        if (cmd->reqLen) {
          memcpy(cmd->credData, credData + accLen, (size_t)chunk);
        }
      }

      cmd->handle = htonl(handle);
      cmd->reqLen = htons(cmd->reqLen);
      memcpy(cmd->credName, credName, credNameLen);
      cmd->credType = credType;
      cmd->credName[credNameLen] = 0;

      /* Send API command to modem */
      ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, WRITECRED_RESP_LEN, &resLen,
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

    if (WRITECRED_RESP_LEN != resLen) {
      DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
      result = CERTMGMT_FAILURE;
      goto errout_with_cmdfree;
    }

    /* Check API return code */
    result = (CertMgmt_Error_e)res->result;
    if (CERTMGMT_SUCCESS == result) {
      handle = ntohl(res->handle);
      writeLen = ntohs(res->writeLen);
      DBGIF_ASSERT(CERTMGMT_MAX_WRITECHUNK_LEN >= writeLen, "Impossible case, we assert here!");
      if (0 == writeLen) {
        remainLen = 0;
        DBGIF_LOG1_DEBUG("Crednetial write finish, %lu bytes\n", (uint32_t)accLen);
        break;
      } else {
        remainLen -= writeLen;
        accLen += writeLen;
        DBGIF_LOG1_DEBUG("Crednetial write finish, %lu bytes\n", (uint32_t)accLen);
      }
    } else {
      DBGIF_LOG1_ERROR("Credential write failed: %lu\n", (uint32_t)result);
      goto errout_with_cmdfree;
    }
  } while (true);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
