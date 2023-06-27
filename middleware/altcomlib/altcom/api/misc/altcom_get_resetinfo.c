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
#include <time.h>

#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_misc.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_get_resetinfo.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define GET_RESETINFO_DATA_LEN (0)
#define GET_RESETINFO_RESP_LEN (sizeof(struct apicmd_getresetinfores_s))

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
 * @brief Get the reset information from MAP.
 *
 * @param [out] info: The reset information structure.
 *
 * @return Misc_Error_e
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_get_resetinfo(ResetInfo_t *info) {
  /* Return error if parameter is invalid */
  if (!info) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return MISC_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return MISC_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  void *cmd = NULL;
  FAR struct apicmd_getresetinfores_s *res = NULL;
  Misc_Error_e result = MISC_FAILURE;

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MISC_GETRESETINFO,
                                         GET_RESETINFO_DATA_LEN, (FAR void **)&res,
                                         GET_RESETINFO_RESP_LEN)) {
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GET_RESETINFO_RESP_LEN, &resLen,
                        ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return MISC_FAILURE;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    result = MISC_FAILURE;
    goto errout_with_cmdfree;
  }

  if (GET_RESETINFO_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    result = MISC_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API result */
  if (res->result == APICMD_GETRESETINFO_RES_OK) {
    result = MISC_SUCCESS;
    info->type = (ResetType_t)res->type;
    info->cause = (ResetCause_t)res->cause;
    info->cpu = (ResetCpu_t)res->cpu;
    info->failure_type = (FailureType_t)res->failure_type;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
