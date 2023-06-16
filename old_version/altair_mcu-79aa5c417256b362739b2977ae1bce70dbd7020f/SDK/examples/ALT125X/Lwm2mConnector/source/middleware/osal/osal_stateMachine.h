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

#ifndef OSAL_STATEMACHINE_H
#define OSAL_STATEMACHINE_H

/* General Header */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Main loop */
#define SM_TASK_STACKSIZE (4096)
#define SM_TASK_PRI (osPriorityNormal)
#define SM_MAX_QUENUM 20

#define ALL_STATES 0xFFFFFFFF
#define ALL_EVENTS 0xFFFFFFFF

/* Event allocate & send helper */
#define EVT_SEND(ctx, eventId, param)                                                      \
  do {                                                                                     \
    SM_evtSend((stateMachine_t *)ctx, (stateMachineEvtId_t)eventId, (void *)param, false); \
  } while (0)

#define EVT_SEND_ISR(ctx, eventId, param)                                                 \
  do {                                                                                    \
    SM_evtSend((stateMachine_t *)ctx, (stateMachineEvtId_t)eventId, (void *)param, true); \
  } while (0)

/* Compilation */
#define APP_UNUSED(x) (void)(x)

/* Time related */
#if defined(__GNUC__)
#define GMTIME_R(time, tm) gmtime_r(time, tm)
#elif defined(__ICCARM__)
#define GMTIME_R(time, tm) gmtime_s(time, tm)
#else
#error time function not supported in this toolchain
#endif

typedef uint32_t state_t;

typedef uint32_t stateMachineEvtId_t;

typedef state_t (*stateTransFunc_t)(void *pCtx, void *pParam);

typedef void (*stateChangedNotificationFunc_t)(state_t state, void *pParam);

typedef void (*stateEventNotificationFunc_t)(state_t state, stateMachineEvtId_t event,
                                             void *pParam);

struct osalTimer;

typedef struct {
  state_t currState;
  stateMachineEvtId_t currEventId;
  osMessageQueueId_t evtQueue;
  osThreadId_t taskHandle;
  stateTransFunc_t **stateTransMatrix;

  stateTransFunc_t defaultEvtHandler;

  stateChangedNotificationFunc_t notifyStateChangedFunc;
  void *notifyStateChangedFunc_param;

  stateEventNotificationFunc_t notifyPreEventProccessingFunc;
  void *notifyPreEventProccessingFunc_param;

  stateEventNotificationFunc_t notifyPostEventProccessingFunc;
  void *notifyPostEventProccessingFunc_param;

  bool taskRunning;

  char *smName;
  char **stateNames;
  char **eventNames;

  bool debugPrintsEnable;

  uint32_t *stateStat;
  uint32_t *eventStat;

  uint32_t numOfStates;
  uint32_t numOfEvents;

  struct osalTimer *sm_timers;
} stateMachine_t;

typedef struct {
  stateMachineEvtId_t eventId;
  void *param;
} stateMachineEvt_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void SM_evtSend(stateMachine_t *pCtx, stateMachineEvtId_t eventId, void *pParam, bool isISR);

int SM_StateMachineInit(stateMachine_t *smCtx, char *smName, uint32_t numOfStates,
                        uint32_t numOfEvents, char **stateNames, char **eventNames);

int SM_StateMachineRegisterEvHandler(stateMachine_t *smCtx, uint32_t state, uint32_t eventId,
                                     stateTransFunc_t eventHandler);

int SM_StateMachineRegisterDefaultEvHandler(stateMachine_t *smCtx, stateTransFunc_t eventHandler);

int SM_StateMachineStart(stateMachine_t *smCtx, uint32_t startState, stateMachineEvt_t *startEv);

int SM_StateMachineSetDebugPrint(stateMachine_t *smCtx, int enable);

int SM_StateMachineRegisterNotifyStateChangedFunc(stateMachine_t *smCtx,
                                                  stateChangedNotificationFunc_t stateChangedFunc,
                                                  void *pParam);

int SM_StateMachineRegisterNotifyPreEventProccessingFunc(
    stateMachine_t *smCtx, stateEventNotificationFunc_t notifyEventFunc, void *pParam);

int SM_StateMachineRegisterNotifyPostEventProccessingFunc(
    stateMachine_t *smCtx, stateEventNotificationFunc_t notifyEventFunc, void *pParam);

void SM_printStateMachinesInfo();

void SM_printStateMachinesStats();

void SM_printStateMachinesStat(stateMachine_t *pCtx);

int SM_GetStatsBulk(uint32_t *sizeRet, char **retBulk);

#endif /* OSAL_STATEMACHINE_H */
