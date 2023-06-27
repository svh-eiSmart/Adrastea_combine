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
 * @file   sleep_mngr.h
 */

#ifndef _MCU_ALT_SMNGR_H_
#define _MCU_ALT_SMNGR_H_

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "timers.h"

/**
 * @defgroup pm_constant PM Constants
 * @{
 */
#define SMNGR_RANGE 32L
/**< Maximum number of tasks can register (become smngr clients) */
/** @} pm_constant */
/****************************************************************************
 * Internal Function Prototypes
 ****************************************************************************/
/// @cond sleep_mngr_internal
/* APIs for internal use */
bool smngr_is_dev_busy(unsigned int *sync_mask, unsigned int *async_mask);

/// @endcond

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/**
 * @defgroup powermanagement PM (Power Management) Driver
 * @{
 */
/* APIs for external use */
/**
 * @defgroup sleep_mngr_funcs Sleep Manager APIs
 * @{
 */
/**
 * @brief Initialize Sleep Manager.
 */
void smngr_init(void);

/**
 * @brief Register a synchronous device/task to sleep manager.
 *
 * @param [in] dev_name: Device/task name.
 * @param [out] id: Sleep_mngr assign a id for busy set/clear operations.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_register_dev_sync(char *dev_name, unsigned int *id);

/**
 * @brief Register a asynchronous device/task to sleep manager.
 *
 * @param [in] dev_name: Device/task name.
 * @param [in] callback: User callback function.
 * @param [in] hw: Parameter to pass to user callback.
 * @param [out] id: Sleep_mngr assign a id for busy set/clear operations.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_register_dev_async(char *dev_name, int (*callback)(void *), void *hw, unsigned int *id);

/**
 * @brief Un-register a synchronous device/task from sleep manager.
 *
 * @param [in] id: The id was got from sleep_mngr thru smngr_register_dev_ API.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_unregister_dev_sync(unsigned int id);

/**
 * @brief Un-register a asynchronous device/task from sleep manager.
 *
 * @param [in] id: The id was got from sleep_mngr thru smngr_register_dev_ API.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_unregister_dev_async(unsigned int id);

/**
 * @brief The registed device/task set busy signal to sleep manager.
 *
 * @param [in] id: The id was got from sleep_mngr thru smngr_register_dev_ API.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_dev_busy_set(unsigned int id);

/**
 * @brief The registed device/task clear busy signal from sleep manager.
 *
 * @param [in] id: The id was got from sleep_mngr thru smngr_register_dev_ API.
 *
 * @return error code. 0-success; other-fail.
 */
int smngr_dev_busy_clr(unsigned int id);

/**
 * @brief Get the device ID registration name.
 *
 * @param [in] id: The id was got from sleep_mngr.
 *
 * @return pointer of registered dev name.
 */
char *smngr_get_devname(unsigned int id);

/** @} sleep_mngr_funcs */

/** @} powermanagement */

#endif /* _MCU_ALT_SMNGR_H_ */
