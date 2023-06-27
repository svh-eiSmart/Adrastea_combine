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

#include <FreeRTOS.h>
#include <event_groups.h>
#include "hifc_osal.h"
#include <stdio.h>
#include <timers.h>

#define TIMER_NAME_MAX (16)
#define TIMER_ID_DEFAULT (0)
#define TIMER_NAME_FMT "Timer%u"

int32_t hifc_osal_create_timer(hifc_sys_timer_t *timer, int32_t period_ms, bool autoreload,
                               hifc_sys_timer_cb_t callback) {
  static unsigned short timer_cnt = 0;
  char name[TIMER_NAME_MAX] = {0};

  if (!timer || (0 > period_ms) || !callback) {
    return -HIFC_EINVAL;
  }

  snprintf(name, TIMER_NAME_MAX, TIMER_NAME_FMT, timer_cnt);

  *timer = xTimerCreate(name, period_ms / portTICK_PERIOD_MS, (UBaseType_t)autoreload,
                        TIMER_ID_DEFAULT, callback);

  if (!*timer) {
    return -HIFC_ENOMEM;
  }
  timer_cnt++;
  return 0;
}

int32_t hifc_osal_start_timer(hifc_sys_timer_t *timer) {
  if (!*timer) {
    return -HIFC_EINVAL;
  }

  if (xTimerStart(*timer, portMAX_DELAY) != pdPASS) {
    return -HIFC_ETIME;
  }

  return 0;
}

int32_t hifc_osal_start_timer_fromISR(hifc_sys_timer_t *timer) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (!*timer) {
    return -HIFC_EINVAL;
  }

  if (xTimerStartFromISR(*timer, &xHigherPriorityTaskWoken) != pdPASS) {
    return -HIFC_ETIME;
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return 0;
}

int32_t hifc_osal_stop_timer(hifc_sys_timer_t *timer) {
  if (!timer) {
    return -HIFC_EINVAL;
  }

  if (pdPASS != xTimerStop(*timer, portMAX_DELAY)) {
    return -HIFC_ETIME;
  }

  return 0;
}

int32_t hifc_osal_stop_timer_fromISR(hifc_sys_timer_t *timer) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (!timer) {
    return -HIFC_EINVAL;
  }

  if (pdPASS != xTimerStopFromISR(*timer, &xHigherPriorityTaskWoken)) {
    return -HIFC_ETIME;
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return 0;
}

int32_t osal_reset_timer(hifc_sys_timer_t *timer) {
  if (!timer) {
    return -HIFC_EINVAL;
  }

  if (xTimerReset(*timer, 0) != pdPASS) {
    return -HIFC_ETIME;
  }

  return 0;
}

int32_t hifc_osal_reset_timer_fromISR(hifc_sys_timer_t *timer) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (!timer) {
    return -HIFC_EINVAL;
  }

  if (xTimerResetFromISR(*timer, &xHigherPriorityTaskWoken) != pdPASS) {
    return -HIFC_ETIME;
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return 0;
}

int32_t hifc_osal_change_timer_period_fromISR(hifc_sys_timer_t *timer, uint32_t period_in_ms) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (xTimerChangePeriodFromISR(*timer, period_in_ms / portTICK_PERIOD_MS,
                                &xHigherPriorityTaskWoken) != pdPASS) {
    return -HIFC_ETIME;
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  return 0;
}

uint32_t hifc_osal_get_timer_expire_time_fromISR(hifc_sys_timer_t *timer) {
  TickType_t xRemainingTime;

  xRemainingTime = (xTimerGetExpiryTime(timer) - xTaskGetTickCountFromISR()) *
                   portTICK_PERIOD_MS;  // conver tick to ms

  return xRemainingTime;
}

void hifc_osal_enter_critical(void) { taskENTER_CRITICAL(); }

void hifc_osal_exit_critical(void) { taskEXIT_CRITICAL(); }
