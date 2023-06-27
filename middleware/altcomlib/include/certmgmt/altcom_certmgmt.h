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

/**
 * @file altcom_certmgmt.h
 */

#ifndef __MODULES_INCLUDE_CERTMGMT_ALTCOM_CERTMGMT_H
#define __MODULES_INCLUDE_CERTMGMT_ALTCOM_CERTMGMT_H

/**
 * @defgroup certmgmt Certificate Management Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup certmgmtcfg Certificate Management Configuration Constants
 * @{
 */

#define CERTMGMT_MAX_FILENAME_LEN (29) /**< Maximum length of filename, includes '\0' symbol. */
#define CERTMGMT_MIN_PROFILE_ID (1)    /**< Minimum Profile ID Value. */
#define CERTMGMT_MAX_PROFILE_ID (10)   /**< Maximum Profile ID Value. */
#define CERTMGMT_MAX_PSKID_LEN (64)    /**< Maximum length of PSK ID. */
#define CERTMGMT_MIN_PSK_LEN (4)       /**< Minimum length of PSK key. */
#define CERTMGMT_MAX_PSK_LEN (32)      /**< Maximum lenth of PSK key. */

/** @} certmgmtcfg */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup certmgmterror Certificate Management Error Code
 * @{
 */

/**
 * @brief Definition of the error code of certificate management connector API.
 */

typedef enum {
  CERTMGMT_SUCCESS = 0,             /**< Operation success. */
  CERTMGMT_FAILURE = 1,             /**< Operation failure, general error case. */
  CERTMGMT_INSUFFICIENT_BUFFER = 2, /**< Buffer size insufficient. */
} CertMgmt_Error_e;

/** @} certmgmterror */

/**
 * @defgroup credtype Credential Type
 * @{
 */

/**
 * @brief Enumeration of credential type used by altcom_WriteCredential() and
 * altcom_GetCredentialList()
 *
 */

typedef enum {
  CREDTYPE_CERT = 0, /**< Certificate type, the original content will be kept on write. */
  CREDTYPE_PRIVKEY,  /**< Private Key, the original content will be encrypted on write. */
  CREDTYPE_PSKID,    /**< PSK ID, the original content will be kept on write. */
  CREDTYPE_PSK,      /**< PSK, the original content will be encrypted on write. */
  CREDTYPE_MAX       /**< PSK, the original content will be encrypted on write. */
} CredentialType_e;

/** @} credtype */

/**
 * @defgroup profilecfgop Profile Configuration Operation
 * @{
 */

/**
 * @brief Enumeration of credential type used by altcom_ConfigureProfile()
 */

typedef enum {
  PROFCFG_GET = 0,   /**< To get a specified credential profile */
  PROFCFG_ADD = 1,   /**< To add a new credential profile */
  PROFCFG_DELETE = 2 /**< To delete an exist credential profile */
} ProfileCfgOp_e;

/** @} profilecfgop */

/**
 * @defgroup capath Trusted CA Path
 * @{
 */

/**
 * @brief Enumeration of Trusted CA path used by altcom_GetCredentialList() and
 * altcom_ConfigureProfile()
 */

typedef enum {
  CAPATH_ROOT = 0, /**< Root Trusted Folder */
  CAPATH_USER = 1, /**< User Trusted Folder */
  CAPATH_UNDEF = 2 /**< Undefined Path */
} TrustedCaPath_e;

/** @} capath */

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/**
 * @defgroup certmgmt_funcs Certificate Management APIs
 * @{
 */

/**
 * @brief Read credential file from MAP storage.
 *
 * @param [in] credName: Name of target certificate. It must be a NULL-Terminated string, and length
 * should less than
 * @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [inout] credBuf: Caller provided buffer to store the content of target certificate.
 * @param [in] credBufLen: Buffer length of certBuf.
 * @param [inout] credLen: Real length of certificate, this parameter could be used to check if the
 * size of provided buffer sufficient or not. Only valid on @ref CERTMGMT_SUCCESS / @ref
 * CERTMGMT_INSUFFICIENT_BUFFER returned.
 *
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_INSUFFICIENT_BUFFER  | Insufficient buffer size
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_ReadCredential(const char *credName, uint8_t *credBuf, uint16_t credBufLen,
                                       uint16_t *credLen);

/**
 * @brief Write credential data(certificate/private key) to MAP storage.
 *
 * @param [in] credType: Type of credential. See @ref CredentialType_e.
 * @param [in] credName: Name of the target credential to write. It must to be a NULL-Terminated
 * string, and length should less than @ref CERTMGMT_MAX_FILENAME_LEN.
 * @param [in] credData: Data buffer of credential to write. It must to be a NULL-Terminated string.
 * @param [in] credLen: Data length of credData.
 *
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_WriteCredential(CredentialType_e credType, const char *credName,
                                        uint8_t *credData, uint16_t credLen);

/**
 * @brief Delete the specified credential file from MAP storage.
 *
 * @param [in] credName: Name of the target credential to delete.
 *
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_DeleteCredential(const char *credName);

/**
 * @brief Get the credential list in trusted folder stored on MAP.
 *
 * @param [in] caPath: Specify which trusted folder to list. See @ref CredentialType_e.
 * @param [inout] credList: Caller provided buffer to store the desired list of credential. The
 * output is a NULL-Terminated string.
 * @param [in] listBufLen: Buffer length of credList.
 * @param [inout] listLen: Real length of credential list, this parameter could be used to check if
 * the size of provided buffer sufficient or not. Only valid on @ref CERTMGMT_SUCCESS / @ref
 * CERTMGMT_INSUFFICIENT_BUFFER returned.
 *
 * @return
 * Error Code                    | Reason/Cause
 * ----------------------------- | -------------------------------------
 * CERTMGMT_SUCCESS              | Operation succeeded
 * CERTMGMT_INSUFFICIENT_BUFFER  | Insufficient buffer size
 * CERTMGMT_FAILURE              | General failure
 */

CertMgmt_Error_e altcom_GetCredentialList(TrustedCaPath_e caPath, uint8_t *credList,
                                          uint16_t listBufLen, uint16_t *listLen);

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
                                          char *pskIdName, char *pskName);

/** @} certmgmt_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} certmgmt */

#endif /* __MODULES_INCLUDE_CERTMGMT_ALTCOM_CERTMGMT_H */
