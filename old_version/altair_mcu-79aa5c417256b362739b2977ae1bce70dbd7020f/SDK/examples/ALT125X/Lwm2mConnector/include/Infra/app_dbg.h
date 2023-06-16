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

#ifndef APP_DBG_H
#define APP_DBG_H

/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Kernel Header */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

#include "assert.h"

/* Logging System Header */
#include "sfplogger.h"

/* App includes. */
#include "apitest_main.h"

/* Compilation */
#define APP_UNUSED(x) (void)(x)

/* Debugging Message */
#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APP_DBG_VERBOSE_PRINT(format, ...)                                                       \
  do {                                                                                           \
    LOCK();                                                                                      \
    sfp_log_formatted(SFPLOG_DBG_PRINT, SFPLOG_VERBOSE, "[%d]" format, __LINE__, ##__VA_ARGS__); \
    UNLOCK();                                                                                    \
  } while (0)

#define APP_DBG_PRINT(format, ...)                                                            \
  do {                                                                                        \
    LOCK();                                                                                   \
    sfp_log_formatted(SFPLOG_DBG_PRINT, SFPLOG_INFO, "[%d]" format, __LINE__, ##__VA_ARGS__); \
    UNLOCK();                                                                                 \
  } while (0)

#define APP_ERR_PRINT(format, ...)                                                             \
  do {                                                                                         \
    LOCK();                                                                                    \
    sfp_log_formatted(SFPLOG_DBG_PRINT, SFPLOG_ERROR, "[%d]" format, __LINE__, ##__VA_ARGS__); \
    UNLOCK();                                                                                  \
  } while (0)

#define APP_ASSERT(condition, format, ...)                                          \
  do {                                                                              \
    if (true != (condition)) {                                                      \
      APP_DBG_PRINT("[%s:%d] ##We ASSERT HERE!!## " format, __FUNCTION__, __LINE__, \
                    ##__VA_ARGS__);                                                 \
      assert(false);                                                                \
    }                                                                               \
  } while (0)

#define APP_ASSERT_ISR(condition, format, ...)                                                    \
  do {                                                                                            \
    if (true != (condition)) {                                                                    \
      printf("[%s:%d] ##We ASSERT HERE(ISR)!!## " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      while (1) {                                                                                 \
        continue;                                                                                 \
      };                                                                                          \
    }                                                                                             \
  } while (0)

#if 0
#define APP_TRACE()                 \
  do {                              \
    APP_DBG_PRINT("APP_TRACE\r\n"); \
  } while (0)
#else
#define APP_TRACE()
#endif

#define TOSTR(x) #x

#endif /* APP_DBG_H */
