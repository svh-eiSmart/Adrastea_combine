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
#include <stdbool.h>

#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_certmgmt.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"
#include "apicmd_CfgCredProfile.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CFGCREDPROF_DATA_LEN (sizeof(struct apicmd_cfgcredprofile_s))
#define CFGCREDPROF_RESP_LEN (sizeof(struct apicmd_cfgcredprofileres_s))

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
 * @brief Configure the credential profile setting,
 * Note:
 * When profile adding, the usage of following two combinations should be mutually exclusive
 * 1.Certificate-based profile: caName, caPath, certName and keyName
 * 2.PSK-based profile: pskIdName and pskName
 *
 * @param [in] oper: Operation of the configuring.
 * @param [in] profileId: The target profile ID to be configured, the range is 1~10.
 * @param [inout] caName: Name of root certificate. The output is valid only if PROFCFG_GET == oper
 * && CERTMGMT_SUCCESS returned. For output purpose, the buffer length should not less than @ref
 * CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] caPath: Specify the trusted folder to search the root certificate. See @ref
 * TrustedCaPath_e. The output is valid only if PROFCFG_GET == oper && CERTMGMT_SUCCESS returned.
 * @param [inout] certName: Name of device certificate. It must to be a NULL-Terminated string, and
 * length should less than @ref CERTMGMT_MAX_FILENAME_LEN. The output is valid only if PROFCFG_GET
 * == oper && CERTMGMT_SUCCESS returned. For output purpose, the buffer length should not less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] keyName: Name of device private key. It must to be a NULL-Terminated string, and
 * length should less than @ref CERTMGMT_MAX_FILENAME_LEN. The output is valid only if PROFCFG_GET
 * == oper && CERTMGMT_SUCCESS returned. For output purpose, the buffer length should not less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] pskIdName: Name of PSK ID file. It must to be a NULL-Terminated string, and
 * length should less than @ref CERTMGMT_MAX_FILENAME_LEN. The output is valid only if PROFCFG_GET
 * == oper && CERTMGMT_SUCCESS returned. For output purpose, the buffer length should not less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] pskName: Name of PSK file. It must to be a NULL-Terminated string, and
 * length should less than @ref CERTMGMT_MAX_FILENAME_LEN. The output is valid only if PROFCFG_GET
 * == oper && CERTMGMT_SUCCESS returned. For output purpose, the buffer length should not less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 *
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_ConfigCredProfile(ProfileCfgOp_e oper, uint8_t profileId, char *caName,
                                          TrustedCaPath_e *caPath, char *certName, char *keyName,
                                          char *pskIdName, char *pskName) {
  uint16_t caNameLen = 0, certNameLen = 0, keyNameLen = 0, pskIdNameLen = 0, pskNameLen = 0;

  /* Return error if parameter is invalid */
  switch (oper) {
    case PROFCFG_GET:
      if (!caName || !caPath || !certName || !keyName || !pskIdName || !pskName) {
        DBGIF_LOG1_ERROR("[oper:%d]Input argument is NULL.\n", oper);
        return CERTMGMT_FAILURE;
      }

      break;

    case PROFCFG_ADD:
      if (!caPath) {
        DBGIF_LOG1_ERROR("[oper:%d]Input argument is NULL.\n", oper);
        return CERTMGMT_FAILURE;
      }

      caNameLen = NULL == caName ? 0 : strlen(caName);
      if (CERTMGMT_MAX_FILENAME_LEN < caNameLen) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid caName length %hu\n", oper, caNameLen);
        return CERTMGMT_FAILURE;
      }

      if (CAPATH_UNDEF < *caPath) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid caPath type %lu\n", oper, (uint32_t)*caPath);
        return CERTMGMT_FAILURE;
      }

      certNameLen = NULL == certName ? 0 : strlen(certName);
      if (CERTMGMT_MAX_FILENAME_LEN < certNameLen) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid certName length %hu\n", oper, certNameLen);
        return CERTMGMT_FAILURE;
      }

      keyNameLen = NULL == keyName ? 0 : strlen(keyName);
      if (CERTMGMT_MAX_FILENAME_LEN < keyNameLen) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid keyName length %hu\n", oper, keyNameLen);
        return CERTMGMT_FAILURE;
      }

      pskIdNameLen = NULL == pskIdName ? 0 : strlen(pskIdName);
      if (CERTMGMT_MAX_FILENAME_LEN < pskIdNameLen) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid pskIdName length %hu\n", oper, pskIdNameLen);
        return CERTMGMT_FAILURE;
      }

      pskNameLen = NULL == pskName ? 0 : strlen(pskName);
      if (CERTMGMT_MAX_FILENAME_LEN < pskNameLen) {
        DBGIF_LOG2_ERROR("[oper:%d]Invalid pskName length %hu\n", oper, pskNameLen);
        return CERTMGMT_FAILURE;
      }
#if 1
      /* Check usage of execlusion */
      if (((caNameLen > 0 || certNameLen > 0 || keyNameLen > 0) &&
           ((pskIdNameLen != 0) || (pskNameLen != 0))) ||
          ((pskIdNameLen > 0 || pskNameLen > 0) &&
           ((caNameLen != 0) || (certNameLen != 0) || (keyNameLen != 0)))) {
        DBGIF_LOG3_ERROR("caNameLen %hu, certNameLen %hu, keyNameLen %hu\n", caNameLen, certNameLen,
                         keyNameLen);
        DBGIF_LOG2_ERROR("pskIdNameLen %hu, pskNameLen %hu\n", pskIdNameLen, pskNameLen);
        DBGIF_LOG_ERROR("Certificate-Based or PSK-Based usage should be mutually exclusive\n");

        return CERTMGMT_FAILURE;
      }
#endif
      break;

    case PROFCFG_DELETE:
      break;

    default:
      DBGIF_LOG1_ERROR("Invalid operation %lu\n", (uint32_t)oper);
      return CERTMGMT_FAILURE;
  }

  if (CERTMGMT_MIN_PROFILE_ID > profileId || CERTMGMT_MAX_PROFILE_ID < profileId) {
    DBGIF_LOG1_ERROR("Invalid profileId %lu\n", (uint32_t)profileId);
    return CERTMGMT_FAILURE;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return CERTMGMT_FAILURE;
  }

  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_cfgcredprofile_s *cmd = NULL;
  FAR struct apicmd_cfgcredprofileres_s *res = NULL;
  CertMgmt_Error_e result = CERTMGMT_SUCCESS;

  altcom_generic_free_cmdandresbuff(cmd, res);
  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_CONFIG_CREDPROFILE,
                                         CFGCREDPROF_DATA_LEN, (FAR void **)&res,
                                         CFGCREDPROF_RESP_LEN)) {
    cmd->oper = (uint8_t)oper;
    cmd->profileId = (uint8_t)profileId;
    if (PROFCFG_ADD == oper) {
      cmd->caPath = (uint8_t)*caPath;
      cmd->caName[caNameLen] = 0;
      if (caNameLen) {
        memcpy(cmd->caName, caName, caNameLen);
      }

      cmd->certName[certNameLen] = 0;
      if (certNameLen) {
        memcpy(cmd->certName, certName, certNameLen);
      }

      cmd->keyName[keyNameLen] = 0;
      if (keyNameLen) {
        memcpy(cmd->keyName, keyName, keyNameLen);
      }

      cmd->pskIdName[pskIdNameLen] = 0;
      if (pskIdNameLen) {
        memcpy(cmd->pskIdName, pskIdName, pskIdNameLen);
      }

      cmd->pskName[pskNameLen] = 0;
      if (pskNameLen) {
        memcpy(cmd->pskName, pskName, pskNameLen);
      }
    }

    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, CFGCREDPROF_RESP_LEN, &resLen,
                        ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return CERTMGMT_FAILURE;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    result = CERTMGMT_FAILURE;
    goto errout_with_cmdfree;
  }

  if (CFGCREDPROF_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    result = CERTMGMT_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  result = (CertMgmt_Error_e)res->result;
  if (CERTMGMT_SUCCESS == result) {
    if (PROFCFG_GET == oper) {
      *caPath = (TrustedCaPath_e)res->caPath;
      memcpy(caName, res->caName, CERTMGMT_MAX_FILENAME_LEN);
      caName[CERTMGMT_MAX_FILENAME_LEN] = 0;
      memcpy(certName, res->certName, CERTMGMT_MAX_FILENAME_LEN);
      certName[CERTMGMT_MAX_FILENAME_LEN] = 0;
      memcpy(keyName, res->keyName, CERTMGMT_MAX_FILENAME_LEN);
      keyName[CERTMGMT_MAX_FILENAME_LEN] = 0;
      memcpy(pskIdName, res->pskIdName, CERTMGMT_MAX_FILENAME_LEN);
      pskIdName[CERTMGMT_MAX_FILENAME_LEN] = 0;
      memcpy(pskName, res->pskName, CERTMGMT_MAX_FILENAME_LEN);
      pskName[CERTMGMT_MAX_FILENAME_LEN] = 0;
    }
  } else {
    DBGIF_LOG1_ERROR("Profile configuration failed: %lu\n", (uint32_t)result);
    goto errout_with_cmdfree;
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return result;
}
