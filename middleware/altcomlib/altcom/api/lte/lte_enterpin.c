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
#include <stdint.h>
#include <errno.h>
#include <ctype.h>

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_enterpin.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ENTERPIN_DATA_LEN (sizeof(struct apicmd_cmddat_enterpin_s))
#define ENTERPIN_RESP_LEN (sizeof(struct apicmd_cmddat_enterpinres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_enter_pin() enter Personal Identification Number(PIN).
 *
 * @param [in] pincode: Current PIN code. Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [in] new_pincode: If not used, set NULL.
 * If the PIN is SIM PUK or SIM PUK2, the new_pincode is required.
 * Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param[inout] simstate: State after PIN enter.
 * See @ref ltesimstate
 * @param[inout] attemptsleft : Number of attempts left.
 * It is set only if the result is not successful.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_enter_pin(int8_t *pincode, int8_t *new_pincode, ltesimstate_e *simstate,
                          uint8_t *attemptsleft) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_enterpin_s *cmd;
  FAR struct apicmd_cmddat_enterpinres_s *res;
  uint8_t pinlen = 0;

  /* Return error if argument is NULL */
  if (!pincode || !simstate || !attemptsleft) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  pinlen = strlen((FAR char *)pincode);
  if (LTE_MIN_PINCODE_LEN > pinlen || LTE_MAX_PINCODE_LEN < pinlen) {
    DBGIF_LOG1_ERROR("Invalid length of pin code %d.\n", pinlen);
    return LTE_RESULT_ERROR;
  } else {
    uint8_t i;
    for (i = 0; i < pinlen; i++) {
      if (!isdigit((int)pincode[i])) {
        DBGIF_LOG_ERROR("Invalid pin code, only 0~9 are valid characters\n");
        return LTE_RESULT_ERROR;
      }
    }
  }

  if (new_pincode) {
    pinlen = strlen((FAR char *)new_pincode);
    if (LTE_MIN_PINCODE_LEN > pinlen || LTE_MAX_PINCODE_LEN < pinlen) {
      DBGIF_LOG1_ERROR("Invalid length of new pin code %d.\n", pinlen);
      return LTE_RESULT_ERROR;
    } else {
      uint8_t i;
      for (i = 0; i < pinlen; i++) {
        if (!isdigit((int)new_pincode[i])) {
          DBGIF_LOG_ERROR("Invalid pin code, only 0~9 are valid characters\n");
          return LTE_RESULT_ERROR;
        }
      }
    }
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ENTER_PIN, ENTERPIN_DATA_LEN,
                                         (FAR void **)&res, ENTERPIN_RESP_LEN)) {
    strncpy((FAR char *)cmd->pincode, (FAR char *)pincode, sizeof(cmd->pincode) - 1);

    if (new_pincode) {
      cmd->newpincodeuse = APICMD_ENTERPIN_NEWPINCODE_USE;
      strncpy((FAR char *)cmd->newpincode, (FAR char *)new_pincode, sizeof(cmd->newpincode) - 1);
    }
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, ENTERPIN_RESP_LEN, &resLen,
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

  if (ENTERPIN_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  *simstate = (ltesimstate_e)res->simstat;
  *attemptsleft = res->attemptsleft;
  if (APICMD_ENTERPIN_RES_OK != ret && APICMD_ENTERPIN_RES_ERR != ret) {
    DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
