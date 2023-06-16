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

/* --------------------------------------------------------------------------
 * Copyright (c) 2013-2020 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *      Name:    freertos_osal.c
 *      Purpose: Altair OSAL wrapper (modified from cmsis_os2.c) for FreeRTOS
 *
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include DEVICE_HEADER
#include "alt_osal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_BITS_EVENT_GROUPS 24U
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

#define TIMER_NAME_MAX (32)
#define TIMER_ID_DEFAULT (0)
#define TIMER_NAME_FMT "Timer%lu"

#define MAX_TASKS_WAITING_ON_COND_VAR (10)

#define IS_IRQ_MASKED() ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U))
#define IS_IRQ_MODE() (__get_IPSR() != 0U)

#define ALT_OSAL_TASK_FLAG_INDEX (configTASK_NOTIFICATION_ARRAY_ENTRIES - 1)

/****************************************************************************
 * Private Data Types
 ****************************************************************************/
typedef struct {
  alt_osal_mutex_handle cond_mutex;
  int num_waiting;
  alt_osal_queue_handle evtQue;
} alt_osal_condition_variable;

typedef struct {
  alt_osal_timer_cb_t func;
  void *arg;
} timer_cb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* clang-format off */
/*
 static void alt_osal_timer_callback(TimerHandle_t hTimer)
 is from
 cmsis_os2.c -> static void TimerCallback (TimerHandle_t hTimer)
*/
/* clang-format on */
static void alt_osal_timer_callback(TimerHandle_t hTimer) {
  timer_cb *cb;

  cb = (timer_cb *)pvTimerGetTimerID(hTimer);

  if (cb != NULL) {
    cb->func(cb->arg);
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* clang-format off */
/*
 uint32_t alt_osal_irq_context(void)
 is from
 cmsis_os2.c -> __STATIC_INLINE uint32_t IRQ_Context (void)
*/
/* clang-format on */
uint32_t alt_osal_irq_context(void) {
  uint32_t irq;
  BaseType_t state;

  irq = 0U;

  if (IS_IRQ_MODE()) {
    /* Called from interrupt context */
    irq = 1U;
  } else {
    /* Get FreeRTOS scheduler state */
    state = xTaskGetSchedulerState();

    if (state != taskSCHEDULER_NOT_STARTED) {
      /* Scheduler was started */
      if (IS_IRQ_MASKED()) {
        /* Interrupts are masked */
        irq = 1U;
      }
    }
  }

  /* Return context, 0: thread context, 1: IRQ context */
  return (irq);
}

/* clang-format off */
/*
 int32_t alt_osal_create_task(FAR alt_osal_task_handle *task, FAR const alt_osal_task_attribute *attr)
 is from
 cmsis_os2.c -> osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_task(FAR alt_osal_task_handle *task,
                             FAR const alt_osal_task_attribute *attr) {
  const char *name;
  uint32_t stack;
  TaskHandle_t hTask;
  UBaseType_t prio;
  int32_t mem;

  if (attr == NULL) {
    return (-EINVAL);
  }

  hTask = NULL;

  if ((alt_osal_irq_context() == 0U) && (attr->function != NULL)) {
    stack = configMINIMAL_STACK_SIZE;
    prio = (UBaseType_t)ALT_OSAL_TASK_PRIO_NORMAL;

    name = NULL;
    mem = -1;

    if (attr->name != NULL) {
      name = attr->name;
    }

    if (attr->priority != ALT_OSAL_TASK_PRIO_NONE) {
      prio = (UBaseType_t)attr->priority;
    }

    if ((attr->priority < ALT_OSAL_TASK_PRIO_IDLE) || (attr->priority > ALT_OSAL_TASK_PRIO_ISR)) {
      return (-EINVAL);
    }

    if (attr->stack_size > 0U) {
      /* In FreeRTOS stack is not in bytes, but in sizeof(StackType_t) which is 4 on ARM ports. */
      /* Stack size should be therefore 4 byte aligned in order to avoid division caused side
       * effects */
      stack = attr->stack_size / sizeof(StackType_t);
    }

    if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_TASK_CBSIZE) &&
        (attr->stack_mem != NULL) && (attr->stack_size > 0U)) {
      /* The memory for control block and stack is provided, use static object */
      mem = 1;
    } else {
      if ((attr->cb_mem == NULL) && (attr->cb_size == 0U) && (attr->stack_mem == NULL)) {
        /* Control block and stack memory will be allocated from the dynamic pool */
        mem = 0;
      }
    }

    if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
      hTask = xTaskCreateStatic((TaskFunction_t)attr->function, name, stack, attr->arg, prio,
                                (StackType_t *)attr->stack_mem, (StaticTask_t *)attr->cb_mem);
#endif
    } else {
      if (mem == 0) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
        if (xTaskCreate((TaskFunction_t)attr->function, name, (uint16_t)stack, attr->arg, prio,
                        &hTask) != pdPASS) {
          hTask = NULL;
        }
#endif
      } else {
        return (-EINVAL);
      }
    }
  } else {
    return (-EPERM);
  }

  if (hTask != NULL) {
    if (task != NULL) {
      *task = (alt_osal_task_handle)hTask;
    }
  } else {
    return (-ENOMEM);
  }

  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_task(FAR alt_osal_task_handle *task)
 is from
 cmsis_os2.c -> osStatus_t osThreadTerminate (osThreadId_t thread_id)
*/
/* clang-format on */
int32_t alt_osal_delete_task(FAR alt_osal_task_handle *task) {
  TaskHandle_t hTask = (task != NULL ? (TaskHandle_t)*task : NULL);
  int32_t stat;
  eTaskState tstate;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hTask == NULL) {
    stat = 0;
    vTaskDelete(NULL);
    for (;;)
      ;
  } else {
    tstate = eTaskGetState(hTask);
    if (tstate != eDeleted) {
      stat = 0;
      vTaskDelete(hTask);
    } else {
      stat = (-EBUSY);
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_sleep_task(int32_t timeout_ms)
 is from
 cmsis_os2.c -> osStatus_t osDelay (uint32_t ticks)
*/
/* clang-format on */
int32_t alt_osal_sleep_task(int32_t timeout_ms) {
  int32_t stat;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else {
    stat = 0;
    if (timeout_ms != 0U) {
      vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
    }
  }

  /* Return execution status */
  return (stat);
}

int32_t alt_osal_list_task(char *buf) {
  if (!buf) {
    return -EINVAL;
  }
  vTaskList((char *)buf);
  return 0;
}

/* clang-format off */
/*
 int32_t alt_osal_enable_dispatch(void)
 is from
 cmsis_os2.c -> int32_t osKernelLock (void)
*/
/* clang-format on */
int32_t alt_osal_enable_dispatch(void) {
  int32_t stat = 0;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else {
    switch (xTaskGetSchedulerState()) {
      case taskSCHEDULER_SUSPENDED:
        if (xTaskResumeAll() != pdTRUE) {
          if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) {
            stat = (-EBUSY);
          }
        }

        break;

      case taskSCHEDULER_RUNNING:
        break;

      case taskSCHEDULER_NOT_STARTED:
      default:
        stat = (-EINVAL);
        break;
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_disable_dispatch(void)
 is from
 cmsis_os2.c -> int32_t osKernelUnlock (void)
*/
/* clang-format on */
int32_t alt_osal_disable_dispatch(void) {
  int32_t stat = 0;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else {
    switch (xTaskGetSchedulerState()) {
      case taskSCHEDULER_SUSPENDED:
        break;

      case taskSCHEDULER_RUNNING:
        vTaskSuspendAll();
        stat = 0;
        break;

      case taskSCHEDULER_NOT_STARTED:
      default:
        stat = (-EINVAL);
        break;
    }
  }

  /* Return previous lock state */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_create_semaphore(FAR alt_osal_semaphore_handle *sem, FAR const alt_osal_semaphore_attribute *attr)
 is from
 cmsis_os2.c -> osSemaphoreId_t osSemaphoreNew (uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_semaphore(FAR alt_osal_semaphore_handle *sem,
                                  FAR const alt_osal_semaphore_attribute *attr) {
  SemaphoreHandle_t hSemaphore;
  int32_t mem;

  if (sem == NULL || attr == NULL) {
    return (-EINVAL);
  }

  hSemaphore = NULL;

  if ((alt_osal_irq_context() == 0U) && (attr->max_count > 0U) &&
      (attr->initial_count <= attr->max_count)) {
    mem = -1;

    if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_SEM_CBSIZE)) {
      /* The memory for control block is provided, use static object */
      mem = 1;
    } else {
      if ((attr->cb_mem == NULL) && (attr->cb_size == 0U)) {
        /* Control block will be allocated from the dynamic pool */
        mem = 0;
      }
    }

    if (mem != -1) {
      if (attr->max_count == 1U) {
        if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
          hSemaphore = xSemaphoreCreateBinaryStatic((StaticSemaphore_t *)attr->cb_mem);
#endif
        } else {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
          hSemaphore = xSemaphoreCreateBinary();
#endif
        }

        if ((hSemaphore != NULL) && (attr->initial_count != 0U)) {
          if (xSemaphoreGive(hSemaphore) != pdPASS) {
            vSemaphoreDelete(hSemaphore);
            hSemaphore = NULL;
            return (-EPERM);
          }
        }
      } else {
        if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
          hSemaphore = xSemaphoreCreateCountingStatic(attr->max_count, attr->initial_count,
                                                      (StaticSemaphore_t *)attr->cb_mem);
#endif
        } else {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
          hSemaphore = xSemaphoreCreateCounting(attr->max_count, attr->initial_count);
#endif
        }
      }

#if (configQUEUE_REGISTRY_SIZE > 0)
      if (hSemaphore != NULL) {
        vQueueAddToRegistry(hSemaphore, attr->name);
      }
#endif
    } else {
      return (-EINVAL);
    }
  } else {
    return (-EPERM);
  }

  if (hSemaphore != NULL) {
    *sem = (alt_osal_semaphore_handle)hSemaphore;
  } else {
    return (-ENOMEM);
  }

  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_semaphore(FAR alt_osal_semaphore_handle *sem)
 is from
 cmsis_os2.c -> osStatus_t osSemaphoreDelete (osSemaphoreId_t semaphore_id)
*/
/* clang-format on */
int32_t alt_osal_delete_semaphore(FAR alt_osal_semaphore_handle *sem) {
  SemaphoreHandle_t hSemaphore = (sem != NULL ? (SemaphoreHandle_t)*sem : NULL);
  int32_t stat;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hSemaphore == NULL) {
    stat = (-EINVAL);
  } else {
#if (configQUEUE_REGISTRY_SIZE > 0)
    vQueueUnregisterQueue(hSemaphore);
#endif

    stat = 0;
    vSemaphoreDelete(hSemaphore);
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_wait_semaphore(FAR alt_osal_semaphore_handle *sem, int32_t timeout_ms)
 is from
 cmsis_os2.c -> osStatus_t osSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_wait_semaphore(FAR alt_osal_semaphore_handle *sem, int32_t timeout_ms) {
  SemaphoreHandle_t hSemaphore = (sem != NULL ? (SemaphoreHandle_t)*sem : NULL);
  int32_t stat;
  BaseType_t yield;

  stat = 0;

  if (hSemaphore == NULL) {
    stat = (-EINVAL);
  } else if (alt_osal_irq_context() != 0U) {
    if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
      stat = (-EINVAL);
    } else {
      yield = pdFALSE;

      if (xSemaphoreTakeFromISR(hSemaphore, &yield) != pdPASS) {
        stat = (-EBUSY);
      } else {
        portYIELD_FROM_ISR(yield);
      }
    }
  } else {
    if (xSemaphoreTake(hSemaphore, (timeout_ms == ALT_OSAL_TIMEO_FEVR
                                        ? portMAX_DELAY
                                        : ((TickType_t)timeout_ms / portTICK_PERIOD_MS))) !=
        pdPASS) {
      if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
        stat = (-ETIME);
      } else {
        stat = (-EBUSY);
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_post_semaphore(FAR alt_osal_semaphore_handle *sem)
 is from
 cmsis_os2.c -> osStatus_t osSemaphoreRelease (osSemaphoreId_t semaphore_id)
*/
/* clang-format on */
int32_t alt_osal_post_semaphore(FAR alt_osal_semaphore_handle *sem) {
  SemaphoreHandle_t hSemaphore = (sem != NULL ? (SemaphoreHandle_t)*sem : NULL);
  int32_t stat;
  BaseType_t yield;

  stat = 0;

  if (hSemaphore == NULL) {
    stat = (-EINVAL);
  } else if (alt_osal_irq_context() != 0U) {
    yield = pdFALSE;

    if (xSemaphoreGiveFromISR(hSemaphore, &yield) != pdTRUE) {
      stat = (-EBUSY);
    } else {
      portYIELD_FROM_ISR(yield);
    }
  } else {
    if (xSemaphoreGive(hSemaphore) != pdPASS) {
      stat = (-EBUSY);
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_create_mutex(FAR alt_osal_mutex_handle *mutex, FAR const alt_osal_mutex_attribute *attr)
 is from
 cmsis_os2.c -> osMutexId_t osMutexNew (const osMutexAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_mutex(FAR alt_osal_mutex_handle *mutex,
                              FAR const alt_osal_mutex_attribute *attr) {
  SemaphoreHandle_t hMutex;
  uint32_t type;
  uint32_t rmtx;
  int32_t mem;
#if (configQUEUE_REGISTRY_SIZE > 0)
  const char *name;
#endif

  if (mutex == NULL) {
    return (-EINVAL);
  }

  hMutex = NULL;

  if (alt_osal_irq_context() == 0U) {
    if (attr != NULL) {
      type = attr->attr_bits;
    } else {
      type = 0U;
    }

    if ((type & ALT_OSAL_MUTEX_RECURSIVE) == ALT_OSAL_MUTEX_RECURSIVE) {
      rmtx = 1U;
    } else {
      rmtx = 0U;
    }

    mem = -1;

    if (attr != NULL) {
      if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_MUTEX_CBSIZE)) {
        /* The memory for control block is provided, use static object */
        mem = 1;
      } else {
        if ((attr->cb_mem == NULL) && (attr->cb_size == 0U)) {
          /* Control block will be allocated from the dynamic pool */
          mem = 0;
        }
      }
    } else {
      mem = 0;
    }

    if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
      if (rmtx != 0U) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
        hMutex = xSemaphoreCreateRecursiveMutexStatic(attr->cb_mem);
#endif
      } else {
        hMutex = xSemaphoreCreateMutexStatic(attr->cb_mem);
      }
#endif
    } else {
      if (mem == 0) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
        if (rmtx != 0U) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
          hMutex = xSemaphoreCreateRecursiveMutex();
#endif
        } else {
          hMutex = xSemaphoreCreateMutex();
        }
#endif
      }
    }

#if (configQUEUE_REGISTRY_SIZE > 0)
    if (hMutex != NULL) {
      if (attr != NULL) {
        name = attr->name;
      } else {
        name = NULL;
      }
      vQueueAddToRegistry(hMutex, name);
    }
#endif

    if ((hMutex != NULL) && (rmtx != 0U)) {
      /* Set LSB as 'recursive mutex flag' */
      hMutex = (SemaphoreHandle_t)((uint32_t)hMutex | 1U);
    }
  } else {
    return (-EPERM);
  }

  if (hMutex != NULL) {
    *mutex = (alt_osal_mutex_handle)hMutex;
  } else {
    return (-ENOMEM);
  }

  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_mutex(FAR alt_osal_mutex_handle *mutex)
 is from
 cmsis_os2.c -> osStatus_t osMutexDelete (osMutexId_t mutex_id)
*/
/* clang-format on */
int32_t alt_osal_delete_mutex(FAR alt_osal_mutex_handle *mutex) {
  int32_t stat;
  SemaphoreHandle_t hMutex;

  hMutex = (mutex != NULL ? ((SemaphoreHandle_t)((uint32_t)*mutex & ~1U)) : NULL);

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hMutex == NULL) {
    stat = (-EINVAL);
  } else {
#if (configQUEUE_REGISTRY_SIZE > 0)
    vQueueUnregisterQueue(hMutex);
#endif
    stat = 0;
    vSemaphoreDelete(hMutex);
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_lock_mutex(FAR alt_osal_mutex_handle *mutex, int32_t timeout_ms)
 is from
 cmsis_os2.c -> osStatus_t osMutexAcquire (osMutexId_t mutex_id, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_lock_mutex(FAR alt_osal_mutex_handle *mutex, int32_t timeout_ms) {
  SemaphoreHandle_t hMutex;
  int32_t stat;
  uint32_t rmtx;
  BaseType_t yield;

  hMutex = (mutex != NULL ? (SemaphoreHandle_t)((uint32_t)*mutex & ~1U) : NULL);

  /* Extract recursive mutex flag */
  rmtx = (mutex != NULL ? (uint32_t)*mutex & 1U : 0);

  stat = 0;

  if (alt_osal_irq_context() != 0U) {
    if (xSemaphoreTakeFromISR(hMutex, &yield) == pdTRUE) {
      portYIELD_FROM_ISR(yield);
      stat = 0;
    } else {
      stat = (-EPERM);
    }
  } else if (hMutex == NULL) {
    stat = (-EINVAL);
  } else {
    if (rmtx != 0U) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (xSemaphoreTakeRecursive(hMutex, (timeout_ms == ALT_OSAL_TIMEO_FEVR
                                               ? portMAX_DELAY
                                               : ((TickType_t)timeout_ms / portTICK_PERIOD_MS))) !=
          pdPASS) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
#endif
    } else {
      if (xSemaphoreTake(hMutex, (timeout_ms == ALT_OSAL_TIMEO_FEVR
                                      ? portMAX_DELAY
                                      : ((TickType_t)timeout_ms / portTICK_PERIOD_MS))) != pdPASS) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_unlock_mutex(FAR alt_osal_mutex_handle *mutex)
 is from
 cmsis_os2.c -> osStatus_t osMutexRelease (osMutexId_t mutex_id)
*/
/* clang-format on */
int32_t alt_osal_unlock_mutex(FAR alt_osal_mutex_handle *mutex) {
  SemaphoreHandle_t hMutex;
  int32_t stat;
  uint32_t rmtx;
  BaseType_t yield;

  hMutex = (mutex != NULL ? (SemaphoreHandle_t)((uint32_t)*mutex & ~1U) : NULL);

  /* Extract recursive mutex flag */
  rmtx = (mutex != NULL ? (uint32_t)*mutex & 1U : 0);

  stat = 0;

  if (alt_osal_irq_context() != 0U) {
    if (xSemaphoreGiveFromISR(hMutex, &yield) == pdTRUE) {
      portYIELD_FROM_ISR(yield);
      stat = 0;
    } else {
      stat = (-EPERM);
    }
  } else if (hMutex == NULL) {
    stat = (-EINVAL);
  } else {
    if (rmtx != 0U) {
#if (configUSE_RECURSIVE_MUTEXES == 1)
      if (xSemaphoreGiveRecursive(hMutex) != pdPASS) {
        stat = (-EBUSY);
      }
#endif
    } else {
      if (xSemaphoreGive(hMutex) != pdPASS) {
        stat = (-EBUSY);
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_create_mqueue(FAR alt_osal_queue_handle *mq, FAR const alt_osal_queue_attribute *attr)
 is from
 cmsis_os2.c -> osMessageQueueId_t osMessageQueueNew (uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_mqueue(FAR alt_osal_queue_handle *mq,
                               FAR const alt_osal_queue_attribute *attr) {
  QueueHandle_t hQueue;
  int32_t mem;

  if (mq == NULL || attr == NULL) {
    return (-EINVAL);
  }

  hQueue = NULL;

  if ((alt_osal_irq_context() == 0U) && (attr->numof_queue > 0U) && (attr->queue_size > 0U)) {
    mem = -1;

    if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_QUEUE_CBSIZE) &&
        (attr->mq_mem != NULL) && (attr->mq_size >= (attr->numof_queue * attr->queue_size))) {
      /* The memory for control block and message data is provided, use static object */
      mem = 1;
    } else {
      if ((attr->cb_mem == NULL) && (attr->cb_size == 0U) && (attr->mq_mem == NULL) &&
          (attr->mq_size == 0U)) {
        /* Control block will be allocated from the dynamic pool */
        mem = 0;
      }
    }

    if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
      hQueue = xQueueCreateStatic(attr->numof_queue, attr->queue_size, attr->mq_mem, attr->cb_mem);
#endif
    } else {
      if (mem == 0) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
        hQueue = xQueueCreate(attr->numof_queue, attr->queue_size);
#endif
      } else {
        return (-EINVAL);
      }
    }

#if (configQUEUE_REGISTRY_SIZE > 0)
    if (hQueue != NULL) {
      vQueueAddToRegistry(hQueue, attr->name);
    }
#endif
  } else {
    return (-EPERM);
  }

  if (hQueue != NULL) {
    *mq = (alt_osal_queue_handle)hQueue;
  } else {
    return (-ENOMEM);
  }

  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_mqueue(FAR alt_osal_queue_handle *mq)
 is from
 cmsis_os2.c -> osStatus_t osMessageQueueDelete (osMessageQueueId_t mq_id)
*/
/* clang-format on */
int32_t alt_osal_delete_mqueue(FAR alt_osal_queue_handle *mq) {
  QueueHandle_t hQueue = (mq != NULL ? (QueueHandle_t)*mq : NULL);
  int32_t stat;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hQueue == NULL) {
    stat = (-EINVAL);
  } else {
#if (configQUEUE_REGISTRY_SIZE > 0)
    vQueueUnregisterQueue(hQueue);
#endif

    stat = 0;
    vQueueDelete(hQueue);
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_send_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len, int32_t timeout_ms)
 is from
 cmsis_os2.c -> osStatus_t osMessageQueuePut (osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_send_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len,
                             int32_t timeout_ms) {
  QueueHandle_t hQueue = (mq != NULL ? (QueueHandle_t)*mq : NULL);
  int32_t stat;
  BaseType_t yield;

  (void)len;

  stat = 0;

  if (alt_osal_irq_context() != 0U) {
    if ((hQueue == NULL) || (msg_ptr == NULL) || (timeout_ms != ALT_OSAL_TIMEO_NOWAIT)) {
      stat = (-EINVAL);
    } else {
      yield = pdFALSE;

      if (xQueueSendToBackFromISR(hQueue, msg_ptr, &yield) != pdTRUE) {
        stat = (-EBUSY);
      } else {
        portYIELD_FROM_ISR(yield);
      }
    }
  } else {
    if ((hQueue == NULL) || (msg_ptr == NULL)) {
      stat = (-EINVAL);
    } else {
      if (xQueueSendToBack(hQueue, msg_ptr,
                           (timeout_ms == ALT_OSAL_TIMEO_FEVR
                                ? portMAX_DELAY
                                : ((TickType_t)timeout_ms / portTICK_PERIOD_MS))) != pdPASS) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_recv_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len, int32_t timeout_ms)
 is from
 cmsis_os2.c -> osStatus_t osMessageQueueGet (osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_recv_mqueue(FAR alt_osal_queue_handle *mq, FAR int8_t *msg_ptr, size_t len,
                             int32_t timeout_ms) {
  QueueHandle_t hQueue = (mq != NULL ? (QueueHandle_t)*mq : NULL);
  int stat;
  BaseType_t yield;

  stat = 0;

  if (alt_osal_irq_context() != 0U) {
    if ((hQueue == NULL) || (msg_ptr == NULL) || (timeout_ms != ALT_OSAL_TIMEO_NOWAIT)) {
      stat = (-EINVAL);
    } else {
      yield = pdFALSE;

      if (xQueueReceiveFromISR(hQueue, msg_ptr, &yield) != pdPASS) {
        stat = (-EBUSY);
      } else {
        portYIELD_FROM_ISR(yield);
      }
    }
  } else {
    if ((hQueue == NULL) || (msg_ptr == NULL)) {
      stat = (-EINVAL);
    } else {
      if (xQueueReceive(hQueue, msg_ptr,
                        (timeout_ms == ALT_OSAL_TIMEO_FEVR
                             ? portMAX_DELAY
                             : ((TickType_t)timeout_ms / portTICK_PERIOD_MS))) != pdPASS) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
    }
  }

  return stat == 0 ? ((int32_t)len) : (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_create_eventflag(FAR alt_osal_eventflag_handle *flag, FAR const alt_osal_eventflag_attribute *attr)
 is from
 cmsis_os2.c -> osEventFlagsId_t osEventFlagsNew (const osEventFlagsAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_eventflag(FAR alt_osal_eventflag_handle *flag,
                                  FAR const alt_osal_eventflag_attribute *attr) {
  EventGroupHandle_t hEventGroup;
  int32_t mem;

  if (flag == NULL) {
    return (-EINVAL);
  }

  hEventGroup = NULL;

  if (alt_osal_irq_context() == 0U) {
    mem = -1;

    if (attr != NULL) {
      if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_EVTFLAG_CBSIZE)) {
        /* The memory for control block is provided, use static object */
        mem = 1;
      } else {
        if ((attr->cb_mem == NULL) && (attr->cb_size == 0U)) {
          /* Control block will be allocated from the dynamic pool */
          mem = 0;
        }
      }
    } else {
      mem = 0;
    }

    if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
      hEventGroup = xEventGroupCreateStatic(attr->cb_mem);
#endif
    } else {
      if (mem == 0) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
        hEventGroup = xEventGroupCreate();
#endif
      }
    }
  } else {
    return (-EPERM);
  }

  if (hEventGroup != NULL) {
    *flag = (alt_osal_eventflag_handle)hEventGroup;
  } else {
    return (-ENOMEM);
  }

  /* Return event flags ID */
  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_eventflag(FAR alt_osal_eventflag_handle *flag)
 is from
 cmsis_os2.c -> osStatus_t osEventFlagsDelete (osEventFlagsId_t ef_id)
*/
/* clang-format on */
int32_t alt_osal_delete_eventflag(FAR alt_osal_eventflag_handle *flag) {
  EventGroupHandle_t hEventGroup = (flag != NULL ? (EventGroupHandle_t)*flag : NULL);
  int32_t stat;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hEventGroup == NULL) {
    stat = (-EINVAL);
  } else {
    stat = 0;
    vEventGroupDelete(hEventGroup);
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_wait_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits wptn, alt_osal_eventflag_mode wmode, bool autoclr, FAR alt_osal_event_bits *flagptn, int32_t timeout_ms)
 is from
 cmsis_os2.c -> uint32_t osEventFlagsWait (osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_wait_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits wptn,
                                alt_osal_eventflag_mode wmode, bool autoclr,
                                FAR alt_osal_event_bits *flagptn, int32_t timeout_ms) {
  EventGroupHandle_t hEventGroup = (flag != NULL ? (EventGroupHandle_t)*flag : NULL);
  BaseType_t wait_all;
  BaseType_t exit_clr;
  uint32_t rflags;
  int32_t stat;

  stat = 0;
  if ((hEventGroup == NULL) || ((wptn & EVENT_FLAGS_INVALID_BITS) != 0U) || (flagptn == NULL)) {
    stat = (-EINVAL);
  } else if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else {
    if (ALT_OSAL_WMODE_TWF_ANDW == wmode) {
      wait_all = pdTRUE;
    } else if (ALT_OSAL_WMODE_TWF_ORW == wmode) {
      wait_all = pdFALSE;
    } else {
      return (-EINVAL);
    }

    if (autoclr == false) {
      exit_clr = pdFAIL;
    } else {
      exit_clr = pdTRUE;
    }

    rflags = xEventGroupWaitBits(
        hEventGroup, (EventBits_t)wptn, exit_clr, wait_all,
        (timeout_ms == ALT_OSAL_TIMEO_FEVR ? portMAX_DELAY
                                           : ((TickType_t)timeout_ms / portTICK_PERIOD_MS)));
    *flagptn = rflags;
    if (ALT_OSAL_WMODE_TWF_ANDW == wmode) {
      if ((wptn & rflags) != wptn) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
    } else {
      if ((wptn & rflags) == 0U) {
        if (timeout_ms != ALT_OSAL_TIMEO_NOWAIT) {
          stat = (-ETIME);
        } else {
          stat = (-EBUSY);
        }
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_set_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits setptn)
 is from
 cmsis_os2.c -> uint32_t osEventFlagsSet (osEventFlagsId_t ef_id, uint32_t flags)
*/
/* clang-format on */
int32_t alt_osal_set_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits setptn) {
  EventGroupHandle_t hEventGroup = (flag != NULL ? (EventGroupHandle_t)*flag : NULL);
  int32_t stat;
  BaseType_t yield;

  stat = 0;
  if ((hEventGroup == NULL) || ((setptn & EVENT_FLAGS_INVALID_BITS) != 0U)) {
    stat = (-EINVAL);
  } else if (alt_osal_irq_context() != 0U) {
    yield = pdFALSE;

    if (xEventGroupSetBitsFromISR(hEventGroup, (EventBits_t)setptn, &yield) == pdFAIL) {
      stat = (-EBUSY);
    } else {
      portYIELD_FROM_ISR(yield);
    }
  } else {
    xEventGroupSetBits(hEventGroup, (EventBits_t)setptn);
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_clear_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits clrptn)
 is from
 cmsis_os2.c -> uint32_t osEventFlagsClear (osEventFlagsId_t ef_id, uint32_t flags)
*/
/* clang-format on */
int32_t alt_osal_clear_eventflag(FAR alt_osal_eventflag_handle *flag, alt_osal_event_bits clrptn) {
  EventGroupHandle_t hEventGroup = (flag != NULL ? (EventGroupHandle_t)*flag : NULL);
  int32_t stat;

  stat = 0;
  if ((hEventGroup == NULL) || ((clrptn & EVENT_FLAGS_INVALID_BITS) != 0U)) {
    stat = (-EINVAL);
  } else if (alt_osal_irq_context() != 0U) {
    if (xEventGroupClearBitsFromISR(hEventGroup, (EventBits_t)clrptn) == pdFAIL) {
      stat = (-EBUSY);
    } else {
      /* xEventGroupClearBitsFromISR only registers clear operation in the timer command queue. */
      /* Yield is required here otherwise clear operation might not execute in the right order. */
      /* See https://github.com/FreeRTOS/FreeRTOS-Kernel/issues/93 for more info.               */
      portYIELD_FROM_ISR(pdTRUE);
    }
  } else {
    xEventGroupClearBits(hEventGroup, (EventBits_t)clrptn);
  }

  /* Return event flags before clearing */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_create_timer(FAR alt_osal_timer_handle *timer, bool autoreload, CODE alt_osal_timer_cb_t callback, void *argument, alt_osal_timer_attribute *attr)
 is from
 cmsis_os2.c -> osTimerId_t osTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
*/
/* clang-format on */
int32_t alt_osal_create_timer(FAR alt_osal_timer_handle *timer, bool autoreload,
                              CODE alt_osal_timer_cb_t callback, void *argument,
                              alt_osal_timer_attribute *attr) {
  const char *name;
  TimerHandle_t hTimer;
  timer_cb *callb;
  UBaseType_t reload;
  int32_t mem;

  if (!timer || !callback) {
    return -EINVAL;
  }

  hTimer = NULL;

  if (alt_osal_irq_context() == 0U) {
    /* Allocate memory to store callback function and argument */
    callb = ALT_OSAL_MALLOC(sizeof(timer_cb));

    if (callb != NULL) {
      callb->func = callback;
      callb->arg = argument;

      reload = autoreload == false ? pdFALSE : pdTRUE;

      mem = -1;
      name = NULL;

      if (attr != NULL) {
        if (attr->name != NULL) {
          name = attr->name;
        }

        if ((attr->cb_mem != NULL) && (attr->cb_size >= MIN_STATIC_TIMER_CBSIZE)) {
          /* The memory for control block is provided, use static object */
          mem = 1;
        } else {
          if ((attr->cb_mem == NULL) && (attr->cb_size == 0U)) {
            /* Control block will be allocated from the dynamic pool */
            mem = 0;
          }
        }
      } else {
        mem = 0;
      }
      /*
        TimerCallback function is always provided as a callback and is used to call application
        specified function with its argument both stored in structure callb.
      */
      if (mem == 1) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
        hTimer = xTimerCreateStatic(name, 1, reload, callb, alt_osal_timer_callback,
                                    (StaticTimer_t *)attr->cb_mem);
#endif
      } else {
        if (mem == 0) {
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
          hTimer = xTimerCreate(name, 1, reload, callb, alt_osal_timer_callback);
#endif
        }
      }

      if ((hTimer == NULL) && (callb != NULL)) {
        /* Failed to create a timer, release allocated resources */
        ALT_OSAL_FREE(callb);
      }
    }
  } else {
    return (-EPERM);
  }

  if (hTimer != NULL) {
    *timer = (alt_osal_timer_handle)hTimer;
  } else {
    return (-ENOMEM);
  }

  return (0);
}

/* clang-format off */
/*
 int32_t alt_osal_start_timer(FAR alt_osal_timer_handle *timer, uint32_t period_ms)
 is from
 cmsis_os2.c -> osStatus_t osTimerStart (osTimerId_t timer_id, uint32_t ticks)
*/
/* clang-format on */
int32_t alt_osal_start_timer(FAR alt_osal_timer_handle *timer, uint32_t period_ms) {
  TimerHandle_t hTimer = (timer != NULL ? (TimerHandle_t)*timer : NULL);
  int32_t stat;
  BaseType_t yield;

  if (alt_osal_irq_context() != 0U) {
    if (xTimerChangePeriodFromISR(hTimer, period_ms / portTICK_PERIOD_MS, &yield) == pdPASS) {
      portYIELD_FROM_ISR(yield);
      stat = 0;
    } else {
      stat = (-EPERM);
    }
  } else if (hTimer == NULL) {
    stat = (-EINVAL);
  } else {
    if (xTimerChangePeriod(hTimer, period_ms / portTICK_PERIOD_MS, 0) == pdPASS) {
      stat = 0;
    } else {
      stat = (-EBUSY);
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_stop_timer(FAR alt_osal_timer_handle *timer)
 is from
 cmsis_os2.c -> osStatus_t osTimerStop (osTimerId_t timer_id)
*/
/* clang-format on */
int32_t alt_osal_stop_timer(FAR alt_osal_timer_handle *timer) {
  TimerHandle_t hTimer = (timer != NULL ? (TimerHandle_t)*timer : NULL);
  int32_t stat;
  BaseType_t yield;

  if (alt_osal_irq_context() != 0U) {
    if (xTimerStopFromISR(hTimer, &yield) == pdPASS) {
      portYIELD_FROM_ISR(yield);
      stat = 0;
    } else {
      stat = (-EPERM);
    }
  } else if (hTimer == NULL) {
    stat = (-EINVAL);
  } else {
    if (xTimerIsTimerActive(hTimer) == pdFALSE) {
      stat = (-EBUSY);
    } else {
      if (xTimerStop(hTimer, 0) == pdPASS) {
        stat = 0;
      } else {
        stat = (-EBUSY);
      }
    }
  }

  /* Return execution status */
  return (stat);
}

/* clang-format off */
/*
 int32_t alt_osal_delete_timer(FAR alt_osal_timer_handle *timer)
 is from
 cmsis_os2.c -> osStatus_t osTimerDelete (osTimerId_t timer_id)
*/
/* clang-format on */
int32_t alt_osal_delete_timer(FAR alt_osal_timer_handle *timer) {
  TimerHandle_t hTimer = (timer != NULL ? (TimerHandle_t)*timer : NULL);
  int32_t stat;
  timer_cb *callb;

  if (alt_osal_irq_context() != 0U) {
    stat = (-EPERM);
  } else if (hTimer == NULL) {
    stat = (-EINVAL);
  } else {
    callb = (timer_cb *)pvTimerGetTimerID(hTimer);

    if (xTimerDelete(hTimer, 0) == pdPASS) {
      ALT_OSAL_FREE(callb);
      stat = 0;
    } else {
      stat = (-EBUSY);
    }
  }

  /* Return execution status */
  return (stat);
}

int32_t alt_osal_thread_cond_init(FAR alt_osal_thread_cond_handle *condition,
                                  FAR alt_osal_thread_cond_attribute *cond_attr) {
  alt_osal_condition_variable *cond, **cond_ptr;
  int ret = 0;

  cond = (alt_osal_condition_variable *)ALT_OSAL_MALLOC(sizeof(alt_osal_condition_variable));
  if (cond) {
    if (alt_osal_create_mutex(&(cond->cond_mutex), NULL) < 0) {
      ALT_OSAL_FREE(cond);
      ret = -ENOMEM;
    } else {
      alt_osal_queue_attribute que_param;

      memset((void *)&que_param, 0, sizeof(alt_osal_queue_attribute));
      que_param.numof_queue = MAX_TASKS_WAITING_ON_COND_VAR;
      que_param.queue_size = sizeof(void *);
      if (alt_osal_create_mqueue(&(cond->evtQue), &que_param) < 0) {
        alt_osal_delete_mutex(&(cond->cond_mutex));
        ALT_OSAL_FREE(cond);
        ret = -ENOMEM;
      } else {
        cond->num_waiting = 0;
        cond_ptr = (alt_osal_condition_variable **)condition;
        *cond_ptr = cond;
      }
    }
  } else {
    ret = -ENOMEM;
  }

  return ret;
}

int32_t alt_osal_thread_cond_destroy(FAR alt_osal_thread_cond_handle *condition) {
  alt_osal_condition_variable **cond = (alt_osal_condition_variable **)condition;

  if (cond && *cond) {
    alt_osal_delete_mutex(&((*cond)->cond_mutex));
    alt_osal_delete_mqueue(&((*cond)->evtQue));
    ALT_OSAL_FREE(*cond);
    return 0;
  } else {
    return -EINVAL;
  }
}

static int __alt_osal_thread_cond_wait(alt_osal_thread_cond_handle *condition,
                                       alt_osal_mutex_handle *mutex, int ticks) {
  alt_osal_condition_variable **cond = (alt_osal_condition_variable **)condition;
  int ret = 0;
  int32_t qRet;
  void *evt;

  if (cond && *cond && mutex) {
    alt_osal_lock_mutex(&((*cond)->cond_mutex), ALT_OSAL_TIMEO_FEVR);
    (*cond)->num_waiting++;
    alt_osal_unlock_mutex(&((*cond)->cond_mutex));
    alt_osal_unlock_mutex(mutex);
    qRet = alt_osal_recv_mqueue(
        &((*cond)->evtQue), (int8_t *)(&evt), sizeof(void *),
        ticks > 0 ? (int32_t)(ticks * portTICK_PERIOD_MS) : (int32_t)ALT_OSAL_TIMEO_FEVR);
    alt_osal_lock_mutex(mutex, ALT_OSAL_TIMEO_FEVR);
    if (-ETIME == qRet) {
      ret = -ETIMEDOUT;
    } else {
      alt_osal_assert(evt == (*cond)->cond_mutex);
    }

    alt_osal_lock_mutex(&((*cond)->cond_mutex), ALT_OSAL_TIMEO_FEVR);
    (*cond)->num_waiting--;
    alt_osal_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

int32_t alt_osal_thread_cond_wait(FAR alt_osal_thread_cond_handle *condition,
                                  FAR alt_osal_mutex_handle *mutex) {
  return __alt_osal_thread_cond_wait(condition, mutex, 0);
}

int32_t alt_osal_thread_cond_timedwait(FAR alt_osal_thread_cond_handle *condition,
                                       FAR alt_osal_mutex_handle *mutex, int32_t timeout_ms) {
  int32_t delay;
  delay = timeout_ms / portTICK_PERIOD_MS;
  return __alt_osal_thread_cond_wait(condition, mutex, delay);
}

int32_t alt_osal_signal_thread_cond(FAR alt_osal_thread_cond_handle *condition) {
  int ret = 0;
  alt_osal_condition_variable **cond = (alt_osal_condition_variable **)condition;

  if (cond && *cond) {
    alt_osal_lock_mutex(&((*cond)->cond_mutex), ALT_OSAL_TIMEO_FEVR);
    if ((*cond)->num_waiting) {
      alt_osal_send_mqueue(&((*cond)->evtQue), (int8_t *)(&(*cond)->cond_mutex), sizeof(void *),
                           ALT_OSAL_TIMEO_FEVR);
    }

    alt_osal_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

int32_t alt_osal_broadcast_thread_cond(FAR alt_osal_thread_cond_handle *condition) {
  int ret = 0;
  int i;
  alt_osal_condition_variable **cond = (alt_osal_condition_variable **)condition;

  if (cond && *cond) {
    alt_osal_lock_mutex(&((*cond)->cond_mutex), ALT_OSAL_TIMEO_FEVR);
    for (i = 0; i < (*cond)->num_waiting; i++) {
      alt_osal_send_mqueue(&((*cond)->evtQue), (int8_t *)(&(*cond)->cond_mutex), sizeof(void *),
                           ALT_OSAL_TIMEO_FEVR);
    }

    alt_osal_unlock_mutex(&((*cond)->cond_mutex));
  } else {
    ret = -EINVAL;
  }

  return ret;
}

void alt_osal_task_yield(void) { taskYIELD(); }

/* clang-format off */
/*
 int32_t alt_osal_clear_taskflag(uint32_t flags)
 is from
 cmsis_os2.c -> uint32_t osThreadFlagsClear (uint32_t flags)
*/
/* clang-format on */
int32_t alt_osal_clear_taskflag(uint32_t flags) {
  alt_osal_task_handle hTask;
  uint32_t rflags, cflags;

  if (alt_osal_irq_context() != 0U) {
    rflags = -EPERM;
  } else if ((flags & TASK_FLAGS_INVALID_BITS) != 0U) {
    rflags = -EINVAL;
  } else {
    hTask = xTaskGetCurrentTaskHandle();

    if (xTaskNotifyAndQueryIndexed(hTask, ALT_OSAL_TASK_FLAG_INDEX, 0, eNoAction, &cflags) ==
        pdPASS) {
      rflags = cflags;
      cflags &= ~flags;

      if (xTaskNotifyIndexed(hTask, ALT_OSAL_TASK_FLAG_INDEX, cflags, eSetValueWithOverwrite) !=
          pdPASS) {
        rflags = -EBUSY;
      }
    } else {
      rflags = -EBUSY;
    }
  }

  /* Return flags before clearing */
  return (rflags);
}

/* clang-format off */
/*
 int32_t alt_osal_set_taskflag(FAR alt_osal_task_handle handle, uint32_t flags)
 is from
 cmsis_os2.c -> uint32_t osThreadFlagsSet (osThreadId_t thread_id, uint32_t flags)
*/
/* clang-format on */
int32_t alt_osal_set_taskflag(FAR alt_osal_task_handle handle, uint32_t flags) {
  alt_osal_task_handle hTask = (alt_osal_task_handle)handle;
  uint32_t rflags;
  BaseType_t yield;

  if ((hTask == NULL) || ((flags & TASK_FLAGS_INVALID_BITS) != 0U)) {
    rflags = -EINVAL;
  } else {
    if (alt_osal_irq_context() != 0U) {
      yield = pdFALSE;

      (void)xTaskNotifyIndexedFromISR(hTask, ALT_OSAL_TASK_FLAG_INDEX, flags, eSetBits, &yield);
      (void)xTaskNotifyAndQueryIndexedFromISR(hTask, ALT_OSAL_TASK_FLAG_INDEX, 0, eNoAction,
                                              &rflags, NULL);

      portYIELD_FROM_ISR(yield);
    } else {
      (void)xTaskNotifyIndexed(hTask, ALT_OSAL_TASK_FLAG_INDEX, flags, eSetBits);
      (void)xTaskNotifyAndQueryIndexed(hTask, ALT_OSAL_TASK_FLAG_INDEX, 0, eNoAction, &rflags);
    }
  }
  /* Return flags after setting */
  return (rflags);
}

/* clang-format off */
/*
 int32_t alt_osal_wait_taskflag(uint32_t flags, uint32_t options, uint32_t timeout)
 is from
 cmsis_os2.c -> uint32_t osThreadFlagsWait (uint32_t flags, uint32_t options, uint32_t timeout)
*/
/* clang-format on */
int32_t alt_osal_wait_taskflag(uint32_t flags, uint32_t options, uint32_t timeout) {
  uint32_t rflags, nval;
  uint32_t clear;
  TickType_t t0, td, tout;
  BaseType_t rval;

  if (alt_osal_irq_context() != 0U) {
    rflags = -EPERM;
  } else if ((flags & TASK_FLAGS_INVALID_BITS) != 0U) {
    rflags = -EINVAL;
  } else {
    if ((options & ALT_OSAL_FLAG_NO_CLEAR) == ALT_OSAL_FLAG_NO_CLEAR) {
      clear = 0U;
    } else {
      clear = flags;
    }

    rflags = 0U;
    tout = timeout;

    t0 = xTaskGetTickCount();
    do {
      rval = xTaskNotifyWaitIndexed(ALT_OSAL_TASK_FLAG_INDEX, 0, clear, &nval, tout);

      if (rval == pdPASS) {
        rflags &= flags;
        rflags |= nval;

        if ((options & ALT_OSAL_WMODE_TWF_ANDW) == ALT_OSAL_WMODE_TWF_ANDW) {
          if ((flags & rflags) == flags) {
            break;
          } else {
            if (timeout == 0U) {
              rflags = -EBUSY;
              break;
            }
          }
        } else {
          if ((flags & rflags) != 0) {
            break;
          } else {
            if (timeout == 0U) {
              rflags = -EBUSY;
              break;
            }
          }
        }

        /* Update timeout */
        td = xTaskGetTickCount() - t0;

        if (td > tout) {
          tout = 0;
        } else {
          tout -= td;
        }
      } else {
        if (timeout == 0) {
          rflags = -EBUSY;
        } else {
          rflags = -ETIME;
        }
      }
    } while (rval != pdFAIL);
  }

  /* Return flags before clearing */
  return (rflags);
}

/* clang-format off */
/*
 alt_osal_task_handle alt_osal_get_current_task_handle(void)
 is from
 cmsis_os2.c -> osThreadId_t osThreadGetId (void)
*/
/* clang-format on */
alt_osal_task_handle alt_osal_get_current_task_handle(void) {
  alt_osal_task_handle id;

  id = (alt_osal_task_handle)xTaskGetCurrentTaskHandle();

  /* Return thread ID */
  return (id);
}

void alt_osal_enter_critital_section(void) { taskENTER_CRITICAL(); }

void alt_osal_exit_critital_section(void) { taskEXIT_CRITICAL(); }

/* clang-format off */
/*
 uint32_t alt_osal_get_tick_count(void)
 is from
 cmsis_os2.c -> uint32_t osKernelGetTickCount (void)
*/
/* clang-format on */
uint32_t alt_osal_get_tick_count(void) {
  TickType_t ticks;

  if (alt_osal_irq_context() != 0U) {
    ticks = xTaskGetTickCountFromISR();
  } else {
    ticks = xTaskGetTickCount();
  }

  /* Return kernel tick count */
  return (ticks);
}

/* clang-format off */
/*
 uint32_t alt_osal_get_tick_freq(void)
 is from
 cmsis_os2.c -> uint32_t osKernelGetTickFreq (void)
*/
/* clang-format on */
uint32_t alt_osal_get_tick_freq(void) { return (configTICK_RATE_HZ); }

uint32_t alt_osal_enter_critical(void) {
  unsigned int status = 0;

  if (xPortIsInsideInterrupt() == pdTRUE) {
    status = taskENTER_CRITICAL_FROM_ISR();
  } else {
    taskENTER_CRITICAL();
  }

  return status;
}

int32_t alt_osal_exit_critical(uint32_t status) {
  if (xPortIsInsideInterrupt() == pdTRUE) {
    taskEXIT_CRITICAL_FROM_ISR(status);
  } else {
    taskEXIT_CRITICAL();
  }

  return 0;
}

bool alt_osal_is_inside_interrupt(void) {
  if (xPortIsInsideInterrupt() == pdTRUE) {
    return true;
  } else {
    return false;
  }
}

uint32_t alt_osal_get_tasks_status(alt_osal_task_status *const task_array, size_t array_size) {
  uint32_t ret, i;
  TaskStatus_t task_status[array_size];

  ret = uxTaskGetSystemState(task_status, array_size, NULL);
  if (ret != 0) {
    for (i = 0; i < ret; i++) {
      task_array[i].handle = (void *)task_status[i].xHandle;
      task_array[i].name = task_status[i].pcTaskName;
      task_array[i].id = (uint32_t)task_status[i].xTaskNumber;
      task_array[i].state = (alt_osal_task_state)task_status[i].eCurrentState;
      task_array[i].priority = (alt_osal_task_priority)task_status[i].uxCurrentPriority;
      task_array[i].stack_base = (alt_osal_stack_type *)task_status[i].pxStackBase;
      task_array[i].stack_size = (size_t)task_status[i].ulStackSize;
      task_array[i].stack_usage = (size_t)task_status[i].ulStackCur;
    }
  }

  return ret;
}