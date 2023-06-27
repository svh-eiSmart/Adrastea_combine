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
 * Included Filesd
 ****************************************************************************/
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_misc.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"
#include "apicmd_GetTime.h"
#include "apicmd_AppLog.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APPLOG_DATA_LEN (sizeof(struct apicmd_applog_s))
#define APPLOG_RESP_LEN (sizeof(struct apicmd_applogres_s))

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
 * @brief Send the SfpLogger to the  MAP.
 *
 * @param [inout] sfplogger: T.
 *
 * @return
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_SendAppLog(int buffer_size, char *sfpdbp) {
  if (sfpdbp == NULL) {
    DBGIF_LOG_ERROR("sfpdbp argument is NULL.\n");
    return MISC_FAILURE;
  }
  // check buffer size to prevent memory leak
  if (buffer_size > MISC_MAX_APPLOG_BUFFER_LEN) {
    DBGIF_LOG1_ERROR("Invalid buffer_size length %hu\n", buffer_size);
    return MISC_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return MISC_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_applog_s *cmd = NULL;
  FAR struct apicmd_applogres_s *res = NULL;
  Misc_Error_e result = MISC_FAILURE;

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MISC_APPLOG, APPLOG_DATA_LEN,
                                         (FAR void **)&res, APPLOG_RESP_LEN)) {
    memcpy(cmd->applogBuffer, sfpdbp, (size_t)buffer_size);
    cmd->applogBufferLen = htons(buffer_size);
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, APPLOG_RESP_LEN, &resLen,
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

  /* Check API return code */
  result = (Misc_Error_e)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}

/**
 * @brief Print the status of Send the SfpLogger to the  MAP.
 *
 * @param [inout] Status name of the procedure: T.
 *
 * @No return
 *
 */
