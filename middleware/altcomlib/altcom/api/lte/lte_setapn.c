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

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_setapn.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SETAPN_DATA_LEN (sizeof(struct apicmd_cmddat_setapn_s))
#define SETAPN_RESP_LEN (sizeof(struct apicmd_cmddat_setapnres_s))

#define SETAPN_IPTYPE_MIN LTE_APN_IPTYPE_IP
#define SETAPN_IPTYPE_MAX LTE_APN_IPTYPE_IPV4V6

#define SETAPN_AUTHTYPE_MIN LTE_APN_AUTHTYPE_NONE
#define SETAPN_AUTHTYPE_MAX LTE_APN_AUTHTYPE_CHAP

#define SETAPN_GET_MAX_STR_LEN(len) ((len)-1)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_set_apn() set access point name settings.
 *
 * @param [in] session_id: The numeric value of the session ID.
 * See @ref ltesessionid for valid range
 * @param [in] apn: Character string of Access Point Name.
 * The maximum string length is LTE_APN_LEN, end with '\0'. See @ref lteapnlen
 * @param [in] ip_type: Internet protocol type. See @ref lteapniptype
 * @param [in] auth_type: Authentication type. See @ref lteapnauthtype
 * @param [in] user_name: Character string of user name.
 * The maximum string length is LTE_APN_USER_NAME_LEN, end with '\0'.
 * See @ref lteapnusrnamelen
 * @param [in] password: Character string of password.
 * The maximum string length is LTE_APN_PASSWD_LEN, end with '\0'.
 * See @ref lteapnusrnamelen
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_apn(uint8_t session_id, int8_t *apn, lteapniptype_e ip_type,
                        lteapnauthtype_e auth_type, int8_t *user_name, int8_t *password) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cmddat_setapn_s *cmd;
  FAR struct apicmd_cmddat_setapnres_s *res;

  /* Return error if parameter is invalid */
  if (!apn || !user_name || !password) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  } else if (LTE_SESSION_ID_MIN > session_id || LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG1_ERROR("Session ID out of range %ld.\n", (uint32_t)session_id);
    return LTE_RESULT_ERROR;
  } else if (SETAPN_GET_MAX_STR_LEN(LTE_APN_LEN) < strlen((FAR char *)apn)) {
    DBGIF_LOG1_ERROR("APN name too long %lu.\n", (uint32_t)strlen((FAR char *)apn));
    return LTE_RESULT_ERROR;
  } else if (SETAPN_IPTYPE_MAX < ip_type) {
    DBGIF_LOG1_ERROR("Invalid IP type %ld.\n", (uint32_t)ip_type);
    return LTE_RESULT_ERROR;
  } else if (SETAPN_AUTHTYPE_MAX < auth_type) {
    DBGIF_LOG1_ERROR("Invalid auth type %ld.\n", (uint32_t)auth_type);
    return LTE_RESULT_ERROR;
  } else if (SETAPN_GET_MAX_STR_LEN(LTE_APN_USER_NAME_LEN) < strlen((FAR char *)user_name)) {
    DBGIF_LOG1_ERROR("Username too long %lu.\n", (uint32_t)strlen((FAR char *)user_name));
    return LTE_RESULT_ERROR;
  } else if (SETAPN_GET_MAX_STR_LEN(LTE_APN_PASSWD_LEN) < strlen((FAR char *)password)) {
    DBGIF_LOG1_ERROR("Password too long %lu.\n", (uint32_t)strlen((FAR char *)password));
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SET_APN, SETAPN_DATA_LEN,
                                         (FAR void **)&res, SETAPN_RESP_LEN)) {
    cmd->session_id = session_id;
    strncpy((FAR char *)cmd->apn, (FAR char *)apn, sizeof(cmd->apn) - 1);
    cmd->ip_type = ip_type;
    cmd->auth_type = auth_type;
    strncpy((FAR char *)cmd->user_name, (FAR char *)user_name, sizeof(cmd->user_name) - 1);
    strncpy((FAR char *)cmd->password, (FAR char *)password, sizeof(cmd->password) - 1);
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, SETAPN_RESP_LEN, &resLen,
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

  /* Check API return code */
  ret = (int32_t)res->result;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}
