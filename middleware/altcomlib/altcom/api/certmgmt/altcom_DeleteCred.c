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
#include "apicmd_DeleteCred.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define DELETECRED_DATA_LEN (sizeof(struct apicmd_deletecred_s))
#define DELETECRED_RESP_LEN (sizeof(struct apicmd_deletecredres_s))

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
 * @brief Delete the specified credential file from MAP storage.
 *
 * @param [in] credName: Name of the target credential to delete.
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_DeleteCredential(const char *credName) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_deletecred_s *cmd = NULL;
  FAR struct apicmd_deletecredres_s *res = NULL;
  uint16_t credNameLen;
  CertMgmt_Error_e result;

  /* Return error if parameter is invalid */
  if (!credName) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return CERTMGMT_FAILURE;
  }

  credNameLen = strlen(credName);
  if (0 == credNameLen || CERTMGMT_MAX_FILENAME_LEN < credNameLen) {
    DBGIF_LOG1_ERROR("Invalid credName length %hu\n", credNameLen);
    return CERTMGMT_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return CERTMGMT_FAILURE;
  }

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_DELETE_CREDENTIAL,
                                         DELETECRED_DATA_LEN, (FAR void **)&res,
                                         DELETECRED_RESP_LEN)) {
    memcpy(cmd->credName, credName, credNameLen);
    cmd->credName[credNameLen] = 0;

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, DELETECRED_RESP_LEN, &resLen,
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

  if (DELETECRED_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    result = CERTMGMT_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  result = (CertMgmt_Error_e)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
