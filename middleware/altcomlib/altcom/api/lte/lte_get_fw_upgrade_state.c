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
#include <string.h>

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_getfwupgradestate.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETFW_UPGRADE_STATE_DATA_LEN (0)
#define GETFW_UPGRADE_STATE_RESP_LEN (sizeof(struct apicmd_cmddat_getfwupgradestateres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief get FW upgrade state
 *
 * @param [out] fw_upgrade_state: FW upgrade state
 * @return  On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_fw_upgrade_state(lte_fw_upgrade_state_t *fw_upgrade_state) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR void *cmd;
  FAR struct apicmd_cmddat_getfwupgradestateres_s *res;

  /* Return error if callback is NULL */
  if (!fw_upgrade_state) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not initialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_FW_UPGRADE_STATUS,
                                         GETFW_UPGRADE_STATE_DATA_LEN, (FAR void **)&res,
                                         GETFW_UPGRADE_STATE_RESP_LEN)) {
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETFW_UPGRADE_STATE_RESP_LEN,
                        &resLen, ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return LTE_RESULT_ERROR;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  if (GETFW_UPGRADE_STATE_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (APICMD_GET_FW_UPGRADE_STATE_RES_OK != ret && APICMD_GET_FW_UPGRADE_STATE_RES_ERR != ret) {
    DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  fw_upgrade_state->state = (fw_upgrade_state_t)res->state;
  fw_upgrade_state->fota_result = (fw_upgrade_result_t)res->fota_result;
  fw_upgrade_state->image_downloaded_size = ntohl(res->image_downloaded_size);
  fw_upgrade_state->image_total_size = ntohl(res->image_total_size);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
