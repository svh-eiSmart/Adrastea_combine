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

#ifndef HIFC_OSAL_H_
#define HIFC_OSAL_H_

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <event_groups.h>
#include <timers.h>
#include "hifc_osal_opt.h"

#ifndef bool
typedef uint32_t bool;
#define true (1)
#define false (0)
#endif

/**
 * @defgroup OSAL OSAL API
 * @{
 */

// clang-format off
/*! @brief Prototype of task handle. */
typedef TaskHandle_t        hifc_sys_task_t;
/*! @brief Prototype of semaphore handle. */
typedef SemaphoreHandle_t   hifc_sys_sem_t;
/*! @brief Prototype of queue handle. */
typedef QueueHandle_t       hifc_sys_mq_t;
/*! @brief Prototype of mutex handle. */
typedef SemaphoreHandle_t   hifc_sys_mutex_t;
/*! @brief Prototype of event flag handle. */
typedef EventGroupHandle_t  hifc_sys_evflag_t;
/*! @brief Prototype of event bit for event flag. */;
typedef EventBits_t         hifc_sys_evflag_ptn_t;
/*! @brief Prototype of OS basetype. */
typedef BaseType_t          hifc_sys_basetype_t;
/*! @brief Prototype of timer handle. */
typedef TimerHandle_t       hifc_sys_timer_t;
/*! @brief Prototype of pended function handle. */
typedef PendedFunction_t    hifc_sys_pendFun_t;
// clang-format on

/**
 * @defgroup callback Callbacks
 * @{
 */
/*! @brief Prototype of timer callback. */
typedef void (*hifc_sys_timer_cb_t)(hifc_sys_timer_t timer);
/** @} callback */

/**
 * @defgroup OSAL_API OSAL APIs
 * APIs for OSAL
 * @{
 */

/**
 * Create a timer object
 *
 * @param [out] timer The handle of the timer
 * @param [in] period_ms expire time in ms
 * @param [in] autoreload  Whether or not ro start the timer repeatedly
 * @param [in] callback The function called when the timer expired.
 * @return If the timer is created successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_create_timer(hifc_sys_timer_t *timer, int32_t period_ms, bool autoreload,
                               hifc_sys_timer_cb_t callback);

/**
 * Start a timer
 *
 * @param [in] timer The handle of the timer
 * @return If the timer starts successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_start_timer(hifc_sys_timer_t *timer);

/**
 * Stop a timer
 *
 * @param [in] timer The handle of the timer
 * @return If the timer stops successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_stop_timer(hifc_sys_timer_t *timer);
/**
 * Stop a timer from ISR
 *
 * @param [in] timer The handle of the timer
 * @return If the timer stops successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_stop_timer_fromISR(hifc_sys_timer_t *timer);

/**
 * Reset a timer. This should be invoked from ISR context
 *
 * @param [in] timer The handle of the timer
 * @return If the timer restarts successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_reset_timer_fromISR(hifc_sys_timer_t *timer);

/**
 * Changes the time of inactivity timer. This should be invoked from ISR context
 *
 * @param [in] timer The handle of the timer
 * @param [in] period_in_ms The desired time to be changed
 * @return If the timer restarts successfully then 0 is returned.
 *   Otherwise negative value is returned.
 */
int32_t hifc_osal_change_timer_period_fromISR(hifc_sys_timer_t *timer, uint32_t period_in_ms);

/**
 * @brief To get remaining time of timer
 *
 * @param [in] timer The timer to be queried
 * @return uint32_t return the remaining time
 */
uint32_t hifc_osal_get_timer_expire_time_fromISR(hifc_sys_timer_t *timer);

/**
 * @brief To enter critical section
 *
 */
void hifc_osal_enter_critical(void);

/**
 * @brief To exit critical section
 *
 */
void hifc_osal_exit_critical(void);
/** @} OSAL_API */
/** @} OSAL */
#endif /* OSAL_OPENRTOS_OSAL_H_ */
