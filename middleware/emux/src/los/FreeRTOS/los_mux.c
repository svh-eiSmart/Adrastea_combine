/*  ---------------------------------------------------------------------------

    (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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
#include "los/los_mux.h"
#include "serial_hal/hal_mux.h"

#define ERROR (-1)

long losGetTimeInMilliseconds() { return (long)xTaskGetTickCount() * portTICK_PERIOD_MS; }

int losMuxCreateMutex(void *mutex) {
  SemaphoreHandle_t *pMutex = (SemaphoreHandle_t *)mutex;
  *pMutex = xSemaphoreCreateMutex();
  return (*pMutex == NULL) ? 0 : 1;
}

void losMuxMutexTake(void *mutex) {
  SemaphoreHandle_t *mutexPointer = (SemaphoreHandle_t *)mutex;
  xSemaphoreTake(*mutexPointer, portMAX_DELAY);
}

void losMuxMutexGive(void *mutex) {
  SemaphoreHandle_t *mutexPointer = (SemaphoreHandle_t *)mutex;
  xSemaphoreGive(*mutexPointer);
}

int losMuxDeleteMutex(void *mutex) {
  SemaphoreHandle_t *pMutex = (SemaphoreHandle_t *)mutex;
  if (*pMutex == NULL) {
    return 0;
  } else {
    vSemaphoreDelete(*pMutex);
    return 1;
  }
}

int losMuxCreateEventGroup(void *eg) {
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  *egPointer = xEventGroupCreate();
  return (*egPointer == NULL) ? 0 : 1;
}

losMuxEvBits_t losMuxEventGroupWait(void *eg, losMuxEvBits_t BitsToWaitFor, uint32_t timeoutMS) {
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  if (timeoutMS == LOS_TIMEOUT_FOREVER) {
    return xEventGroupWaitBits(*egPointer, BitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);
  } else {
    return xEventGroupWaitBits(*egPointer, BitsToWaitFor, pdTRUE, pdFALSE,
                               timeoutMS / portTICK_PERIOD_MS);
  }
}

void losMuxEventGroupSetBitsISR(void *eg, losMuxEvBits_t evBits) {
  BaseType_t xHigherPriorityTaskWoken, xResult;
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  xResult = xEventGroupSetBitsFromISR(*egPointer, evBits, &xHigherPriorityTaskWoken);
  if (xResult != pdFAIL) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void losMuxEventGroupSetBits(void *eg, losMuxEvBits_t evBits) {
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  xEventGroupSetBits(*egPointer, evBits);
}

void losMuxEventGroupClearBits(void *eg, losMuxEvBits_t evBits) {
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  xEventGroupClearBits(*egPointer, evBits);
}

int losMuxDeleteEventGroup(void *eg) {
  EventGroupHandle_t *egPointer = (EventGroupHandle_t *)eg;
  if (*egPointer == NULL) {
    return 0;
  } else {
    vEventGroupDelete(*egPointer);
    return 1;
  }
}

int losMuxCreateTask(losMuxTaskFunction_t taskFunction, const char *const pcName,
                     uint32_t stackSize, void *const pvParameters, uint16_t taskPrio) {
  if (xTaskCreate(taskFunction, pcName, stackSize, pvParameters, taskPrio, NULL) == pdPASS)
    return 1;
  else
    return 0;
}

// return port handler
void *losBindToPortApi(int logicalPortNum, muxRxFp_t rxProcessFp, void *appCookie,
                       muxTxBuffFp_t *txCharFp) {
  int muxID = logicalPortNum;
  if (halSerialConfigure(muxID, rxProcessFp, appCookie, txCharFp) == ERROR) {
    goto err;
  }
  return (void *)muxID;
err:
  return (void *)ERROR;
}
