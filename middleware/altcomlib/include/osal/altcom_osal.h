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

#ifndef __MODULES_INCLUDE_OSAL_ATLCOM_OSAL_H
#define __MODULES_INCLUDE_OSAL_ATLCOM_OSAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_cc.h"
#include "altcom_osal_opt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef ALTCOM_SYS_MALLOC
#define ALTCOM_SYS_MALLOC(sz) malloc(sz)
#endif

#ifndef ALTCOM_SYS_FREE
#define ALTCOM_SYS_FREE(ptr) free(ptr)
#endif

#define ALTCOM_SYS_TIMEO_NOWAIT (0)        /* no waiting */
#define ALTCOM_SYS_TIMEO_FEVR (-1)         /* timeout forever */
#define ALTCOM_SYS_WMODE_TWF_ANDW (0x0000) /* wait mode : AND */
#define ALTCOM_SYS_WMODE_TWF_ORW (0x0001)  /* wait mode : OR */

#define ALTCOM_SYS_TASK_PRIO_LOW (0)
#define ALTCOM_SYS_TASK_PRIO_NORMAL (1)
#define ALTCOM_SYS_TASK_PRIO_HIGH (2)

#define ALTCOM_SYS_OWN_TASK (NULL)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
  FAR void *arg;
  CODE void (*function)(FAR void *arg);
  FAR const int8_t *name;
  int32_t priority;
  uint32_t stack_size;
} altcom_sys_cretask_s;

typedef struct {
  uint32_t initial_count;
  uint32_t max_count;
} altcom_sys_cresem_s;

typedef struct {
  int8_t dummy;
} altcom_sys_cremtx_s;

typedef struct {
  int32_t numof_queue;
  int32_t queue_size;
} altcom_sys_cremq_s;

typedef struct {
  int8_t dummy;
} altcom_sys_creevflag_s;

typedef void (*altcom_sys_timer_cb_t)(altcom_sys_timer_t timer);

#ifndef bool
typedef uint32_t bool;
#define true(1)
#define false(0)
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sys_create_task
 *
 * Description:
 *   Create a new task.
 *
 * Input Parameters:
 *   task   Used to pass a handle to the created task
 *          out of the altcom_sys_create_task() function.
 *   params A value that will passed into the created task
 *          as the task's parameter.
 *
 * Returned Value:
 *   If the task was created successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_create_task(FAR altcom_sys_task_t *task, FAR const altcom_sys_cretask_s *params);

/****************************************************************************
 * Name: altcom_sys_delete_task
 *
 * Description:
 *   Delete a specified task.
 *
 * Input Parameters:
 *   task   The handle of the task to be deleted.
 *          If task set to ALTCOM_SYS_OWN_TASK then delete caller own task.
 *
 * Returned Value:
 *   If the task was deleted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_delete_task(FAR altcom_sys_task_t *task);

/****************************************************************************
 * Name: altcom_sys_sleep_task
 *
 * Description:
 *   Make self task sleep.
 *
 * Input Parameters:
 *   timeout_ms   The sleep time in milliseconds.
 *
 * Returned Value:
 *   If the task was slept successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_sleep_task(int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_enable_dispatch
 *
 * Description:
 *   Resume the scheduler.
 *
 * Input Parameters:
 *   none
 *
 * Returned Value:
 *   If the scheduler was resumed successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_enable_dispatch(void);

/****************************************************************************
 * Name: altcom_sys_disable_dispatch
 *
 * Description:
 *   Suspend the scheduler.
 *
 * Input Parameters:
 *   none
 *
 * Returned Value:
 *   If the scheduler was suspended successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_disable_dispatch(void);

/****************************************************************************
 * Name: altcom_sys_create_semaphore
 *
 * Description:
 *   Create a new semaphore.
 *
 * Input Parameters:
 *   sem   Used to pass a handle to the created semaphore
 *          out of the altcom_sys_create_semaphore() function.
 *   params A value that will passed into the created semaphore
 *          as the semaphore's parameter.
 *
 * Returned Value:
 *   If the semaphore was created successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_create_semaphore(FAR altcom_sys_sem_t *sem,
                                    FAR const altcom_sys_cresem_s *params);

/****************************************************************************
 * Name: altcom_sys_delete_semaphore
 *
 * Description:
 *   Delete a specified semaphore.
 *
 * Input Parameters:
 *   sem   The handle of the semaphore to be deleted.
 *
 * Returned Value:
 *   If the semaphore was deleted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_delete_semaphore(FAR altcom_sys_sem_t *sem);

/****************************************************************************
 * Name: altcom_sys_wait_semaphore
 *
 * Description:
 *   Waiting a semaphore for become available.
 *
 * Input Parameters:
 *   sem        The handle of the semaphore to be waited.
 *   timeout_ms The time in milliseconds to wait for the semaphore
 *              to become available.
 *              If timeout_ms set to ALTCOM_SYS_TIMEO_FEVR then wait until
 *              the semaphore to become available.
 *
 * Returned Value:
 *   If the semaphore was become available then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_wait_semaphore(FAR altcom_sys_sem_t *sem, int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_post_semaphore
 *
 * Description:
 *   Post a semaphore.
 *
 * Input Parameters:
 *   sem The handle of the semaphore to be posted.
 *
 * Returned Value:
 *   If the semaphore was posted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_post_semaphore(FAR altcom_sys_sem_t *sem);

/****************************************************************************
 * Name: altcom_sys_create_mutex
 *
 * Description:
 *   Create a new mutex.
 *
 * Input Parameters:
 *   mutex  Used to pass a handle to the created mutex
 *          out of the altcom_sys_create_mutex() function.
 *   params A value that will passed into the created mutex
 *          as the mutex's parameter.
 *
 * Returned Value:
 *   If the mutex was created successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_create_mutex(FAR altcom_sys_mutex_t *mutex,
                                FAR const altcom_sys_cremtx_s *params);

/****************************************************************************
 * Name: altcom_sys_delete_mutex
 *
 * Description:
 *   Delete a specified mutex.
 *
 * Input Parameters:
 *   mutex The handle of the mutex to be deleted.
 *
 * Returned Value:
 *   If the mutex was deleted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_delete_mutex(FAR altcom_sys_mutex_t *mutex);

/****************************************************************************
 * Name: altcom_sys_lock_mutex
 *
 * Description:
 *   Lock a mutex.
 *
 * Input Parameters:
 *   mutex The handle of the mutex to be locked.
 *
 * Returned Value:
 *   If the mutex was locked successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_lock_mutex(FAR altcom_sys_mutex_t *mutex);

/****************************************************************************
 * Name: altcom_sys_unlock_mutex
 *
 * Description:
 *   Unlock a mutex.
 *
 * Input Parameters:
 *   mutex The handle of the mutex to be unlocked.
 *
 * Returned Value:
 *   If the mutex was unlocked successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_unlock_mutex(FAR altcom_sys_mutex_t *mutex);

/****************************************************************************
 * Name: altcom_sys_create_mqueue
 *
 * Description:
 *   Create a new message queue.
 *
 * Input Parameters:
 *   mq     Used to pass a handle to the created message queue
 *          out of the altcom_sys_create_mqueue() function.
 *   params A value that will passed into the created message queue
 *          as the message queue's parameter.
 *
 * Returned Value:
 *   If the message queue was created successfully then 0 is
 *   returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_create_mqueue(FAR altcom_sys_mq_t *mq, FAR const altcom_sys_cremq_s *params);

/****************************************************************************
 * Name: altcom_sys_delete_mqueue
 *
 * Description:
 *   Delete a specified message queue.
 *
 * Input Parameters:
 *   mq The handle of the message queue to be deleted.
 *
 * Returned Value:
 *   If the message queue was deleted successfully then 0 is
 *   returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_delete_mqueue(FAR altcom_sys_mq_t *mq);

/****************************************************************************
 * Name: altcom_sys_send_mqueue
 *
 * Description:
 *   Send message by a message queue.
 *
 * Input Parameters:
 *   mq         The handle of the message queue to be send.
 *   message    The message to be send.
 *   len        The length of the message.
 *   timeout_ms The time in milliseconds to block until send timeout occurs.
 *              If timeout_ms set to ALTCOM_SYS_TIMEO_FEVR then wait until
 *              the message queue to become available.
 *
 * Returned Value:
 *   If the message queue was sent successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_send_mqueue(FAR altcom_sys_mq_t *mq, FAR int8_t *message, size_t len,
                               int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_recv_mqueue
 *
 * Description:
 *   Receive message from a message queue.
 *
 * Input Parameters:
 *   mq         The handle of the mq to be received.
 *   message    The buffer to be received.
 *   len        The length of the buffer.
 *   timeout_ms The time in milliseconds to block until receive timeout
 *              occurs.
 *              If timeout_ms set to ALTCOM_SYS_TIMEO_FEVR then wait until
 *              the message queue to become not empty.
 *
 * Returned Value:
 *   If the message queue was received successfully then the length of the
 *   selected message in bytes is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_recv_mqueue(FAR altcom_sys_mq_t *mq, FAR int8_t *message, size_t len,
                               int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_create_evflag
 *
 * Description:
 *   Create a new event flag.
 *
 * Input Parameters:
 *   flag   Used to pass a handle to the created event flag
 *          out of the altcom_sys_create_evflag() function.
 *   params A value that will passed into the created event flag
 *          as the event flag's parameter.
 *
 * Returned Value:
 *   If the event flag was created successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_create_evflag(FAR altcom_sys_evflag_t *flag,
                                 FAR const altcom_sys_creevflag_s *params);

/****************************************************************************
 * Name: altcom_sys_delete_evflag
 *
 * Description:
 *   Delete a specified event flag.
 *
 * Input Parameters:
 *   flag The handle of the event flag to be deleted.
 *
 * Returned Value:
 *   If the event flag was deleted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_delete_evflag(FAR altcom_sys_evflag_t *flag);

/****************************************************************************
 * Name: altcom_sys_wait_evflag
 *
 * Description:
 *   Waiting a event flag to be set.
 *
 * Input Parameters:
 *   flag       The handle of the event flag to be waited.
 *   wptn       The wait bit pattern.
 *   wmode      The wait mode. If wmode set to ALTCOM_SYS_WMODE_TWF_ANDW then
 *              the release condition requires all the bits in wptn
 *              to be set.
 *              If wmode set to ALTCOM_SYS_WMODE_TWF_ORW then the release
 *              condition only requires at least one bit in wptn to be set.
 *   flagptn    The current bit pattern.
 *   autoclr    Whether or not to clear bits automatically
 *              when function returns.
 *   timeout_ms The time in milliseconds to wait for the event flag
 *              to become set.
 *              If timeout_ms set to ALTCOM_SYS_TIMEO_FEVR then wait until
 *              the event flag to become set.
 *
 * Returned Value:
 *   If the event flag was waited successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_wait_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t wptn,
                               altcom_sys_evflag_mode_t wmode, bool autoclr,
                               FAR altcom_sys_evflag_ptn_t *flagptn, int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_set_evflag
 *
 * Description:
 *   Set a specified event flag.
 *
 * Input Parameters:
 *   flag       The handle of the event flag to be waited.
 *   setptn     The set bit pattern.
 *
 * Returned Value:
 *   If the event flag was seted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_set_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t setptn);

/****************************************************************************
 * Name: altcom_sys_set_evflag_isr
 *
 * Description:
 *   Set a specified event flag. A version of altcom_sys_set_evflag() that can be
 *   called from an interrupt service routine(ISR).
 *
 * Input Parameters:
 *   flag       The handle of the event flag to be waited.
 *   setptn     The set bit pattern.
 *
 * Returned Value:
 *   If the event flag was seted successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_set_evflag_isr(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t setptn);

/****************************************************************************
 * Name: altcom_sys_clear_evflag
 *
 * Description:
 *   Clear a specified event flag.
 *
 * Input Parameters:
 *   flag       The handle of the event flag to be waited.
 *   clrptn     The clear bit pattern.
 *
 * Returned Value:
 *   If the event flag was cleared successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_clear_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t clrptn);

/****************************************************************************
 * Name: altcom_sys_start_timer
 *
 * Description:
 *   Create and start timer.
 *
 * Input Parameters:
 *   timer        The handle of the timer to be started.
 *   period_ms    The period of the timer in milliseconds.
 *   autoreload   Whether or not ro start the timer repeatedly.
 *   callback     The function called when the timer expired.
 *
 * Returned Value:
 *   If the timer was started successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_start_timer(FAR altcom_sys_timer_t *timer, int32_t period_ms, bool autoreload,
                               CODE altcom_sys_timer_cb_t callback);

/****************************************************************************
 * Name: altcom_sys_stop_timer
 *
 * Description:
 *   Stop and delete timer.
 *
 * Input Parameters:
 *   timer        The handle of the timer to be stopped.
 *
 * Returned Value:
 *   If the timer was started successfully then 0 is returned.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_stop_timer(FAR altcom_sys_timer_t *timer);

/****************************************************************************
 * Name: altcom_sys_thread_cond_init
 *
 * Description:
 *   The altcom_sys_thread_cond_init() function shall initialize the condition
 *   variable referenced by cond with attributes referenced by attr.
 *   If attr is NULL, the default condition variable attributes shall be
 *   used.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   cond_attr   Condition attributes.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_thread_cond_init(FAR altcom_sys_thread_cond_t *cond,
                                    FAR altcom_sys_thread_condattr_t *cond_attr);

/****************************************************************************
 * Name: altcom_sys_thread_cond_destroy
 *
 * Description:
 *   The altcom_sys_thread_cond_destroy() function shall destroy the given
 *   condition variable specified by cond.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_thread_cond_destroy(FAR altcom_sys_thread_cond_t *cond);

/****************************************************************************
 * Name: altcom_sys_thread_cond_wait
 *
 * Description:
 *   The altcom_sys_thread_cond_wait() functions shall block on a condition
 *   variable.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_thread_cond_wait(FAR altcom_sys_thread_cond_t *cond,
                                    FAR altcom_sys_mutex_t *mutex);

/****************************************************************************
 * Name: altcom_sys_thread_cond_timedwait
 *
 * Description:
 *   The altcom_sys_thread_cond_timedwait() functions shall block on a condition
 *   variable.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *   timeout_ms  The time in milliseconds to wait.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_thread_cond_timedwait(FAR altcom_sys_thread_cond_t *cond,
                                         FAR altcom_sys_mutex_t *mutex, int32_t timeout_ms);

/****************************************************************************
 * Name: altcom_sys_signal_thread_cond
 *
 * Description:
 *   The altcom_sys_signal_thread_cond() function shall unblock at least one of the
 *   threads that are blocked on the specified condition variable cond.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_signal_thread_cond(FAR altcom_sys_thread_cond_t *cond);

/****************************************************************************
 * Name: altcom_sys_broadcast_thread_cond
 *
 * Description:
 *   The altcom_sys_broadcast_thread_cond() function shall unblock all of the
 *   threads that are blocked on the specified condition variable cond.
 *   This function is utility wrapper for using altcom_sys_thread_cond_signal.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

int32_t altcom_sys_broadcast_thread_cond(FAR altcom_sys_thread_cond_t *cond);

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sys_create_thread_cond_mutex
 *
 * Description:
 *   Create thread condition and mutex resorces. This function is utility
 *   wrapper for using altcom_sys_thread_cond_init.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *
 * Returned Value:
 *   If successful, shall return zero.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

static inline int32_t altcom_sys_create_thread_cond_mutex(FAR altcom_sys_thread_cond_t *cond,
                                                          FAR altcom_sys_mutex_t *mutex) {
  int32_t ret;
  altcom_sys_cremtx_s mtx_param = {0};

  ret = altcom_sys_create_mutex(mutex, &mtx_param);
  if (ret == 0) {
    ret = altcom_sys_thread_cond_init(cond, NULL);
    if (ret != 0) {
      altcom_sys_delete_mutex(mutex);
    }
  }

  return ret;
}

/****************************************************************************
 * Name: altcom_sys_delete_thread_cond_mutex
 *
 * Description:
 *   Delete thread condition and mutex resorces. This function is utility
 *   wrapper for using altcom_sys_thread_cond_destroy.
 *
 * Input Parameters:
 *   cond        Condition variable.
 *   mutex       The handle of the mutex.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static inline void altcom_sys_delete_thread_cond_mutex(FAR altcom_sys_thread_cond_t *cond,
                                                       FAR altcom_sys_mutex_t *mutex) {
  altcom_sys_delete_mutex(mutex);
  altcom_sys_thread_cond_destroy(cond);
}

/****************************************************************************
 * Name: sys_altcom_assert
 *
 * Description:
 *   Check assertion condition.
 *
 * Input Parameters:
 *   condition   Assertion condition.
 *
 * Returned Value:
 * None
 *
 ****************************************************************************/

#define sys_altcom_assert(x) \
    do {                     \
        configASSERT(x);     \
    } while (0)

#endif /* __MODULES_INCLUDE_OSAL_ALTCOM_OSAL_H */
