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

#ifndef OSAL_TIMERS_H
#define OSAL_TIMERS_H

/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

#include "osal/osal_stateMachine.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef enum { osalTimer_event, osalTimer_callback } osalTimer_type_t;

typedef struct osalTimer {
  struct osalTimer *next;
  struct osalTimer *prev;

  osalTimer_type_t type;
  osTimerId_t xTimer;
  stateMachine_t *pCtx;
  stateMachineEvt_t event;
} osalTimer_t;

typedef void (*osalTimerCallback_t)(void *cbData);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int32_t osal_TimerStart(osalTimer_t *pTimer, uint32_t blockTimeMs);
int32_t osal_TimerStop(osalTimer_t *pTimer, uint32_t blockTimeMs);
osalTimer_t *osal_eventTimerCreate(stateMachine_t *pCtx, const char *const timerName,
                                   uint32_t timeoutMs, stateMachineEvt_t *event);
osalTimer_t *osal_callbackTimerCreate(stateMachine_t *pCtx, const char *const timerName,
                                      uint32_t timeoutMs, osalTimerCallback_t callback,
                                      void *cbData);
int32_t osal_TimerDelete(osalTimer_t *ptimer);
const char *osal_TimerGetName(osalTimer_t *ptimer);

#endif /* OSAL_TIMERS_H */
