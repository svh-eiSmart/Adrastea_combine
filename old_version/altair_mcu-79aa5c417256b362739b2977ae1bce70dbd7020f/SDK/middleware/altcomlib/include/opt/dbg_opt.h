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

#ifndef __ALTCOM_INCLUDE_DBG_OPT_H
#define __ALTCOM_INCLUDE_DBG_OPT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "alt_osal.h"
#include "altcom.h"
#ifdef __ALTCOM_USE_SFP_LOG__
#include "sfplogger.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DBG_LOCK(handle)                                 \
  do {                                                   \
    alt_osal_lock_mutex(&(handle), ALT_OSAL_TIMEO_FEVR); \
  } while (0)
#define DBG_UNLOCK(handle)            \
  do {                                \
    alt_osal_unlock_mutex(&(handle)); \
  } while (0)

/* Debug log level */
#define DBGIF_DEFAULT_OUTPUT_LV 3

#define DBGIF_LV_ERR 1
#define DBGIF_LV_WARN 2
#define DBGIF_LV_NORM 3
#define DBGIF_LV_INF 4
#define DBGIF_LV_DBG 5

#ifdef __ALTCOM_USE_SFP_LOG__
#define DBGIF_LOG(lv, fmt, ...)                               \
  do {                                                        \
    DbgIf_Log_Lock();                                         \
    sfp_log_formatted(SFPLOG_ALTCOM, lv, fmt, ##__VA_ARGS__); \
    DbgIf_Log_Unlock();                                       \
  } while (0)
#else
#define DBGIF_LOG(lv, fmt, ...) DbgIf_Log(lv, fmt, ##__VA_ARGS__)
#endif

#ifdef NDEBUG
#define DBGIF_ASSERT(asrt, fmt, ...) \
  do {                               \
    alt_osal_assert(asrt);           \
  } while (0)
#else
#define DBGIF_ASSERT(asrt, fmt, ...)               \
  do {                                             \
    if (!(asrt)) {                                 \
      DBGIF_LOG(DBGIF_LV_ERR, fmt, ##__VA_ARGS__); \
      alt_osal_assert(false);                      \
    }                                              \
  } while (0)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void DbgIf_Log(uint32_t lv, const char* fmt, ...);
uint32_t DbgIf_GetLogLevel(void);
void DbgIf_SetLogLevel(uint32_t level);
void DbgIf_Log_Lock(void);
void DbgIf_Log_Unlock(void);
int32_t DbgIf_Init(loglock_if_t* loglock_if);
int32_t DbgIf_Uninit(void);
#endif /* __ALTCOM_INCLUDE_DBG_OPT_H */
