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

#include <stdio.h>
#include "altcom_osal.h"

#define TASK_PRIO_LOW 1
#define TASK_PRIO_NORMAL 2
#define TASK_PRIO_HIGH 3
#define TASK_PRIO_MIN (ALTCOM_SYS_TASK_PRIO_LOW)
#define TASK_PRIO_MAX (ALTCOM_SYS_TASK_PRIO_HIGH)
#define NUM_OF_TASK_PRIO 3

#define TIMER_NAME_MAX (16)
#define TIMER_ID_DEFAULT (0)
#define TIMER_NAME_FMT "Timer%u"

int32_t altcom_sys_create_task(FAR altcom_sys_task_t *task,
                               FAR const altcom_sys_cretask_s *params) {
  BaseType_t ret;
  const UBaseType_t prio_table[NUM_OF_TASK_PRIO] = {TASK_PRIO_LOW, TASK_PRIO_NORMAL,
                                                    TASK_PRIO_HIGH};

  if (!params || !params->function || !params->name) {
    return -EINVAL;
  }

  if ((params->priority < TASK_PRIO_MIN) || (TASK_PRIO_MAX < params->priority)) {
    return -EINVAL;
  }

  ret = xTaskCreate((TaskFunction_t)params->function, (const char *)params->name,
                    (uint16_t)params->stack_size / STACK_WORD_SIZE, params->arg,
                    prio_table[params->priority], task);

  if (ret == pdPASS) {
    return 0;
  } else {
    return -ENOMEM;
  }
}

int32_t altcom_sys_delete_task(FAR altcom_sys_task_t *task) {
  if (!task) {
    vTaskDelete(ALTCOM_SYS_OWN_TASK);
  } else {
    vTaskDelete(*task);
  }

  return 0;
}

int32_t altcom_sys_sleep_task(int32_t timeout_ms) {
  vTaskDelay(timeout_ms / portTICK_PERIOD_MS);

  return 0;
}

int32_t altcom_sys_enable_dispatch(void) {
  BaseType_t ret;

  ret = xTaskResumeAll();

  if (ret == pdTRUE) {
    return 0;
  } else {
    return -EBUSY;
  }
}

int32_t altcom_sys_disable_dispatch(void) {
  vTaskSuspendAll();

  return 0;
}

int32_t altcom_sys_create_semaphore(FAR altcom_sys_sem_t *sem,
                                    FAR const altcom_sys_cresem_s *params) {
  if (!sem) {
    return -EINVAL;
  }

  // Counting semaphore is not supported.
  if (params && ((params->max_count > 1) || (params->initial_count != 0))) {
    return -EINVAL;
  }

  *sem = xSemaphoreCreateBinary();

  if (*sem) {
    return 0;
  } else {
    return -ENOMEM;
  }
}

int32_t altcom_sys_delete_semaphore(FAR altcom_sys_sem_t *sem) {
  if (!sem) {
    return -EINVAL;
  } else {
    vSemaphoreDelete(*sem);
    return 0;
  }
}

int32_t altcom_sys_wait_semaphore(FAR altcom_sys_sem_t *sem, int32_t timeout_ms) {
  BaseType_t ret;

  if (!sem) {
    return -EINVAL;
  }

  if (ALTCOM_SYS_TIMEO_FEVR == timeout_ms) {
    ret = xSemaphoreTake(*sem, portMAX_DELAY);
  } else {
    ret = xSemaphoreTake(*sem, timeout_ms / portTICK_PERIOD_MS);
  }

  if (ret == pdPASS) {
    return 0;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_post_semaphore(FAR altcom_sys_sem_t *sem) {
  BaseType_t ret;

  if (!sem) {
    return -EINVAL;
  }

  ret = xSemaphoreGive(*sem);

  if (ret == pdPASS) {
    return 0;
  } else {
    return -EPERM;
  }
}

int32_t altcom_sys_create_mutex(FAR altcom_sys_mutex_t *mutex,
                                FAR const altcom_sys_cremtx_s *params) {
  if (!mutex) {
    return -EINVAL;
  }

  *mutex = xSemaphoreCreateMutex();

  if (*mutex) {
    return 0;
  } else {
    return -ENOMEM;
  }
}

int32_t altcom_sys_delete_mutex(FAR altcom_sys_mutex_t *mutex) {
  if (!mutex) {
    return -EINVAL;
  } else {
    vSemaphoreDelete(*mutex);
    return 0;
  }
}

int32_t altcom_sys_lock_mutex(FAR altcom_sys_mutex_t *mutex) {
  BaseType_t ret;
  if (!mutex) {
    return -EINVAL;
  }

  ret = xSemaphoreTake(*mutex, portMAX_DELAY);

  if (ret == pdPASS) {
    return 0;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_unlock_mutex(FAR altcom_sys_mutex_t *mutex) {
  BaseType_t ret;

  if (!mutex) {
    return -EINVAL;
  }

  ret = xSemaphoreGive(*mutex);

  if (ret == pdPASS) {
    return 0;
  } else {
    return -EPERM;
  }
}

int32_t altcom_sys_create_mqueue(FAR altcom_sys_mq_t *mq, FAR const altcom_sys_cremq_s *params) {
  if (!mq || !params) {
    return -EINVAL;
  }

  *mq = xQueueCreate((UBaseType_t)params->numof_queue, (UBaseType_t)params->queue_size);

  if (*mq) {
    return 0;
  } else {
    return -ENOMEM;
  }
}

int32_t altcom_sys_delete_mqueue(FAR altcom_sys_mq_t *mq) {
  if (!mq) {
    return -EINVAL;
  } else {
    vQueueDelete(*mq);
    return 0;
  }
}

int32_t altcom_sys_send_mqueue(FAR altcom_sys_mq_t *mq, FAR int8_t *message, size_t len,
                               int32_t timeout_ms) {
  BaseType_t ret;

  if (!mq || !message || !(*mq)) {
    return -EINVAL;
  }

  if (ALTCOM_SYS_TIMEO_FEVR == timeout_ms) {
    ret = xQueueSend(*mq, (const void *)message, portMAX_DELAY);
  } else {
    ret = xQueueSend(*mq, (const void *)message, timeout_ms / portTICK_PERIOD_MS);
  }

  if (ret == pdPASS) {
    return 0;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_recv_mqueue(FAR altcom_sys_mq_t *mq, FAR int8_t *message, size_t len,
                               int32_t timeout_ms) {
  BaseType_t ret;

  if (!mq || !message || !(*mq)) {
    return -EINVAL;
  }

  if (ALTCOM_SYS_TIMEO_FEVR == timeout_ms) {
    ret = xQueueReceive(*mq, (void *)message, portMAX_DELAY);
  } else {
    ret = xQueueReceive(*mq, (void *)message, timeout_ms / portTICK_PERIOD_MS);
  }

  if (ret == pdPASS) {
    return len;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_create_evflag(FAR altcom_sys_evflag_t *flag,
                                 FAR const altcom_sys_creevflag_s *params) {
  if (!flag) {
    return -EINVAL;
  }

  *flag = xEventGroupCreate();

  if (*flag) {
    return 0;
  } else {
    return -ENOMEM;
  }
}

int32_t altcom_sys_delete_evflag(FAR altcom_sys_evflag_t *flag) {
  if (!flag) {
    return -EINVAL;
  } else {
    vEventGroupDelete(*flag);
    return 0;
  }
}

int32_t altcom_sys_wait_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t wptn,
                               altcom_sys_evflag_mode_t wmode, bool autoclr,
                               FAR altcom_sys_evflag_ptn_t *flagptn, int32_t timeout_ms) {
  EventBits_t wait_allbits = pdTRUE;

  if (!flag || !flagptn) {
    return -EINVAL;
  }

  if (ALTCOM_SYS_WMODE_TWF_ANDW == wmode) {
    wait_allbits = pdTRUE;
  } else if (ALTCOM_SYS_WMODE_TWF_ORW == wmode) {
    wait_allbits = pdFALSE;
  } else {
    return -EINVAL;
  }

  if (ALTCOM_SYS_TIMEO_FEVR == timeout_ms) {
    *flagptn = xEventGroupWaitBits(*flag, (const EventBits_t)wptn, (const BaseType_t)autoclr,
                                   (const BaseType_t)wait_allbits, portMAX_DELAY);
  } else {
    *flagptn = xEventGroupWaitBits(*flag, (const EventBits_t)wptn, (const BaseType_t)autoclr,
                                   (const BaseType_t)wait_allbits, timeout_ms / portTICK_PERIOD_MS);
  }

  return 0;
}

int32_t altcom_sys_set_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t setptn) {
  if (!flag) {
    return -EINVAL;
  }

  xEventGroupSetBits(*flag, (const EventBits_t)setptn);

  return 0;
}

int32_t altcom_sys_set_evflag_isr(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t setptn) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (!flag) {
    return -EINVAL;
  }

  if (pdFAIL !=
      xEventGroupSetBitsFromISR(*flag, (const EventBits_t)setptn, &xHigherPriorityTaskWoken)) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
  return 0;
}

int32_t altcom_sys_clear_evflag(FAR altcom_sys_evflag_t *flag, altcom_sys_evflag_ptn_t clrptn) {
  if (!flag) {
    return -EINVAL;
  } else {
    xEventGroupClearBits(*flag, (const EventBits_t)clrptn);
    return 0;
  }
}

int32_t altcom_sys_start_timer(FAR altcom_sys_timer_t *timer, int32_t period_ms, bool autoreload,
                               CODE altcom_sys_timer_cb_t callback) {
  static unsigned short timer_cnt = 0;
  char name[TIMER_NAME_MAX];

  if (!timer || (0 > period_ms) || !callback) {
    return -EINVAL;
  }

  snprintf(name, TIMER_NAME_MAX, TIMER_NAME_FMT, timer_cnt);
  *timer = xTimerCreate(name, period_ms / portTICK_PERIOD_MS, (UBaseType_t)autoreload,
                        TIMER_ID_DEFAULT, callback);

  if (!*timer) {
    return -ENOMEM;
  }

  if (pdPASS == xTimerStart(*timer, portMAX_DELAY)) {
    timer_cnt++;
    return 0;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_stop_timer(FAR altcom_sys_timer_t *timer) {
  if (!timer) {
    return -EINVAL;
  }

  if (pdPASS != xTimerStop(*timer, portMAX_DELAY)) {
    return -ETIME;
  }

  if (pdPASS == xTimerDelete(*timer, portMAX_DELAY)) {
    return 0;
  } else {
    return -ETIME;
  }
}

int32_t altcom_sys_thread_cond_init(FAR altcom_sys_thread_cond_t *condition,
                                    FAR altcom_sys_thread_condattr_t *cond_attr) {
  altcom_thread_cond_variable_t *cond, **cond_ptr;
  int ret = 0;

  cond = (altcom_thread_cond_variable_t *)pvPortMalloc(sizeof(altcom_thread_cond_variable_t));
  if (cond) {
    if (altcom_sys_create_mutex(&(cond->cond_mutex), NULL) < 0) {
      vPortFree(cond);
      ret = -ENOMEM;
    } else {
      altcom_sys_cremq_s que_param;

      que_param.numof_queue = ALTCOM_MAX_TASKS_WAITING_ON_COND_VAR;
      que_param.queue_size = sizeof(void *);
      if (altcom_sys_create_mqueue(&(cond->evtQue), &que_param) < 0) {
        altcom_sys_delete_mutex(&(cond->cond_mutex));
        vPortFree(cond);
        ret = -ENOMEM;
      } else {
        cond->num_waiting = 0;
        cond_ptr = (altcom_thread_cond_variable_t **)condition;
        *cond_ptr = cond;
      }
    }
  } else {
    ret = -ENOMEM;
  }

  return ret;
}

int32_t altcom_sys_thread_cond_destroy(FAR altcom_sys_thread_cond_t *condition) {
  altcom_thread_cond_variable_t **cond = (altcom_thread_cond_variable_t **)condition;

  if (cond && *cond) {
    altcom_sys_delete_mutex(&((*cond)->cond_mutex));
    altcom_sys_delete_mqueue(&((*cond)->evtQue));
    vPortFree(*cond);
    return 0;
  } else {
    return -EINVAL;
  }
}

static int __altcom_sys_thread_cond_wait(altcom_sys_thread_cond_t *condition,
                                         altcom_sys_mutex_t *mutex, int ticks) {
  altcom_thread_cond_variable_t **cond = (altcom_thread_cond_variable_t **)condition;
  int ret = 0;
  int32_t qRet;
  void *evt;

  if (cond && *cond && mutex) {
    altcom_sys_lock_mutex(&((*cond)->cond_mutex));
    (*cond)->num_waiting++;
    altcom_sys_unlock_mutex(&((*cond)->cond_mutex));
    altcom_sys_unlock_mutex(mutex);
    qRet = altcom_sys_recv_mqueue(
        &((*cond)->evtQue), (int8_t *)(&evt), sizeof(void *),
        ticks > 0 ? (int32_t)(ticks * portTICK_PERIOD_MS) : (int32_t)ALTCOM_SYS_TIMEO_FEVR);
    altcom_sys_lock_mutex(mutex);
    if (-ETIME == qRet) {
      ret = -ETIMEDOUT;
    } else {
      sys_altcom_assert(evt == (*cond)->cond_mutex);
    }

    altcom_sys_lock_mutex(&((*cond)->cond_mutex));
    (*cond)->num_waiting--;
    altcom_sys_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

int32_t altcom_sys_thread_cond_wait(FAR altcom_sys_thread_cond_t *condition,
                                    FAR altcom_sys_mutex_t *mutex) {
  return __altcom_sys_thread_cond_wait(condition, mutex, 0);
}

int32_t altcom_sys_thread_cond_timedwait(FAR altcom_sys_thread_cond_t *condition,
                                         FAR altcom_sys_mutex_t *mutex, int32_t timeout_ms) {
  int32_t delay;
  delay = timeout_ms / portTICK_PERIOD_MS;
  return __altcom_sys_thread_cond_wait(condition, mutex, delay);
}

int32_t altcom_sys_signal_thread_cond(FAR altcom_sys_thread_cond_t *condition) {
  int ret = 0;
  altcom_thread_cond_variable_t **cond = (altcom_thread_cond_variable_t **)condition;

  if (cond && *cond) {
    altcom_sys_lock_mutex(&((*cond)->cond_mutex));
    if ((*cond)->num_waiting) {
      altcom_sys_send_mqueue(&((*cond)->evtQue), (int8_t *)(&(*cond)->cond_mutex), sizeof(void *),
                             ALTCOM_SYS_TIMEO_FEVR);
    }

    altcom_sys_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

int32_t altcom_sys_broadcast_thread_cond(FAR altcom_sys_thread_cond_t *condition) {
  int ret = 0;
  int i;
  altcom_thread_cond_variable_t **cond = (altcom_thread_cond_variable_t **)condition;

  if (cond && *cond) {
    altcom_sys_lock_mutex(&((*cond)->cond_mutex));
    for (i = 0; i < (*cond)->num_waiting; i++) {
      altcom_sys_send_mqueue(&((*cond)->evtQue), (int8_t *)(&(*cond)->cond_mutex), sizeof(void *),
                             ALTCOM_SYS_TIMEO_FEVR);
    }

    altcom_sys_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

void sys_task_yield(void) { taskYIELD(); }
