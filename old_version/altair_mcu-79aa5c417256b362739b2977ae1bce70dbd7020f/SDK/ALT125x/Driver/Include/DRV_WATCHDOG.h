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
/**
 * @file DRV_WATCHDOG.h
 */
#ifndef DRV_WATCHDOG_H_
#define DRV_WATCHDOG_H_
#include <stdint.h>
/**
 * @defgroup wdt_driver Watchdog Driver
 * @{
 */
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup wdt_static_config Watchdog Constants
 * @{
 */
#define CONFIG_WATCHDOG_DEFAULT_TIMEOUT 30 /**< WDT Default timeout value */
#ifndef CONFIG_WATCHDOG_SUPPORT_PM
#define CONFIG_WATCHDOG_SUPPORT_PM configUSE_ALT_SLEEP /**< Support pm sleep behavior */
#endif
/** @} wdt_static_config */

/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup wdt_data_types Watchdog Types
 * @{
 */
/**
 * @typedef DRV_WATCHDOG_Status
 * Definition of watchdog API return code.
 */
typedef enum {
  DRV_WATCHDOG_OK = 0x00U,  /**< Watchdog API returns with no error */
  DRV_WATCHDOG_ERROR_STATE, /**< Watchdog API returns with worng state error */
  DRV_WATCHDOG_ERROR_PARAM, /**< Watchdog API returns with parameter error */
  DRV_WATCHDOG_ERROR_PM     /**< Watchdog API returns with pm operation error */
} DRV_WATCHDOG_Status;

/**
 * @typedef DRV_WATCHDOG_Time
 * Definition of watchdog timeout type
 */
typedef uint32_t DRV_WATCHDOG_Time;

/**
 * @typedef DRV_WATCHDOG_TimeoutCallback
 * Definition of callback function for wdt timeout.
 * @param[in] user_param : Parameter for this callback.
 */
typedef void (*DRV_WATCHDOG_TimeoutCallback)(void *user_param);

/**
 * @struct DRV_WATCHDOG_Config
 * Definition of watchdog configurations used for initialization
 */
typedef struct {
  DRV_WATCHDOG_Time timeout;               /**< Watchdog timeout in seconds */
  DRV_WATCHDOG_TimeoutCallback timeout_cb; /**< Watchdog timeout callback for notification */
  void *timeout_cb_param;                  /**< User parameter for timeout callback */
} DRV_WATCHDOG_Config;
/** @} wdt_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */
/**
 * @defgroup wdt_apis Watchdog APIs
 * @{
 */

/**
 * @brief Initialize watchdog timer to be ready to start.
 *
 * @param [in] config: Watchdog initialzation parameter with type @ref DRV_WATCHDOG_Config
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_Initialize(DRV_WATCHDOG_Config *config);

/**
 * @brief Uninitialize watchdog timer.
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_Uninitialize(void);

/**
 * @brief Enable watchdog timer
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_Start(void);
/**
 * @brief Disable watchdog timer
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_Stop(void);
/**
 * @brief API to kick the watchdog.
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_Kick(void);
/**
 * @brief Reconfigure watchdog timer with new timeout after watchdog was already started
 *
 * @param [in] timeout: Specify watchdog timeout in seconds
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_SetTimeout(DRV_WATCHDOG_Time timeout);
/**
 * @brief Get the current configured watchdog timer in seconds
 *
 * @param [out] timeout: Configured watchdog timeout in seconds
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_GetTimeout(DRV_WATCHDOG_Time *timeout);
/**
 * @brief Get the remaining time in seconds before watchdog timer expires.
 *
 * @param [out] timeleft: Remaining time before watchdog timer expires.
 *
 * @return @ref DRV_WATCHDOG_Status
 */
DRV_WATCHDOG_Status DRV_WATCHDOG_GetRemainingTime(DRV_WATCHDOG_Time *timeleft);
/** @} wdt_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} wdt_driver */
#endif /*DRV_WATCHDOG_H_*/
