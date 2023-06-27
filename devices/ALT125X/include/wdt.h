/*  ---------------------------------------------------------------------------

        (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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
 * @file wdt.h
 */
#ifndef _WDT_ALT125X_H_
#define _WDT_ALT125X_H_

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
#define CONFIG_ALT1250_WDT_DEFAULT_TIMEOUT 30 /**< WDT Default timeout value */
/** @} wdt_static_config */

/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup wdt_data_types Watchdog Types
 * @{
 */
/**
 * @typedef wdt_status_e
 * Definition of watchdog API return code.
 */
typedef enum {
  WDT_SUCCESS = 0, /**< WDT API returns with no error */
  WDT_FAILED       /**< WDT API returns with error */
} wdt_status_e;
/**
 * @typedef wdt_timeout_t
 * Definition of watchdog timeout type
 */
typedef unsigned int wdt_timeout_t;
/**
 * @typedef wdt_timeout_cb
 * Definition of callback function for wdt timeout.
 * @param[in] user_param : Parameter for this callback.
 */
typedef void (*wdt_timeout_cb)(void *user_param);
/** @} wdt_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @defgroup wdt_apis Watchdog APIs
 * @{
 */
/**
 * @brief Enable watchdog timer with the timeout value specified with timeout parameter in seconds.
 *
 * @param [in] timeout: Specify watchdog timeout in seconds.
 */
void watchdog_start(wdt_timeout_t timeout);
/**
 * @brief Disable watchdog timer
 *
 * @return @ref wdt_status_e.
 */
wdt_status_e watchdog_stop(void);
/**
 * @brief Register callback function to receive watchdog first expiration notification
 *
 * @param [in] cb: User callback with type @ref wdt_timeout_cb to receive wdt timeout notification.
 * @param [in] user_param: Parameter to pass to user callback
 */
void watchdog_register_timeout_callback(wdt_timeout_cb cb, void *user_param);
/**
 * @brief API to kick the watchdog.
 *
 * @return @ref wdt_status_e
 */
wdt_status_e watchdog_kick(void);
/**
 * @brief Reconfigure watchdog timer with new timeout after watchdog was already enabled
 *
 * @param [in] new_timeout: Specify watchdog timeout in seconds
 * @return @ref wdt_status_e.
 */
void watchdog_set_timeout(wdt_timeout_t new_timeout);
/**
 * @brief Get the maximum configured watchdog timer in seconds
 *
 * @return Maximum timeout configured currently.
 */
wdt_timeout_t watchdog_get_max_timeout(void);
/**
 * @brief Get the time left in seconds before watchdog timer expires.
 *
 * @return Time left before the dog barks.
 */
wdt_timeout_t watchdog_get_time_left(void);
/** @} wdt_apis */
/** @} wdt_driver */

/*Internal APIs*/
/*! @cond Doxygen_Suppress */
void watchdog_sleep_pre_process(void);
void watchdog_sleep_post_process(void);
/*! @endcond */
#endif
