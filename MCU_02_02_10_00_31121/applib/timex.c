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
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timex.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
static time_t up_time = 0;
static time_t delta_system_time = 0;

static volatile int64_t x64TickCnt = (int64_t)configINITIAL_TICK_COUNT;
static volatile int64_t x64PrevUpdateTickCnt = (int64_t)configINITIAL_TICK_COUNT;

static void update_system_time(int seconds) {
  if (pdFALSE == xPortIsInsideInterrupt()) {
    taskENTER_CRITICAL();
  }

  up_time += seconds;
  if (pdFALSE == xPortIsInsideInterrupt()) {
    taskEXIT_CRITICAL();
  }
}

/****************************************************************************
 * Public Data
 ****************************************************************************/
time_t time(time_t *t) {
  time_t time;

  taskENTER_CRITICAL();
  time = up_time + delta_system_time;
  taskEXIT_CRITICAL();

  if (t) {
    *t = time;
  }

  return time;
}

int stime(const time_t *t) {
  taskENTER_CRITICAL();
  delta_system_time = *t - up_time;
  taskEXIT_CRITICAL();
  return 0;
}

void uptime(time_t *t) {
  taskENTER_CRITICAL();
  *t = up_time;
  taskEXIT_CRITICAL();
}

void vApplicationTickHook(void) {
  int64_t x64TickCntDiff;

  x64TickCnt = (INT64_MAX == x64TickCnt) ? (0) : (x64TickCnt + 1);
  x64TickCntDiff = (x64TickCnt >= x64PrevUpdateTickCnt)
                       ? (x64TickCnt - x64PrevUpdateTickCnt)
                       : (INT64_MAX - x64PrevUpdateTickCnt + x64TickCnt);
  if (x64TickCntDiff > 1000 * portTICK_PERIOD_MS) {
    update_system_time(1);
    x64PrevUpdateTickCnt =
        ((INT64_MAX - 1000 * portTICK_PERIOD_MS) >= x64PrevUpdateTickCnt)
            ? (x64PrevUpdateTickCnt + 1000 * portTICK_PERIOD_MS)
            : ((1000 * portTICK_PERIOD_MS - (INT64_MAX - x64PrevUpdateTickCnt) - 1));
  }
}

void trace_tick_count(const uint32_t xTicksToJump) {
  int64_t x64TickCntDiff;

  x64TickCnt = (INT64_MAX == x64TickCnt) ? (0) : (x64TickCnt + xTicksToJump);
  x64TickCntDiff = (x64TickCnt >= x64PrevUpdateTickCnt)
                       ? (x64TickCnt - x64PrevUpdateTickCnt)
                       : (INT64_MAX - x64PrevUpdateTickCnt + x64TickCnt);
  if (x64TickCntDiff > 1000 * portTICK_PERIOD_MS) {
    update_system_time(x64TickCntDiff / (1000 * portTICK_PERIOD_MS));
    x64PrevUpdateTickCnt = ((INT64_MAX - x64TickCntDiff) >= x64PrevUpdateTickCnt)
                               ? (x64PrevUpdateTickCnt + x64TickCntDiff)
                               : ((x64TickCntDiff - (INT64_MAX - x64PrevUpdateTickCnt) - 1));
  }
}