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
#include "apicmd_scanscheme.h"
#include "apicmd_repevt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define SCANSCHEME_DATA_LEN (sizeof(struct apicmd_cmddat_scanscheme_s))
#define SCANSCHEME_RESP_LEN (sizeof(struct apicmd_cmddat_scanschemeres_s))

/****************************************************************************
 * External Function
 ****************************************************************************/
extern int32_t lte_set_report_scanresult(scanresult_report_cb_t scanresult_callback,
                                         void *userPriv);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief lte_set_scan_scheme() set the scanning scheme.
 *
 * @param [in] scheme: The scan scheme ptr
 *
 * @return On success, LTE_RESULT_OK is returned; On failure, LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_scan_scheme(lte_scan_scheme_t *scheme) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_scanscheme_s *cmd;
  FAR struct apicmd_cmddat_scanschemeres_s *res;

  /* Return error if argument is NULL */
  if (!scheme) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SCAN_SCHEME,
                                         SCANSCHEME_DATA_LEN, (FAR void **)&res,
                                         SCANSCHEME_RESP_LEN)) {
    cmd->set1_get2 = 1;
    cmd->num_mru_scans = htons(scheme->num_mru_scans);
    cmd->num_country_scans = htons(scheme->num_country_scans);
    cmd->num_region_scans = htons(scheme->num_region_scans);
    cmd->num_full_scans = scheme->num_full_scans;
    cmd->fallback_full_to_country = scheme->fallback_full_to_country;
    cmd->slptime_btwn_scans = htonl(scheme->slptime_btwn_scans);
    cmd->max_slptime_btwn_scans = htonl(scheme->max_slptime_btwn_scans);
    cmd->slptime_step = htons(scheme->slptime_step);

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, SCANSCHEME_RESP_LEN, &resLen,
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

  if (SCANSCHEME_RESP_LEN != resLen) {
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
/**
 * @brief lte_get_scan_scheme() get the scanning scheme.
 *
 * @param [out] scheme: The scan scheme, and it's valid only when LTE_RESULT_OK returned.
 *
 * @return On success, LTE_RESULT_OK is returned; On failure, LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_scan_scheme(lte_scan_scheme_t *scheme) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_scanscheme_s *cmd;
  FAR struct apicmd_cmddat_scanschemeres_s *res;

  /* Return error if argument is NULL */
  if (!scheme) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SCAN_SCHEME,
                                         SCANSCHEME_DATA_LEN, (FAR void **)&res,
                                         SCANSCHEME_RESP_LEN)) {
    cmd->set1_get2 = 2;

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, SCANSCHEME_RESP_LEN, &resLen,
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

  if (SCANSCHEME_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  if ((ret = (int32_t)res->result) == APICMD_SCANSCHEME_RES_OK) {
    scheme->num_mru_scans = ntohs(res->num_mru_scans);
    scheme->num_country_scans = ntohs(res->num_country_scans);
    scheme->num_region_scans = ntohs(res->num_region_scans);
    scheme->num_full_scans = res->num_full_scans;
    scheme->fallback_full_to_country = res->fallback_full_to_country;
    scheme->slptime_btwn_scans = ntohl(res->slptime_btwn_scans);
    scheme->max_slptime_btwn_scans = ntohl(res->max_slptime_btwn_scans);
    scheme->slptime_step = ntohs(res->slptime_step);
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
