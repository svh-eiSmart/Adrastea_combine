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

/* Middleware Header */
#include <osal/osal_stateMachine.h>
#include <osal/osal_timers.h>
#include "Infra/app_dbg.h"

#define MAX_NUM_OF_SM 50

typedef struct {
  stateMachine_t *stateMachinesArray[MAX_NUM_OF_SM];
  int numOfActiveSMs;
} SM_data_t;

static SM_data_t gSMData = {0};

void SM_evtSend(stateMachine_t *pCtx, stateMachineEvtId_t eventId, void *pParam, bool isISR) {
  stateMachineEvt_t evt;

  evt.eventId = eventId;
  evt.param = pParam;
  APP_ASSERT(osOK == osMessageQueuePut(pCtx->evtQueue, (const void *)&evt, 0, osWaitForever),
             "event send fail\r\n");
}

void SM_printStateMachinesInfo() {
  int i;
  stateMachine_t *pCtx;
  APP_DBG_PRINT("%-25s%-30s%-8s%-8s\r\n", "StateMachine", "CurrState", "Running", "Debug");
  APP_DBG_PRINT("%-25s%-30s%-8s%-8s\r\n", "------------", "---------", "-------", "-----");
  for (i = 0; i < gSMData.numOfActiveSMs; i++) {
    pCtx = gSMData.stateMachinesArray[i];

    APP_DBG_PRINT("%-25s%-30s%-8d%-8d\r\n", pCtx->smName, pCtx->stateNames[pCtx->currState],
                  pCtx->taskRunning, pCtx->debugPrintsEnable);
  }
}

void SM_printStateMachinesStat(stateMachine_t *pCtx) {
  uint32_t j;

  APP_DBG_PRINT("StateMachine Stat: %s\r\n", pCtx->smName);
  APP_DBG_PRINT("------------------------------------------------------\r\n");
  for (j = 0; j < pCtx->numOfStates; j++) {
    APP_DBG_PRINT("State: %-35s count: %lu\r\n", pCtx->stateNames[j], pCtx->stateStat[j]);
  }

  for (j = 0; j < pCtx->numOfEvents; j++) {
    APP_DBG_PRINT("Event: %-35s count: %lu\r\n", pCtx->eventNames[j], pCtx->eventStat[j]);
  }
}

void SM_printStateMachinesStats() {
  int i;
  stateMachine_t *pCtx;
  for (i = 0; i < gSMData.numOfActiveSMs; i++) {
    pCtx = gSMData.stateMachinesArray[i];

    SM_printStateMachinesStat(pCtx);
  }
}

static void SM_StateMachineTask(void *arg) {
  stateMachine_t *ctx = (stateMachine_t *)arg;
  osStatus_t ret;
  stateMachineEvt_t evt;
  state_t currState;

  APP_TRACE();

  /* Main event loop */
  while (ctx->taskRunning) {
    /* Event receiving */
    ret = osMessageQueueGet(ctx->evtQueue, (void *)&evt, 0, osWaitForever);
    if (ret != osOK) {
      APP_DBG_PRINT("xQueueReceive fail, ret = %ld\r\n", (int32_t)ret);
      break;
    }

    ctx->eventStat[evt.eventId]++;

    currState = ctx->currState;
    /* Event handling and state transition */
    if (ctx->debugPrintsEnable) {
      APP_DBG_PRINT("Stat %s evt %s (inQ %d)\r\n", ctx->stateNames[ctx->currState],
                    ctx->eventNames[evt.eventId], uxQueueMessagesWaiting(ctx->evtQueue));
    }

    if (ctx->notifyPreEventProccessingFunc) {
      ctx->notifyPreEventProccessingFunc(ctx->currState, evt.eventId,
                                         ctx->notifyPreEventProccessingFunc_param);
    }

    ctx->currEventId = evt.eventId;

    if (ctx->stateTransMatrix[ctx->currState][evt.eventId]) {
      ctx->currState =
          ctx->stateTransMatrix[ctx->currState][evt.eventId]((void *)ctx, (void *)evt.param);
    } else {
      if (ctx->defaultEvtHandler) {
        ctx->currState = ctx->defaultEvtHandler((void *)ctx, (void *)evt.param);
      } else if (ctx->debugPrintsEnable) {
        APP_DBG_PRINT("evt %s ignored in Stat %s\r\n", ctx->eventNames[evt.eventId],
                      ctx->stateNames[ctx->currState]);
      }
    }

    /* Release event element */

    if (currState != ctx->currState) {
      ctx->stateStat[ctx->currState]++;

      if (ctx->debugPrintsEnable) {
        APP_DBG_PRINT("nextState: %s (inQ %d)\r\n", ctx->stateNames[(int)ctx->currState],
                      uxQueueMessagesWaiting(ctx->evtQueue));
      }
      if (ctx->notifyStateChangedFunc) {
        ctx->notifyStateChangedFunc(ctx->currState, ctx->notifyStateChangedFunc_param);
      }
    }

    if (ctx->notifyPostEventProccessingFunc) {
      ctx->notifyPostEventProccessingFunc(ctx->currState, evt.eventId,
                                          ctx->notifyPostEventProccessingFunc_param);
    }
  }

  APP_TRACE();

  /* Task finished */
  osThreadExit();
}

int SM_StateMachineInit(stateMachine_t *smCtx, char *smName, uint32_t numOfStates,
                        uint32_t numOfEvents, char **stateNames, char **eventNames) {
  uint32_t i;
  stateTransFunc_t *sPtr;

  APP_TRACE();

  gSMData.stateMachinesArray[gSMData.numOfActiveSMs] = smCtx;
  gSMData.numOfActiveSMs++;

  memset(smCtx, 0, sizeof(stateMachine_t));
  smCtx->stateNames = stateNames;
  smCtx->eventNames = eventNames;
  smCtx->smName = smName;
  smCtx->numOfEvents = numOfEvents;
  smCtx->numOfStates = numOfStates;

  smCtx->debugPrintsEnable = true;

  // init the sm timers ll
  smCtx->sm_timers = NULL;

  /* Initialize state transition table*/
  smCtx->stateTransMatrix = malloc(sizeof(stateTransFunc_t *) * numOfStates +
                                   sizeof(stateTransFunc_t) * numOfStates * numOfEvents);

  APP_ASSERT((smCtx->stateTransMatrix) != NULL, "stateTransMatrix malloc fail\r\n");

  sPtr = (stateTransFunc_t *)(smCtx->stateTransMatrix + numOfStates);
  for (i = 0; i < numOfStates; i++) smCtx->stateTransMatrix[i] = (sPtr + i * numOfEvents);

  memset(sPtr, 0, sizeof(stateTransFunc_t *) * numOfStates * numOfEvents);

  smCtx->stateStat = malloc(sizeof(uint32_t) * numOfStates);
  APP_ASSERT((smCtx->stateStat) != NULL, "stateStat malloc fail\r\n");

  memset(smCtx->stateStat, 0, sizeof(uint32_t) * numOfStates);

  smCtx->eventStat = malloc(sizeof(uint32_t) * numOfEvents);
  APP_ASSERT((smCtx->eventStat) != NULL, "eventStat malloc fail\r\n");

  memset(smCtx->eventStat, 0, sizeof(stateTransFunc_t *) * numOfEvents);

  /* Initialize misc */
  smCtx->taskRunning = true;
  smCtx->evtQueue = osMessageQueueNew(SM_MAX_QUENUM, sizeof(stateMachineEvt_t), NULL);
  if (!smCtx->evtQueue) {
    APP_DBG_PRINT("evtQueue create fail\r\n");
    return -1;
  }

  /*Create main loop task */
  osThreadAttr_t attr = {0};

  attr.name = (const char *)smName;
  attr.stack_size = SM_TASK_STACKSIZE;
  attr.priority = SM_TASK_PRI;
  smCtx->taskHandle = osThreadNew(SM_StateMachineTask, smCtx, &attr);
  if (!smCtx->taskHandle) {
    APP_DBG_PRINT("osThreadNew failed\n");
    return -2;
  }

  return 0;
}

int SM_StateMachineSetDebugPrint(stateMachine_t *smCtx, int enable) {
  smCtx->debugPrintsEnable = enable;
  return 0;
}

int SM_StateMachineRegisterDefaultEvHandler(stateMachine_t *smCtx, stateTransFunc_t eventHandler) {
  APP_TRACE();

  smCtx->defaultEvtHandler = eventHandler;
  APP_DBG_VERBOSE_PRINT("Registered default event handler\r\n");

  return 0;
}

int SM_StateMachineRegisterNotifyStateChangedFunc(stateMachine_t *smCtx,
                                                  stateChangedNotificationFunc_t stateChangedFunc,
                                                  void *pParam) {
  APP_TRACE();

  smCtx->notifyStateChangedFunc = stateChangedFunc;
  smCtx->notifyStateChangedFunc_param = pParam;
  APP_DBG_VERBOSE_PRINT("Registered NotifyStateChangedFunc\r\n");

  return 0;
}

int SM_StateMachineRegisterNotifyPostEventProccessingFunc(
    stateMachine_t *smCtx, stateEventNotificationFunc_t notifyEventFunc, void *pParam) {
  APP_TRACE();

  smCtx->notifyPostEventProccessingFunc = notifyEventFunc;
  smCtx->notifyPostEventProccessingFunc_param = pParam;
  APP_DBG_VERBOSE_PRINT("Registered NotifyPostEventProccessingFunc\r\n");

  return 0;
}

int SM_StateMachineRegisterNotifyPreEventProccessingFunc(
    stateMachine_t *smCtx, stateEventNotificationFunc_t notifyEventFunc, void *pParam) {
  APP_TRACE();

  smCtx->notifyPreEventProccessingFunc = notifyEventFunc;
  smCtx->notifyPreEventProccessingFunc_param = pParam;
  APP_DBG_VERBOSE_PRINT("Registered NotifyPreEventProccessingFunc\r\n");

  return 0;
}

int SM_StateMachineRegisterEvHandler(stateMachine_t *smCtx, uint32_t state, uint32_t eventId,
                                     stateTransFunc_t eventHandler) {
  int firstState, lastState, firstEvent, lastEvent;
  int i, j;
  APP_TRACE();

  if (state == ALL_STATES) {
    firstState = 0;
    lastState = smCtx->numOfStates - 1;
  } else {
    firstState = lastState = state;
  }

  if (eventId == ALL_EVENTS) {
    firstEvent = 0;
    lastEvent = smCtx->numOfEvents - 1;
  } else {
    firstEvent = lastEvent = eventId;
  }

  for (i = firstState; i <= lastState; i++) {
    for (j = firstEvent; j <= lastEvent; j++) {
      smCtx->stateTransMatrix[i][j] = eventHandler;
      APP_DBG_VERBOSE_PRINT("Reg stat %s evt %s\r\n", smCtx->stateNames[i], smCtx->eventNames[j]);
    }
  }

  return 0;
}

int SM_StateMachineStart(stateMachine_t *smCtx, uint32_t startState, stateMachineEvt_t *startEv) {
  APP_TRACE();

  smCtx->currState = startState;
  /*Send init event */
  EVT_SEND(smCtx, startEv->eventId, startEv->param);

  return 0;
}

int SM_GetStatsBulk(uint32_t *sizeRet, char **retBulk) {
  int i;
  uint32_t j;
  stateMachine_t *pCtx = NULL;
  uint32_t *pStatsData;

  // First get number of events and states for each SM to allocate buffer
  *sizeRet = 0;

  for (i = 0; i < gSMData.numOfActiveSMs; i++) {
    pCtx = gSMData.stateMachinesArray[i];

    *sizeRet += pCtx->numOfStates;
    *sizeRet += pCtx->numOfEvents;
  }

  *sizeRet = *sizeRet * sizeof(uint32_t);  // compensate for entry size

  pStatsData = malloc(*sizeRet);
  *retBulk = (char *)pStatsData;

  if (pStatsData == NULL) {
    APP_DBG_PRINT("SM Get Bulf alloc ERR");
    return -1;
  }

  for (i = 0; i < gSMData.numOfActiveSMs; i++) {
    pCtx = gSMData.stateMachinesArray[i];

    for (j = 0; j < pCtx->numOfStates; j++) {
      *pStatsData = pCtx->stateStat[j];
      pStatsData++;
    }

    for (j = 0; j < pCtx->numOfEvents; j++) {
      *pStatsData = pCtx->eventStat[j];
      pStatsData++;
    }
  }

  return 0;
}
