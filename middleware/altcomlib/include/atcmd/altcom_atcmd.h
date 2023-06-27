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
 * @file altcom_atcmd.h
 */

#ifndef __MODULES_INCLUDE_ATCMD_ALTCOM_ATCMD_H
#define __MODULES_INCLUDE_ATCMD_ALTCOM_ATCMD_H

/**
 * @defgroup atcmd ATCMD Connector APIs
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
 * @defgroup atcmdcfg ATCMD length definition
 * @{
 */

#define ATCMD_MAX_CMD_LEN 3092       /**< Maximum length of AT command.*/
#define ATCMDCFG_MAX_CMDRES_LEN 3072 /**< Maximum length of AT command response.*/
#define ATCMDCFG_MAX_URC_LEN 3072    /**< Maximum length of URC.*/

/** @} atcmdcfg */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup atcmderror ATCMD Error Code
 * @{
 */

/**
 * @brief Definition of the error code of ATCMD connector API.
 */

typedef enum {
  ATCMD_SUCCESS = 0,  /**< Operation success */
  ATCMD_FAILURE = -1, /**< Operation failure */
} ATCMDError_e;

/** @} atcmderror */

/**
 * @defgroup atcmdcallback ATCMD URC Callback
 * @{
 */

/**
 * @brief Definition of the callback function when URC arrival.
 *
 * @param [in] urcStr: Incoming URC string.
 * @param [in] userPriv: Pointer to user's private data.
 */

typedef void (*AtcmdUrcCbFunc_t)(const char *urcStr, void *userPriv);

/** @} atcmdcallback */

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
 * @defgroup atcmd_funcs ATCMD APIs
 * @{
 */

/**
 * @brief altcom_atcmdRegUnsol() Register URCs and corresponding callback. The event
 * registration behavior of this API effected by @ref altcom_init_t.is_cbreg_only_until_appready.
 *
 * @param [in] urcStr: The URC string to be registered.
 * @param [in] urcCb: A corresponding callback to URC; NULL value implies to deregister URCs.
 * @param [in] cbParam: An user parameter to the urcCb.
 *
 * @return ATCMD_SUCCESS on success;  ATCMD_FAILURE on failure.
 */

ATCMDError_e altcom_atcmdRegUnsol(const char *urcStr, AtcmdUrcCbFunc_t urcCb, void *cbParam);

/**
 * @brief altcom_atcmdSendCmd() Send AT command.
 *
 * @param [in] cmdStr: A null-terminated string contains AT command to send; Note that there is no
 * need to append "\r" or "\r\n" at the end of string.
 * @param [in] resBufSize: The size of user provided response buffer; If resBufSize less then the
 * response, -1 returned.
 * @param [out] resBuf: An user provided buffer to fill the response of AT command.
 * @return On success, the length of response includes null-terminated returned; -1 on failure.
 */

int altcom_atcmdSendCmd(const char *cmdStr, uint16_t resBufSize, char *resBuf);

/** @} atcmd_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} atcmd */

#endif /* __MODULES_LTE_INCLUDE_ATCMD_ALTCOM_ATCMD_H */
