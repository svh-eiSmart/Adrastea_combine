/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_getpdpaddr.h"
#include "buffpoolwrapper.h"
#include "altcom_helperlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETPDPADDR_DATA_LEN (sizeof(struct apicmd_cmddat_getpdpaddr_s))
#define GETPDPADDR_RESP_LEN (sizeof(struct apicmd_cmddat_getpdpaddrres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Get active PDN information
 *
 * @param [in] cid: The cid of PDP context.
 * @param [out] pdp_addr: Address list of PDP context with specified cid.
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_pdp_addr(uint32_t cid, lte_pdp_addr_t *pdp_addr) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_getpdpaddr_s *cmd;
  FAR struct apicmd_cmddat_getpdpaddrres_s *res;

  /* Return error if parameter is NULL */
  if (!pdp_addr) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_PDPADDR,
                                         GETPDPADDR_DATA_LEN, (FAR void **)&res,
                                         GETPDPADDR_RESP_LEN)) {
    cmd->cid = htonl(cid);
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETPDPADDR_RESP_LEN, &resLen,
                        ALT_OSAL_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return LTE_RESULT_ERROR;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  if (GETPDPADDR_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (APICMD_GETPDPADDR_RES_OK == ret) {
    uint8_t i;
    pdp_addr->cid = htonl(res->cid);

    for (i = 0; i < 2; i++) {
      pdp_addr->addr[i].is_valid = (bool)res->pdpaddr[i].is_valid;
      if (pdp_addr->addr[i].is_valid) {
        pdp_addr->addr[i].ip_type = (lteapniptype_e)res->pdpaddr[i].ip_type;
        DBGIF_ASSERT(pdp_addr->addr[i].ip_type <= LTE_APN_IPTYPE_IPV6, "Invalid ip_type %lu",
                     (uint32_t)pdp_addr->addr[i].ip_type);

        if (pdp_addr->addr[i].ip_type == LTE_APN_IPTYPE_IP) {
          snprintf(pdp_addr->addr[i].ip_str, LTE_IP_STRLEN, "%s", res->pdpaddr[i].ip_str);
        } else if (pdp_addr->addr[i].ip_type == LTE_APN_IPTYPE_IPV6) {
          if (altcom_helperlib_conv_dot2colon_v6(res->pdpaddr[i].ip_str, pdp_addr->addr[i].ip_str) <
              0) {
            pdp_addr->addr[i].ip_str[0] = '\0';
          }
        }
      }
    }
  } else {
    DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
