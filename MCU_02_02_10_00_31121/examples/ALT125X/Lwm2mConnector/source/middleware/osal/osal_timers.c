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

/****************************************************************************
 * Included Files
 ****************************************************************************/
/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

#include "osal/osal_timers.h"
#include "Infra/app_dbg.h"

#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

static void osal_timer_cb(void *arg) {
  osalTimer_t *timer = (osalTimer_t *)arg;

  EVT_SEND(timer->pCtx, timer->event.eventId, timer->event.param);
}

int32_t osal_TimerStart(osalTimer_t *pTimer, uint32_t blockTimeMs) {
  return (int32_t)osTimerStart(pTimer->xTimer, blockTimeMs * OS_TICK_PERIOD_MS);
}

int32_t osal_TimerStop(osalTimer_t *pTimer, uint32_t blockTimeMs) {
  return (int32_t)osTimerStart(pTimer->xTimer, blockTimeMs * OS_TICK_PERIOD_MS);
}

osalTimer_t *osal_eventTimerCreate(stateMachine_t *pCtx, const char *const timerName,
                                   uint32_t timeoutMs, stateMachineEvt_t *event) {
  osalTimer_t *timer;

  timer = malloc(sizeof(osalTimer_t));
  APP_ASSERT(NULL != timer, "malloc fail\r\n");

  timer->pCtx = pCtx;
  timer->type = osalTimer_event;
  timer->event = *event;

  osTimerAttr_t attr = {0};

  attr.name = timerName;
  timer->xTimer = osTimerNew((osTimerFunc_t)osal_timer_cb, osTimerOnce, (void *)timer, &attr);
  if (!timer->xTimer) {
    APP_DBG_PRINT("osTimerNew fail\r\n");
    free(timer);
    return NULL;
  }

  timer->next = pCtx->sm_timers;
  timer->prev = NULL;

  if (pCtx->sm_timers) {
    pCtx->sm_timers->prev = timer;
  }

  pCtx->sm_timers = timer;

  return timer;
}

osalTimer_t *osal_callbackTimerCreate(stateMachine_t *pCtx, const char *const timerName,
                                      uint32_t timeoutMs, osalTimerCallback_t callback,
                                      void *cbData) {
  osalTimer_t *timer;

  timer = malloc(sizeof(osalTimer_t));
  APP_ASSERT(NULL != timer, "malloc fail\r\n");

  timer->pCtx = pCtx;
  timer->type = osalTimer_callback;

  osTimerAttr_t attr = {0};

  attr.name = timerName;
  timer->xTimer = osTimerNew((osTimerFunc_t)callback, osTimerOnce, cbData, &attr);
  if (!timer->xTimer) {
    APP_DBG_PRINT("osTimerNew create fail\r\n");
    free(timer);
    return NULL;
  }

  timer->next = pCtx->sm_timers;
  timer->prev = NULL;

  if (pCtx->sm_timers) {
    pCtx->sm_timers->prev = timer;
  }

  pCtx->sm_timers = timer;

  return timer;
}

int32_t osal_TimerDelete(osalTimer_t *ptimer) {
  osStatus_t ret = osTimerDelete(ptimer->xTimer);

  if (ret != osOK) {
    APP_ERR_PRINT("xTimerDelete fail\r\n");
    return (int32_t)ret;
  }

  if (ptimer->prev) {
    ptimer->prev->next = ptimer->next;
  }

  if (ptimer->next) {
    ptimer->next->prev = ptimer->prev;
  }

  if (ptimer->pCtx->sm_timers == ptimer) {
    ptimer->pCtx->sm_timers = ptimer->next;
  }

  free(ptimer);

  ptimer = NULL;
  return 0;
}

const char *osal_TimerGetName(osalTimer_t *ptimer) { return osTimerGetName(ptimer->xTimer); }
