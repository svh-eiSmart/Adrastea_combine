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
 * @file alt_osal.h
 */

#ifndef MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_H
#define MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "alt_osal_opt.h"

/**
 * @defgroup alt_osal Altair OS Abstraction Layer
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup alt_osal_constants ALT OSAL Constants
 * @{
 */

#define ALT_OSAL_TIMEO_NOWAIT (0)        /**< no waiting */
#define ALT_OSAL_TIMEO_FEVR (-1)         /**< timeout forever */
#define ALT_OSAL_WMODE_TWF_ORW (0x0000)  /**< wait mode : OR */
#define ALT_OSAL_WMODE_TWF_ANDW (0x0001) /**< wait mode : AND */
#define ALT_OSAL_FLAG_NO_CLEAR (0x0002)  /**< Do not clear flag */

#define ALT_OSAL_MUTEX_RECURSIVE (1 << 0) /**<  Recursive attribute of mutex */

#define ALT_OSAL_OWN_TASK (NULL) /**< Speficy the task itself */

#define ALT_OSAL_MIN_TASK_CBSIZE \
  (MIN_STATIC_TASK_CBSIZE) /**< Minimum size of static task control block */
#define ALT_OSAL_MIN_SEMAPHORE_CBSIZE \
  (MIN_STATIC_SEM_CBSIZE) /**< Minimum size of static semaphore control block */
#define ALT_OSAL_MIN_MUTEX_CBSIZE \
  (MIN_STATIC_MUTEX_CBSIZE) /**< Minimum size of static mutex control block */
#define ALT_OSAL_MIN_QUEUE_CBSIZE \
  (MIN_STATIC_QUEUE_CBSIZE) /**< Minimum size of static queue control block */
#define ALT_OSAL_MIN_EVENTFLAG_CBSIZE \
  (MIN_STATIC_EVTFLAG_CBSIZE) /**< Minimum size of static event flag control block */
#define ALT_OSAL_MIN_TIMER_CBSIZE \
  (MIN_STATIC_TIMER_CBSIZE) /**< Minimum size of static timer control block */

/**
 * @brief Check assertion condition.
 */

#define alt_osal_assert(x) \
  do {                     \
    configASSERT(x);       \
  } while (0)

/** @} alt_osal_constants */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup alt_osal_types ALT OSAL Types
 * @{
 */

/**
 * @brief Enumeration of task priority.
 */

typedef enum {
  ALT_OSAL_TASK_PRIO_NONE = 0,               /**< No priority (not initialized) */
  ALT_OSAL_TASK_PRIO_IDLE = 1,               /**< Reserved for idle task */
  ALT_OSAL_TASK_PRIO_LOW = 8,                /**< Priority: low */
  ALT_OSAL_TASK_PRIO_LOW1 = 8 + 1,           /**< Priority: low + 1 */
  ALT_OSAL_TASK_PRIO_LOW2 = 8 + 2,           /**< Priority: low + 2 */
  ALT_OSAL_TASK_PRIO_LOW3 = 8 + 3,           /**< Priority: low + 3 */
  ALT_OSAL_TASK_PRIO_LOW4 = 8 + 4,           /**< Priority: low + 4 */
  ALT_OSAL_TASK_PRIO_LOW5 = 8 + 5,           /**< Priority: low + 5 */
  ALT_OSAL_TASK_PRIO_LOW6 = 8 + 6,           /**< Priority: low + 6 */
  ALT_OSAL_TASK_PRIO_LOW7 = 8 + 7,           /**< Priority: low + 7 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL = 16,      /**< Priority: below normal */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL1 = 16 + 1, /**< Priority: below normal + 1 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL2 = 16 + 2, /**< Priority: below normal + 2 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL3 = 16 + 3, /**< Priority: below normal + 3 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL4 = 16 + 4, /**< Priority: below normal + 4 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL5 = 16 + 5, /**< Priority: below normal + 5 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL6 = 16 + 6, /**< Priority: below normal + 6 */
  ALT_OSAL_TASK_PRIO_BELOW_NORMAL7 = 16 + 7, /**< Priority: below normal + 7 */
  ALT_OSAL_TASK_PRIO_NORMAL = 24,            /**< Priority: normal */
  ALT_OSAL_TASK_PRIO_NORMAL1 = 24 + 1,       /**< Priority: normal + 1 */
  ALT_OSAL_TASK_PRIO_NORMAL2 = 24 + 2,       /**< Priority: normal + 2 */
  ALT_OSAL_TASK_PRIO_NORMAL3 = 24 + 3,       /**< Priority: normal + 3 */
  ALT_OSAL_TASK_PRIO_NORMAL4 = 24 + 4,       /**< Priority: normal + 4 */
  ALT_OSAL_TASK_PRIO_NORMAL5 = 24 + 5,       /**< Priority: normal + 5 */
  ALT_OSAL_TASK_PRIO_NORMAL6 = 24 + 6,       /**< Priority: normal + 6 */
  ALT_OSAL_TASK_PRIO_NORMAL7 = 24 + 7,       /**< Priority: normal + 7 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL = 32,      /**< Priority: above normal */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL1 = 32 + 1, /**< Priority: above normal + 1 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL2 = 32 + 2, /**< Priority: above normal + 2 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL3 = 32 + 3, /**< Priority: above normal + 3 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL4 = 32 + 4, /**< Priority: above normal + 4 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL5 = 32 + 5, /**< Priority: above normal + 5 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL6 = 32 + 6, /**< Priority: above normal + 6 */
  ALT_OSAL_TASK_PRIO_ABOVE_NORMAL7 = 32 + 7, /**< Priority: above normal + 7 */
  ALT_OSAL_TASK_PRIO_HIGH = 40,              /**< Priority: high */
  ALT_OSAL_TASK_PRIO_HIGH1 = 40 + 1,         /**< Priority: high + 1 */
  ALT_OSAL_TASK_PRIO_HIGH2 = 40 + 2,         /**< Priority: high + 2 */
  ALT_OSAL_TASK_PRIO_HIGH3 = 40 + 3,         /**< Priority: high + 3 */
  ALT_OSAL_TASK_PRIO_HIGH4 = 40 + 4,         /**< Priority: high + 4 */
  ALT_OSAL_TASK_PRIO_HIGH5 = 40 + 5,         /**< Priority: high + 5 */
  ALT_OSAL_TASK_PRIO_HIGH6 = 40 + 6,         /**< Priority: high + 6 */
  ALT_OSAL_TASK_PRIO_HIGH7 = 40 + 7,         /**< Priority: high + 7 */
  ALT_OSAL_TASK_PRIO_REALTIME = 48,          /**< Priority: realtime */
  ALT_OSAL_TASK_PRIO_REALTIME1 = 48 + 1,     /**< Priority: realtime + 1 */
  ALT_OSAL_TASK_PRIO_REALTIME2 = 48 + 2,     /**< Priority: realtime + 2 */
  ALT_OSAL_TASK_PRIO_REALTIME3 = 48 + 3,     /**< Priority: realtime + 3 */
  ALT_OSAL_TASK_PRIO_REALTIME4 = 48 + 4,     /**< Priority: realtime + 4 */
  ALT_OSAL_TASK_PRIO_REALTIME5 = 48 + 5,     /**< Priority: realtime + 5 */
  ALT_OSAL_TASK_PRIO_REALTIME6 = 48 + 6,     /**< Priority: realtime + 6 */
  ALT_OSAL_TASK_PRIO_REALTIME7 = 48 + 7,     /**< Priority: realtime + 7 */
  ALT_OSAL_TASK_PRIO_ISR = 56,               /**< Reserved for ISR deferred thread */
  ALT_OSAL_TASK_PRIO_ERROR = -1,             /**< Reserved for ISR deferred thread */
  ALT_OSAL_TASK_PRIO_RESERVED = 0x7FFFFFFF,  /**< Prevents enum down-size compiler optimization */
} alt_osal_task_priority;

/**
 * @brief Definition of task attribute.
 */

typedef struct {
  const char *name;                     /**< name of the task */
  uint32_t attr_bits;                   /**< attribute bits */
  void *cb_mem;                         /**< memory for control block */
  uint32_t cb_size;                     /**< size of provided memory for control block */
  void *stack_mem;                      /**< memory for stack */
  uint32_t stack_size;                  /**< size of stack */
  alt_osal_task_priority priority;      /**< initial task priority */
  FAR void *arg;                        /**< argument of task priority */
  CODE void (*function)(FAR void *arg); /**< task function */
} alt_osal_task_attribute;

/**
 * @brief Definition of semaphore attribute.
 */

typedef struct {
  const char *name;       /**< name of the semaphore */
  uint32_t attr_bits;     /**< attribute bits */
  void *cb_mem;           /**< memory for control block */
  uint32_t cb_size;       /**< size of provided memory for control block */
  uint32_t initial_count; /**< initial count of the semaphore */
  uint32_t max_count;     /**< max count of the semaphore */
} alt_osal_semaphore_attribute;

/**
 * @brief Definition of mutex attribute.
 */

typedef struct {
  const char *name;   /**< name of the semaphore */
  uint32_t attr_bits; /**< attribute bits */
  void *cb_mem;       /**< memory for control block */
  uint32_t cb_size;   /**< size of provided memory for control block */
} alt_osal_mutex_attribute;

/**
 * @brief Definition of queue attribute.
 */

typedef struct {
  const char *name;     /**< name of the queue */
  uint32_t attr_bits;   /**< attribute bits */
  void *cb_mem;         /**< memory for control block */
  uint32_t cb_size;     /**< size of provided memory for control block */
  void *mq_mem;         /**< memory for data storage */
  uint32_t mq_size;     /**< size of provided memory for data storage */
  uint32_t numof_queue; /**< number of elements in queue */
  uint32_t queue_size;  /**< size of element in queue */
} alt_osal_queue_attribute;

/**
 * @brief Definition of event flag attribute.
 */

typedef struct {
  const char *name;   /**< name of the event flag */
  uint32_t attr_bits; /**< attribute bits */
  void *cb_mem;       /**< memory for control block */
  uint32_t cb_size;   /**< size of provided memory for control block */
} alt_osal_eventflag_attribute;

/**
 * @brief Definition of timer attribute.
 */

typedef struct {
  const char *name;   /**< name of the timer */
  uint32_t attr_bits; /**< attribute bits */
  void *cb_mem;       /**< memory for control block */
  uint32_t cb_size;   /**< size of provided memory for control block */
} alt_osal_timer_attribute;

/**
 * @brief Definition of condition variable attribute.
 */

typedef void *alt_osal_thread_cond_attribute;

/**
 * @brief Definition of task handle.
 */

typedef void *alt_osal_task_handle;

/**
 * @brief Definition of semaphore handle.
 */

typedef void *alt_osal_semaphore_handle;

/**
 * @brief Definition of queue handle.
 */

typedef void *alt_osal_queue_handle;

/**
 * @brief Definition of mutex handle.
 */

typedef void *alt_osal_mutex_handle;

/**
 * @brief Definition of event flag handle.
 */

typedef void *alt_osal_eventflag_handle;

/**
 * @brief Definition of event bits.
 */

typedef uint32_t alt_osal_event_bits;

/**
 * @brief Definition of event flag mode.
 */

typedef uint32_t alt_osal_eventflag_mode;

/**
 * @brief Definition of timer handle.
 */

typedef void *alt_osal_timer_handle;

/**
 * @brief Definition of condition variable handle.
 */

typedef void *alt_osal_thread_cond_handle;

/**
 * @brief Prototype of timer callback function
 *
 * @param [in] argument: User provide argument.
 */
typedef void (*alt_osal_timer_cb_t)(void *argument);

/**
 * @brief Enumeration of task state.
 */

typedef enum {
  ALT_OSAL_TASK_RUNNING = 0, /**< A task is querying the state of itself, so must be running. */
  ALT_OSAL_TASK_READY,       /**< The task being queried is in a read or pending ready list. */
  ALT_OSAL_TASK_BLOCKED,     /**< The task being queried is in the Blocked state. */
  ALT_OSAL_TASK_SUSPENED,    /**< The task being queried is in the Suspended state, or is in the
                                Blocked state with an infinite time out. */
  ALT_OSAL_TASK_DELETED, /**< The task being queried has been deleted, but its TCB has not yet been
                            freed. */
  ALT_OSAL_TASK_INVALID  /**< Used as an 'invalid state' value. */
} alt_osal_task_state;

/**
 * @brief Definition of task status.
 */
typedef struct {
  void *handle;              /**< Task handle. */
  const char *name;          /**< A pointer to the task's name. */
  uint32_t id;               /**< A number unique to the task. */
  alt_osal_task_state state; /**< The current state of task. */
  alt_osal_task_priority
      priority; /**< The priority at which the task was running (may be inherited). */
  alt_osal_stack_type *stack_base; /**< Points to the lowest address of the task's stack area. */
  size_t stack_size;               /**< Stack size of task */
  size_t stack_usage;              /**< Current stack usage */
} alt_osal_task_status;

/** @} alt_osal_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/**
 * @defgroup alt_osal_apis ALT OSAL APIs
 * @{
 */

/**
 * @brief Get the current context
 *
 * @return If the context is IRQ, return 1. Otherwise 0 is returned.
 */
uint32_t alt_osal_irq_context(void);
/**
 * @brief Create a new task.
 *
 * @param [in] task: Used to pass a handle to the created task out of the alt_osal_create_task()
 * function.
 * @param [in] attr: A value that will passed into the created task
 * as the task's parameter.
 *
 * @return If the task was created successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_task(FAR alt_osal_task_handle *task,
                             FAR const alt_osal_task_attribute *attr);

/**
 * @brief Delete a specified task.
 *
 * @param [in] task: The handle of the task to be deleted.
 * If task set to ALT_OSAL_OWN_TASK then delete caller own task.
 *
 * @return If the task was deleted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_task(FAR alt_osal_task_handle *task);

/**
 * @brief Make self task sleep.
 *
 * @param [in] timeout_ms: The sleep time in milliseconds.
 *
 * @return If the task was slept successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_sleep_task(int32_t timeout_ms);

/**
 * @brief List running task information
 *
 * @param buf A space that store the information
 * @return int32_t return 0 on success, negtive value on failure
 */
int32_t alt_osal_list_task(char *buf);
/**
 * @brief Resume the scheduler.
 *
 * @return If the scheduler was resumed successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_enable_dispatch(void);

/**
 * @brief Suspend the scheduler.
 *
 * @return If the scheduler was suspended successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_disable_dispatch(void);

/**
 * @brief Create a new semaphore.
 *
 * @param [in] sem: Used to pass a handle to the created semaphore
 * out of the alt_osal_create_semaphore() function.
 * @param [in] attr: A value that will passed into the created semaphore
 * as the semaphore's parameter.
 *
 * @return If the semaphore was created successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_semaphore(FAR alt_osal_semaphore_handle *sem,
                                  FAR const alt_osal_semaphore_attribute *attr);

/**
 * @brief Delete a specified semaphore.
 *
 * @param [in] sem: The handle of the semaphore to be deleted.
 *
 * @return If the semaphore was deleted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_semaphore(FAR alt_osal_semaphore_handle *sem);

/**
 * @brief Waiting for a semaphore to become available.
 *
 * @param [in] sem: The handle of the semaphore to be waited.
 * @param [in] timeout_ms: The time in milliseconds to wait for the semaphore
 * to become available.
 * If timeout_ms set to @ref ALT_OSAL_TIMEO_FEVR then wait until
 * the semaphore to become available.
 *
 * @return If the semaphore was become available then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_wait_semaphore(FAR alt_osal_semaphore_handle *sem, int32_t timeout_ms);

/**
 * @brief Post a semaphore.
 *
 * @param [in] sem: sem The handle of the semaphore to be posted.
 *
 * @return If the semaphore was posted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_post_semaphore(FAR alt_osal_semaphore_handle *sem);

/**
 * @brief Create a new mutex.
 *
 * @param[in] mutex: Used to pass a handle to the created mutex
 * out of the alt_osal_create_mutex() function.
 * @param[in] attr: A value that will passed into the created mutex
 * as the mutex's parameter.
 *
 * @return If the mutex was created successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_mutex(FAR alt_osal_mutex_handle *mutex,
                              FAR const alt_osal_mutex_attribute *attr);

/**
 * @brief Delete a specified mutex.
 *
 * @param [in] mutex: The handle of the mutex to be deleted.
 *
 * @return If the mutex was deleted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_mutex(FAR alt_osal_mutex_handle *mutex);

/**
 * @brief Lock a mutex.
 *
 * @param [in] mutex: The handle of the mutex to be locked.
 * @param [in] timeout_ms: The time in milliseconds to block until timeout occur.
 * If timeout_ms set to @ref ALT_OSAL_TIMEO_FEVR then wait until the mutex locked.
 *
 * @return If the mutex was locked successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_lock_mutex(FAR alt_osal_mutex_handle *mutex, int32_t timeout_ms);

/**
 * @brief Unlock a mutex.
 *
 * @param [in] mutex: The handle of the mutex to be unlocked.
 *
 * @return If the mutex was unlocked successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_unlock_mutex(FAR alt_osal_mutex_handle *mutex);

/**
 * @brief Create a new message queue.
 *
 * @param [in] mq: Used to pass a handle to the created message queue
 * out of the alt_osal_create_mqueue() function.
 * @param [in] attr: A value that will passed into the created message queue
 * as the message queue's parameter.
 *
 * @return If the message queue was created successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_mqueue(FAR alt_osal_queue_handle *mq,
                               FAR const alt_osal_queue_attribute *attr);

/**
 * @brief Delete a specified message queue.
 *
 * @param [in] mq: The handle of the message queue to be deleted.
 *
 * @return If the message queue was deleted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_mqueue(FAR alt_osal_queue_handle *mq);

/**
 * @brief Send message by a message queue.
 *
 * @param [in] mq: The handle of the message queue to be send.
 * @param [in] msg_ptr: The message to be send.
 * @param [in] len: The length of the message.
 * @param [in] timeout_ms: The time in milliseconds to block until send timeout occurs.
 * If timeout_ms set to @ref ALT_OSAL_TIMEO_FEVR then wait until the message queue to become
 * available.
 *
 * @return If the message queue was sent successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_send_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len,
                             int32_t timeout_ms);

/**
 * @brief Receive message from a message queue.
 *
 * @param [in] mq: The handle of the mq to be received.
 * @param [in] msg_ptr:  The buffer to be received.
 * @param [in] len: The length of the buffer.
 * @param [in] timeout_ms: The time in milliseconds to block until receive timeout occurs.
 * If timeout_ms set to @ref ALT_OSAL_TIMEO_FEVR then wait until the message queue to become not
 * empty.
 *
 * @return If the message queue was received successfully then the length of the
 * selected message in bytes is returned. Otherwise negative value is returned.
 */

int32_t alt_osal_recv_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len,
                             int32_t timeout_ms);

/**
 * @brief Create a new event flag.
 *
 * @param [in] flag: Used to pass a handle to the created event flag
 * out of the alt_osal_create_eventflag() function.
 * @param [in] attr: A value that will passed into the created event flag
 * as the event flag's parameter.
 *
 * @return If the event flag was created successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_eventflag(FAR alt_osal_eventflag_handle *flag,
                                  FAR const alt_osal_eventflag_attribute *attr);

/**
 * @brief Delete a specified event flag.
 *
 * @param [in] flag: The handle of the event flag to be deleted.
 *
 * @return If the event flag was deleted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_eventflag(FAR alt_osal_eventflag_handle *flag);

/**
 * @brief Waiting a event flag to be set.
 *
 * @param [in] flag: The handle of the event flag to be waited.
 * @param [in] wptn: The wait bit pattern.
 * @param [in] wmode: The wait mode. If wmode set to @ref ALT_OSAL_WMODE_TWF_ANDW then
 * the release condition requires all the bits in wptn to be set.
 * If wmode set to @ref ALT_OSAL_WMODE_TWF_ORW then the release
 * condition only requires at least one bit in wptn to be set.
 * @param [in] autoclr: Whether or not to clear bits automatically when function returns.
 * @param [in] flagptn: The current bit pattern.
 * @param [in] timeout_ms: The time in milliseconds to wait for the event flag to become set.
 * If timeout_ms set to @ref ALT_OSAL_TIMEO_FEVR then wait until the event flag to become set.
 *
 * @return If the event flag was waited successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_wait_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits wptn,
                                alt_osal_eventflag_mode wmode, bool autoclr,
                                FAR alt_osal_event_bits *flagptn, int32_t timeout_ms);

/**
 * @brief Set a specified event flag.
 *
 * @param [in] flag: The handle of the event flag to be waited.
 * @param [in] setptn: The set bit pattern.
 *
 * @return If the event flag was seted successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_set_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits setptn);

/**
 * @brief Clear a specified event flag.
 *
 * @param [in] flag: The handle of the event flag to be waited.
 * @param [in] clrptn: The clear bit pattern.
 *
 * @return If the event flag was cleared successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_clear_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits clrptn);

/**
 * @brief Create a new timer.
 *
 * @param [in] timer: Used to pass a handle to the created timer
 * out of the alt_osal_create_timer() function.
 * @param [in] autoreload: Whether or not ro start the timer repeatedly.
 * @param [in] callback: The function called when the timer expired.
 * @param [in] argument: A user provided argument of callback
 * @param [in] attr: A value that will passed into the created timer
 * as the timer's parameter.
 *
 * @return If the timer was started successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_create_timer(FAR alt_osal_timer_handle *timer, bool autoreload,
                              CODE alt_osal_timer_cb_t callback, void *argument,
                              alt_osal_timer_attribute *attr);

/**
 * @brief Start timer.
 *
 * @param [in] timer: The handle of the timer to be started.
 * @param [in] period_ms: The period of the timer in milliseconds.
 *
 * @return If the timer was started successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_start_timer(FAR alt_osal_timer_handle *timer, uint32_t period_ms);

/**
 * @brief Stop timer.
 *
 * @param [in] timer: The handle of the timer to be stopped.
 *
 * @return If the timer was started successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_stop_timer(FAR alt_osal_timer_handle *timer);

/**
 * @brief Delete timer.
 *
 * @param [in] timer: The handle of the timer to be deleted.

 * @return If the timer was started successfully then 0 is returned.
 * Otherwise negative value is returned.
 */

int32_t alt_osal_delete_timer(FAR alt_osal_timer_handle *timer);

/**
 * @brief The alt_osal_thread_cond_init() function shall initialize the condition
 * variable referenced by cond with attributes referenced by attr.
 * If attr is NULL, the default condition variable attributes shall be used.
 *
 * @param [in] cond: Condition variable.
 * @param [in] cond_attr: Condition attributes.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_thread_cond_init(FAR alt_osal_thread_cond_handle *cond,
                                  FAR alt_osal_thread_cond_attribute *cond_attr);

/**
 * @brief The alt_osal_thread_cond_destroy() function shall destroy the given
 * condition variable specified by cond.
 *
 * @param [in] cond: Condition variable.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_thread_cond_destroy(FAR alt_osal_thread_cond_handle *cond);

/**
 * @brief The alt_osal_thread_cond_wait() functions shall block on a condition
 * variable.
 *
 * @param [in] cond:  Condition variable.
 * @param [in] mutex: The handle of the mutex.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_thread_cond_wait(FAR alt_osal_thread_cond_handle *cond,
                                  FAR alt_osal_mutex_handle *mutex);

/**
 * @brief The alt_osal_thread_cond_timedwait() functions shall block on a condition
 * variable.
 *
 * @param [in] cond: Condition variable.
 * @param [in] mutex: The handle of the mutex.
 * @param [in] timeout_ms: The time in milliseconds to wait.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_thread_cond_timedwait(FAR alt_osal_thread_cond_handle *cond,
                                       FAR alt_osal_mutex_handle *mutex, int32_t timeout_ms);

/**
 * @brief The alt_osal_signal_thread_cond() function shall unblock at least one of the
 * threads that are blocked on the specified condition variable cond.
 *
 * @param [in] cond: Condition variable.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_signal_thread_cond(FAR alt_osal_thread_cond_handle *cond);

/**
 * @brief The alt_osal_broadcast_thread_cond() function shall unblock all of the
 * threads that are blocked on the specified condition variable cond.
 * This function is utility wrapper for using alt_osal_thread_cond_signal.
 *
 * @param [in] cond: Condition variable.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

int32_t alt_osal_broadcast_thread_cond(FAR alt_osal_thread_cond_handle *cond);

/**
 * @brief Set the specified Task Flags of a thread.
 *
 * @param [in] handle: Task handle that the flag is sending to
 * @param [in] flags: Flags to be sent to a task
 * @return int32_t task flags after setting or negtive value on error
 */
int32_t alt_osal_set_taskflag(FAR alt_osal_task_handle handle, uint32_t flags);

/**
 * @brief Wait specified flags
 *
 * @param [in] flags: Flags to be waiting for
 * @param [in] options: Possible value are ALT_OSAL_WMODE_TWF_ORW, ALT_OSAL_WMODE_TWF_ANDW and
 * ALT_OSAL_FLAG_NO_CLEAR
 * @param [in] timeout: Specify the timeout value on waiting flags
 * @return int32_t Task flags before clearing or negtive value on error
 */
int32_t alt_osal_wait_taskflag(uint32_t flags, uint32_t options, uint32_t timeout);

/**
 * @brief Clear the specified Thread Flags of current running thread.
 *
 * @param [in] flags: Specify the flags that shall be cleared
 * @return int32_t thread flags before clearing or negtive value on error
 */
int32_t alt_osal_clear_taskflag(uint32_t flags);

/**
 * @brief Get the task handle of current running task
 *
 * @return alt_osal_task_handle
 */
alt_osal_task_handle alt_osal_get_current_task_handle(void);

/**
 * @brief Enter critical section
 *
 */
void alt_osal_enter_critital_section(void);

/**
 * @brief Exit critical section
 *
 */
void alt_osal_exit_critital_section(void);

/**
 * @brief Get the RTOS kernel tick count
 *
 * @return uint32_t tick count
 */
uint32_t alt_osal_get_tick_count(void);

/**
 * @brief Get the RTOS kernel tick frequency
 *
 * @return uint32_t tick frequency HZ
 */
uint32_t alt_osal_get_tick_freq(void);

/**
 * @brief Enter critical section
 *
 * @return uint32_t interrupt status
 */
uint32_t alt_osal_enter_critical(void);

/**
 * @brief Exit critical section
 *
 * @param [in] status: Specify the flags that shall be cleared
 *
 * @return 0: success; otherwise: fail
 */
int32_t alt_osal_exit_critical(uint32_t status);

/**
 * @brief Is inside interrupt
 *
 * @return boolean (true /false)
 */
bool alt_osal_is_inside_interrupt(void);

/**
 * @brief Task yield.
 *
 */
void alt_osal_task_yield(void);

/**
 * @brief Get tasks status with a specified amount of array.
 *
 * @param [inout] task_array: The destination array to store the task status.
 * @param [in] array_size: The size of destination array.
 *
 * @return Tasks amount stored in destination array or 0 if failed to get information(array too
 * small, ...etc)
 */
uint32_t alt_osal_get_tasks_status(alt_osal_task_status *const task_array, size_t array_size);

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/**
 * @brief Create thread condition and mutex resorces. This function is utility
 *   wrapper for using alt_osal_thread_cond_init.
 *
 * @param [in] cond: Condition variable.
 * @param [in] mutex: The handle of the mutex.
 *
 * @return If successful, shall return zero. Otherwise negative value is returned.
 */

static inline int32_t alt_osal_create_thread_cond_mutex(FAR alt_osal_thread_cond_handle *cond,
                                                        FAR alt_osal_mutex_handle *mutex) {
  int32_t ret;
  alt_osal_mutex_attribute mtx_param = {0};

  ret = alt_osal_create_mutex(mutex, &mtx_param);
  if (ret == 0) {
    ret = alt_osal_thread_cond_init(cond, NULL);
    if (ret != 0) {
      alt_osal_delete_mutex(mutex);
    }
  }

  return ret;
}

/**
 * @brief Delete thread condition and mutex resorces. This function is utility
 * wrapper for using alt_osal_thread_cond_destroy.
 *
 * @param [in] cond: Condition variable.
 * @param [in] mutex: The handle of the mutex.
 */

static inline void alt_osal_delete_thread_cond_mutex(FAR alt_osal_thread_cond_handle *cond,
                                                     FAR alt_osal_mutex_handle *mutex) {
  alt_osal_delete_mutex(mutex);
  alt_osal_thread_cond_destroy(cond);
}

/** @} alt_osal_apis */
#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} alt_osal */

#endif /* MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_H */
