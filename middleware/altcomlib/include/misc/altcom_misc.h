/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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
 * @file altcom_misc.h
 */

#ifndef __MODULES_INCLUDE_MISC_ALTCOM_MISC_H
#define __MODULES_INCLUDE_MISC_ALTCOM_MISC_H

/**
 * @defgroup misc Miscellaneous Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <time.h>
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup misc_constants Miscellaneous Constants
 * @{
 */

#define MISC_MAX_CFGNAME_LEN \
  (128) /**< Maximum length of configuration name, includes '\0' symbol. */
#define MISC_MAX_CFGVALUE_LEN \
  (128) /**< Maximum length of configuration value, includes '\0' symbol. */

/** @} misc_constants */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup misc_types Miscellaneous Types
 * @{
 */

/**
 * @brief Definition of the error code of miscellaneous connector API.
 */
typedef enum {
  MISC_SUCCESS, /**< Operation success. */
  MISC_FAILURE  /**< Operation failure, general error case. */
} Misc_Error_e;

/**
 * @brief Definition of the reset types.
 */

typedef enum {
  RESET_TYPE_POWERON,  /**< Power on */
  RESET_TYPE_WARMBOOT, /**< Warm boot */
  RESET_TYPE_COLDBOOT  /**< Cold boot */
} ResetType_t;

/**
 * @brief Definition of the reset cause.
 */
typedef enum {
  RESET_CAUSE_UNKNOWN,    /**< Unknown */
  RESET_CAUSE_SHUTDOWN,   /**< Shutdown button */
  RESET_CAUSE_POWER,      /**< Power button */
  RESET_CAUSE_LOWVOLTAGE, /**< Low voltage recovery */
  RESET_CAUSE_WATCHDOG,   /**< Watchdog */
  RESET_CAUSE_OVERHEAT,   /**< Over-heating */
  RESET_CAUSE_SWFAILURE,  /**< Software failure */
  RESET_CAUSE_USER,       /**< User triggered(i.e. ATZ)*/
  RESET_CAUSE_LWM2M,      /**< LWM2M triggered */
  RESET_CAUSE_FOTA        /**< Firmware upgrade triggered */
} ResetCause_t;

/**
 * @brief Definition of the source CPU of reset, relevant to ::RESET_CAUSE_WATCHDOG or
 * ::RESET_CAUSE_SWFAILURE, also see ::ResetCause_t.
 */
typedef enum {
  RESET_CPU_PMP,     /**< PMP faulure */
  RESET_CPU_MAC,     /**< MAC faulure */
  RESET_CPU_PHY,     /**< PHY faulure */
  RESET_CPU_MAP,     /**< MAP faulure */
  RESET_CPU_MCU,     /**< MCU faulure */
  RESET_CPU_UNKNOWN, /**< Unknown */
} ResetCpu_t;

/**
 * @brief Definition of the failure type, relevant to ::RESET_CAUSE_SWFAILURE, also see
 * ::ResetCause_t.
 */
typedef enum {
  FAILURE_UNKNOWN,   /**< Unknown */
  FAILURE_ASSERT,    /**< Assertion */
  FAILURE_EXCEPTION, /**< Exception */
} FailureType_t;

/**
 * @brief Definition of the strucutre of system reset information.
 */
typedef struct {
  ResetType_t type;   /**< Reset type, also see ::ResetType_t */
  ResetCause_t cause; /**< Reset cause, also see ::ResetCause_t */
  ResetCpu_t cpu;     /**< Source CPU of reset, only valid on reset caused by ::RESET_CAUSE_WATCHDOG
                        or ::RESET_CAUSE_SWFAILURE, also see ::ResetCause_t. */
  FailureType_t failure_type; /**< Failure type, only valid on reset caused by
                                ::RESET_CAUSE_SWFAILURE, also see ::ResetCause_t. */
} ResetInfo_t;

/** @} misc_types */

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
 * @defgroup misc_funcs Miscellaneous APIs
 * @{
 */

/**
 * @brief Get configuration parameter through %GETACFG command.
 *
 * @param [in] cfgName: The configuration name in "Config.Section.Option" format. It must to be a
 * NULL-Terminated string, and length should less than @ref MISC_MAX_CFGNAME_LEN.
 * @param [inout] cfgValue: The user provided buffer to contain the target configuration value.
 * Please note that the buffer size should never less than @ref MISC_MAX_CFGVALUE_LEN. It will be a
 * NULL-Terminated string.
 *
 * @return
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_GetACfg(const char *cfgName, char *cfgValue);

/**
 * @brief Set configuration parameter through %SETACFG command.
 *
 * @param [in] cfgName: The configuration name in "Config.Section.Option" format. It must to be a
 * NULL-Terminated string, and length should less than @ref MISC_MAX_CFGNAME_LEN.
 * @param [inout] cfgValue: The configuration value to be set to cfgName. Please note that the
 * string length should never larger than @ref MISC_MAX_CFGVALUE_LEN. It must to be a
 * NULL-Terminated string.
 *
 * @return
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_SetACfg(const char *cfgName, char *cfgValue);

/**
 * @brief Get the time as the number of seconds since the Epoch from MAP.
 *
 * @param [inout] mapTime: The MAP time in seconds.
 *
 * @return
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_GetTime(time_t *mapTime);

/**
 * @brief Send the SfpLogger to the MAP via ALTCOM API.
 *
 * @param [in] buffer_size: The buffer size is the length of the sfplog
 * @param [in] sfpdbp: the buffer of the SFPLOG  It must to be a different of NULL
 *
 *
 * @return
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_SendAppLog(int buffer_size, char *sfpdbp);

/**
 * @brief Get the reset information from MAP.
 *
 * @param [out] info: The reset information structure.
 *
 * @return Misc_Error_e
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */

Misc_Error_e altcom_get_resetinfo(ResetInfo_t *info);

/**
 * @brief Get the modem temperature in Celsius.
 *
 * @param [out] temperature: the temperature of modem. The range is between -128 ~ +128
 * @return Misc_Error_e
 * Error Code                | Reason/Cause
 * --------------------------| --------------------------
 * MISC_SUCCESS              | Operation succeeded
 * MISC_FAILURE              | General failure
 */
Misc_Error_e altcom_get_mdm_temp(int16_t *temperature);
/** @} misc_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} misc */

#endif /* __MODULES_INCLUDE_MISC_ALTCOM_MISC_H */
