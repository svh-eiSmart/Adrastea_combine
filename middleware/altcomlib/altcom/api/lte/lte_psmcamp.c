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

#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "dbg_if.h"
#include "altcom_osal.h"
#include "apiutil.h"
#include "apicmdgw.h"
#include "apicmd_psmcamp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PSMCAMP_DATA_LEN (sizeof(struct apicmd_cmddat_psmcamp_s))
#define PSMCAMP_RESP_LEN (sizeof(struct apicmd_cmddat_psmcampres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_psm_camp() enable/disable cell camping while still staying in PSM network state.
 *
 * @param [in] enable: enable/disable camping
 * @param [in] timeout: how long to try and camp on a cell in sec. valid range is 1~12000, 0 = means
 * the system default of 2min
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_psm_camp(lteenableflag_e enable, uint32_t timeout) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_psmcamp_s *cmd;
  FAR struct apicmd_cmddat_psmcampres_s *res = NULL;

  /* Return error if parameter is invalid */
  if (LTE_ENABLE < enable) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return LTE_RESULT_ERROR;
  }

  if (LTE_MAX_PSMCAMP_ATTAMPT_TIMEOUT < timeout) {
    DBGIF_LOG1_ERROR("Invalid timeout, %lu.\n", timeout);
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_PSM_CAMP, PSMCAMP_DATA_LEN,
                                         (FAR void **)&res, PSMCAMP_RESP_LEN)) {
    cmd->enability = (uint8_t)enable;
    cmd->timeout = htonl(timeout);

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, PSMCAMP_RESP_LEN, &resLen,
                        ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return LTE_RESULT_ERROR;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  if (PSMCAMP_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
