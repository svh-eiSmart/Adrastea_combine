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
#include "apicmd_getpdninfo.h"
#include "buffpoolwrapper.h"
#include "altcom_helperlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GETPDNINFO_DATA_LEN (sizeof(struct apicmd_cmddat_getpdninfo_s))
#define GETPDNINFO_RESP_LEN (sizeof(struct apicmd_cmddat_getpdninfores_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static char *find_nmask_pos(char *src, lteapniptype_e ip_type) {
  if (src == NULL || ip_type >= LTE_APN_IPTYPE_IPV4V6) {
    return NULL;
  }

  size_t srclen = strlen(src);

  if (!srclen) {
    return NULL;
  }

  char *src_cpy;
  int parse_cnt;

  src_cpy = BUFFPOOL_ALLOC(srclen + 1);
  DBGIF_ASSERT(src_cpy, "BUFFPOOL_ALLOC fail\n");

  memcpy((void *)src_cpy, (void *)src, srclen + 1);
  parse_cnt = (ip_type == LTE_APN_IPTYPE_IP ? 4 : 16);

  int i;
  char *token, *saveptr, *src_tmp;

  for (i = 0, src_tmp = src_cpy; i < parse_cnt; i++, src_tmp = NULL) {
    token = strtok_r(src_tmp, ".", &saveptr);
    if (token == NULL) {
      break;
    }
  }

  BUFFPOOL_FREE(src_cpy);
  return (i != parse_cnt ? NULL : src + (saveptr - src_cpy));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Get active PDN information
 *
 * @param [in] cid: The cid of active PDN.
 * @param [out] pdn_info: information of target DN with specified cid.
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_pdninfo(uint32_t cid, lte_pdn_info_t *pdn_info) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_getpdninfo_s *cmd;
  FAR struct apicmd_cmddat_getpdninfores_s *res;

  /* Return error if parameter is NULL */
  if (!pdn_info) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_PDNINFO,
                                         GETPDNINFO_DATA_LEN, (FAR void **)&res,
                                         GETPDNINFO_RESP_LEN)) {
    cmd->cid = htonl(cid);
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GETPDNINFO_RESP_LEN, &resLen,
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

  if (GETPDNINFO_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  ret = (int32_t)res->result;
  if (APICMD_GETPDNINFO_RES_OK == ret) {
    uint8_t i;

    for (i = 0; i < 2; i++) {
      pdn_info->pdn[i].is_valid = (bool)res->pdninfo[i].is_valid;
      if (pdn_info->pdn[i].is_valid) {
        pdn_info->pdn[i].ip_type = (lteapniptype_e)res->pdninfo[i].ip_type;
        DBGIF_ASSERT(pdn_info->pdn[i].ip_type <= LTE_APN_IPTYPE_IPV6, "Invalid ip_type %lu",
                     (uint32_t)pdn_info->pdn[i].ip_type);

        pdn_info->pdn[i].cid = htonl(res->pdninfo[i].cid);
        pdn_info->pdn[i].bearer_id = htonl(res->pdninfo[i].bearer_id);
        memcpy((void *)pdn_info->pdn[i].apn_name, (void *)res->pdninfo[i].apn_name, LTE_APN_LEN);

        char *nmask_pos;

        nmask_pos = find_nmask_pos(res->pdninfo[i].ip_nmask_str, pdn_info->pdn[i].ip_type);
        if (pdn_info->pdn[i].ip_type == LTE_APN_IPTYPE_IP) {
          DBGIF_ASSERT(nmask_pos, "NULL IP/GW string\n");

          snprintf(pdn_info->pdn[i].ip_str, nmask_pos - &res->pdninfo[i].ip_nmask_str[0], "%s",
                   res->pdninfo[i].ip_nmask_str);
          snprintf(pdn_info->pdn[i].nmask_str, LTE_IP_STRLEN, "%s", nmask_pos);
          memcpy((void *)pdn_info->pdn[i].gw_str, (void *)res->pdninfo[i].gw_str, LTE_IP_STRLEN);
          memcpy((void *)pdn_info->pdn[i].pri_dns_str, (void *)res->pdninfo[i].pri_dns_str,
                 LTE_IP_STRLEN);
          memcpy((void *)pdn_info->pdn[i].sec_dns_str, (void *)res->pdninfo[i].sec_dns_str,
                 LTE_IP_STRLEN);
        } else if (pdn_info->pdn[i].ip_type == LTE_APN_IPTYPE_IPV6) {
          DBGIF_ASSERT(nmask_pos, "NULL IP/GW string\n");

          if (altcom_helperlib_conv_dot2colon_v6(res->pdninfo[i].ip_nmask_str,
                                                 pdn_info->pdn[i].ip_str) < 0) {
            pdn_info->pdn[i].ip_str[0] = '\0';
          }

          if (altcom_helperlib_conv_dot2colon_v6(res->pdninfo[i].gw_str, pdn_info->pdn[i].gw_str) <
              0) {
            pdn_info->pdn[i].gw_str[0] = '\0';
          }

          if (altcom_helperlib_conv_dot2colon_v6(res->pdninfo[i].pri_dns_str,
                                                 pdn_info->pdn[i].pri_dns_str) < 0) {
            pdn_info->pdn[i].pri_dns_str[0] = '\0';
          }

          if (altcom_helperlib_conv_dot2colon_v6(res->pdninfo[i].sec_dns_str,
                                                 pdn_info->pdn[i].sec_dns_str) < 0) {
            pdn_info->pdn[i].sec_dns_str[0] = '\0';
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
