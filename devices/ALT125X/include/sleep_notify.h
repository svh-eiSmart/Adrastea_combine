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
 * @file sleep_notify.h
 */

#ifndef _ALT125X_SLEEP_NOTIFY_H_
#define _ALT125X_SLEEP_NOTIFY_H_

/**
 * @defgroup powermanagement PM (Power Management) Driver
 * @{
 */
/**
 * @defgroup pm_constant PM Constants
 * @{
 */
#define SLEEP_NOTIFY_SUCCESS (0) /**< Sleep Notification APIs return value - Success. */
#define SLEEP_NOTIFY_ERROR (-1)  /**< Sleep Notification APIs return value - Error. */
/** @} pm_constant */

/**
 * @defgroup pm_types PM Types
 * @{
 */
/**
 * @brief Enum for Sleep Notification callback states: SUSPENDING before entering Sleep(DH) modes.
 *        RESUMING after exiting Sleep(DH) modes.
 */
typedef enum {
  SUSPENDING = 0, /**< 0: SUSPENDING before entering to sleep modes. */
  RESUMING = 1    /**< 1: RESUMING after exiting sleep modes. */
} sleep_notify_state;

/** @} pm_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup sleep_notify_funcs Sleep Notify APIs
 * @{
 */
/**
 * @brief Initialize Sleep notification.
 *      1.Initialize sleep notify structure (fields of structures).
 *      2.Create mutex semaphore for application function.
 *
 * @return SLEEP_NOTIFY_SUCCESS: initialize Sleep notification successfully.
 *         SLEEP_NOTIFY_ERROR: failed to initialize Sleep notification.
 */
int sleep_notify_init(void);

/**
 * @brief Register and add item to sleep notify list (API).
 *
 * @param [in] P_Callback (pointer): Address of function to be called back (pointer to function).
 * @param [out] item_inx (pointer): New index of the new item.
 * @param [in] ptr_ctx (pointer): Address of additional context passed to call back function.
 *
 * @return error code. 0-success; other-fail.
 * @note The user CB should defined as the below prototype:
 *       int pm_notify_callback(pm_notify_state pm_state, void* ptr_ctx)
 * The user CB should defined as the below prototype:
 * int pm_notify_callback(pm_notify_state pm_state, void* ptr_ctx)
 * Arguments:
 *   pm_state (pm_notify_state) – PM notify state suspend or resume. SUSPENDING is the state for
 * notification before entering low power/sleep mode and RESUMING is the state for notification
 * after exiting low power/sleep mode. ptr_ctx (void*) - PM notify context which was supplied in the
 * registration function.
 */
int sleep_notify_insert_callback_item(int (*P_Callback)(sleep_notify_state, void*),
                                      int32_t* item_inx, void* ptr_ctx);

/**
 * @brief Remove item from sleep notify list (API).
 *
 * @param [in] item_inx: Index of item to need to be removed from list.
 *
 * @return error code. 0-success; other-fail.
 */
int sleep_notify_remove_callback_item(int32_t item_inx);

/**
 * @brief Execute all the items callback.
 *
 * @param [in] sleep_state: Sleep notify state (suspending or resuming).
 *
 * @return SLEEP_NOTIFY_SUCCESS or SLEEP_NOTIFY_ERROR.
 */
int sleep_notify(sleep_notify_state sleep_state);

/** @} sleep_notify_funcs */

/** @} powermanagement */
#endif /* _ALT125X_SLEEP_NOTIFY_H_ */
