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

#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_setapn.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SETAPN_DATA_LEN (sizeof(struct apicmd_cmddat_setapn_s))
#define SETAPNV2_DATA_LEN (sizeof(struct apicmd_cmddat_setapnv2_s))
#define SETAPN_RESP_LEN (sizeof(struct apicmd_cmddat_setapnres_s))

#define SETAPN_IPTYPE_MIN LTE_APN_IPTYPE_IP
#define SETAPN_IPTYPE_MAX LTE_APN_IPTYPE_NONIP

#define SETAPN_AUTHTYPE_MIN LTE_APN_AUTHTYPE_NONE
#define SETAPN_AUTHTYPE_MAX LTE_APN_AUTHTYPE_CHAP

#define SETAPN_GET_MAX_STR_LEN(len) ((len)-1)

/****************************************************************************
 * Public Functions
 ****************************************************************************/
lteresult_e lte_set_apn(uint8_t session_id, int8_t *apn, lteapniptype_e ip_type,
                        lteapnauthtype_e auth_type, int8_t *user_name, int8_t *password) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_setapn_s *cmd;
  FAR struct apicmd_cmddat_setapnres_s *res;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Return error if parameter is invalid */
  if (LTE_SESSION_ID_MIN > session_id || LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG1_ERROR("Session ID out of range %ld.\n", (uint32_t)session_id);
    return LTE_RESULT_ERROR;
  } else if (apn && SETAPN_GET_MAX_STR_LEN(LTE_APN_LEN) < strlen((FAR char *)apn)) {
    DBGIF_LOG1_ERROR("APN name too long %lu.\n", (uint32_t)strlen((FAR char *)apn));
    return LTE_RESULT_ERROR;
  } else if (SETAPN_IPTYPE_MAX < ip_type) {
    DBGIF_LOG1_ERROR("Invalid IP type %ld.\n", (uint32_t)ip_type);
    return LTE_RESULT_ERROR;
  } else if (SETAPN_AUTHTYPE_MAX < auth_type) {
    DBGIF_LOG1_ERROR("Invalid auth type %ld.\n", (uint32_t)auth_type);
    return LTE_RESULT_ERROR;
  } else if (user_name &&
             SETAPN_GET_MAX_STR_LEN(LTE_APN_USER_NAME_LEN) < strlen((FAR char *)user_name)) {
    DBGIF_LOG1_ERROR("Username too long %lu.\n", (uint32_t)strlen((FAR char *)user_name));
    return LTE_RESULT_ERROR;
  } else if (password &&
             SETAPN_GET_MAX_STR_LEN(LTE_APN_PASSWD_LEN) < strlen((FAR char *)password)) {
    DBGIF_LOG1_ERROR("Password too long %lu.\n", (uint32_t)strlen((FAR char *)password));
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SET_APN, SETAPN_DATA_LEN,
                                         (FAR void **)&res, SETAPN_RESP_LEN)) {
    cmd->session_id = session_id;
    if (apn) {
      strncpy((FAR char *)cmd->apn, (FAR char *)apn, sizeof(cmd->apn) - 1);
    }
    cmd->ip_type = ip_type;
    cmd->auth_type = auth_type;
    if (user_name) {
      strncpy((FAR char *)cmd->user_name, (FAR char *)user_name, sizeof(cmd->user_name) - 1);
    }

    if (password) {
      strncpy((FAR char *)cmd->password, (FAR char *)password, sizeof(cmd->password) - 1);
    }

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, SETAPN_RESP_LEN, &resLen,
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

  /* Check API return code */
  ret = (int32_t)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}

lteresult_e lte_set_apnv2(uint8_t session_id, int8_t *apn, lteapniptype_e ip_type,
                          lteapnauthtype_e auth_type, int8_t *user_name, int8_t *password,
                          int8_t *hostname, lteenableflag_e pcscf_discovery,
                          lte_ipv4addralloc_t ipv4addralloc, lte_nslpi_t nslpi) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_setapnv2_s *cmd;
  FAR struct apicmd_cmddat_setapnres_s *res;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Return error if parameter is invalid */
  if (LTE_SESSION_ID_MIN > session_id || LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG1_ERROR("Session ID out of range %ld.\n", (uint32_t)session_id);
    return LTE_RESULT_ERROR;
  } else if (apn && SETAPN_GET_MAX_STR_LEN(LTE_APN_LEN) < strlen((FAR char *)apn)) {
    DBGIF_LOG1_ERROR("APN name too long %lu.\n", (uint32_t)strlen((FAR char *)apn));
    return LTE_RESULT_ERROR;
  } else if (SETAPN_IPTYPE_MAX < ip_type) {
    DBGIF_LOG1_ERROR("Invalid IP type %ld.\n", (uint32_t)ip_type);
    return LTE_RESULT_ERROR;
  } else if (SETAPN_AUTHTYPE_MAX < auth_type) {
    DBGIF_LOG1_ERROR("Invalid auth type %ld.\n", (uint32_t)auth_type);
    return LTE_RESULT_ERROR;
  } else if (user_name &&
             SETAPN_GET_MAX_STR_LEN(LTE_APN_USER_NAME_LEN) < strlen((FAR char *)user_name)) {
    DBGIF_LOG1_ERROR("Username too long %lu.\n", (uint32_t)strlen((FAR char *)user_name));
    return LTE_RESULT_ERROR;
  } else if (password &&
             SETAPN_GET_MAX_STR_LEN(LTE_APN_PASSWD_LEN) < strlen((FAR char *)password)) {
    DBGIF_LOG1_ERROR("Password too long %lu.\n", (uint32_t)strlen((FAR char *)password));
    return LTE_RESULT_ERROR;
  } else if (hostname && SETAPN_GET_MAX_STR_LEN(LTE_APN_LEN) < strlen((FAR char *)hostname)) {
    DBGIF_LOG1_ERROR("Hostname too long %lu.\n", (uint32_t)strlen((FAR char *)hostname));
    return LTE_RESULT_ERROR;
  } else if (ipv4addralloc >= ALLOC_MAX) {
    DBGIF_LOG1_ERROR("Invalid IP type %ld.\n", (uint32_t)ipv4addralloc);
    return LTE_RESULT_ERROR;
  } else if (nslpi >= NSLPI_MAX) {
    DBGIF_LOG1_ERROR("Invalid IP type %ld.\n", (uint32_t)nslpi);
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SET_APNV2, SETAPNV2_DATA_LEN,
                                         (FAR void **)&res, SETAPN_RESP_LEN)) {
    cmd->session_id = session_id;
    if (apn) {
      strncpy((FAR char *)cmd->apn, (FAR char *)apn, sizeof(cmd->apn) - 1);
    }
    cmd->ip_type = ip_type;
    cmd->auth_type = auth_type;
    if (user_name) {
      strncpy((FAR char *)cmd->user_name, (FAR char *)user_name, sizeof(cmd->user_name) - 1);
    }

    if (password) {
      strncpy((FAR char *)cmd->password, (FAR char *)password, sizeof(cmd->password) - 1);
    }

    if (hostname) {
      strncpy((FAR char *)cmd->hostname, (FAR char *)hostname, sizeof(cmd->hostname) - 1);
    }

    cmd->ipv4addralloc = (uint8_t)ipv4addralloc;
    cmd->pcscf_discovery = (uint8_t)pcscf_discovery;
    cmd->nslpi = (uint8_t)nslpi;

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, SETAPN_RESP_LEN, &resLen,
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

  /* Check API return code */
  ret = (int32_t)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
