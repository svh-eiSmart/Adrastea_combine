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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alt_osal.h"
#include "dbg_if.h"
#include "altcom.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
static uint32_t g_altcomLogLevel = DBGIF_DEFAULT_OUTPUT_LV;
static alt_osal_mutex_handle g_logmtx = NULL;
static loglock_if_t g_loglock_if = {0};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
void DbgIf_Log_Lock_Priv() {
  if (g_logmtx) {
    alt_osal_lock_mutex(&g_logmtx, ALT_OSAL_TIMEO_FEVR);
  }
}

void DbgIf_Log_Unlock_Priv() {
  if (g_logmtx) {
    alt_osal_unlock_mutex(&g_logmtx);
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void DbgIf_Log_Lock(void) {
  if (g_loglock_if.lock) {
    g_loglock_if.lock();
  }
}

void DbgIf_Log_Unlock(void) {
  if (g_loglock_if.unlock) {
    g_loglock_if.unlock();
  }
}

void DbgIf_Log(uint32_t lv, const char* fmt, ...) {
  va_list args;

  DbgIf_Log_Lock();
  if (lv > g_altcomLogLevel) {
    DbgIf_Log_Unlock();
    return;
  }

  printf("[%lu] ", lv);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  DbgIf_Log_Unlock();
}

uint32_t DbgIf_GetLogLevel(void) {
  uint32_t level;

  DbgIf_Log_Lock();
  level = g_altcomLogLevel;
  DbgIf_Log_Unlock();
  return level;
}

void DbgIf_SetLogLevel(uint32_t level) {
  DbgIf_Log_Lock();
  g_altcomLogLevel = level;
  DbgIf_Log_Unlock();
}

int32_t DbgIf_Init(loglock_if_t* loglock_if) {
  if (loglock_if) {
    DBGIF_LOG_NORMAL("Use application log lock interface.\n");
    g_loglock_if = *loglock_if;
  } else {
    int32_t ret;
    alt_osal_mutex_attribute mtx_param = {0};

    if (!g_logmtx) {
      ret = alt_osal_create_mutex(&g_logmtx, &mtx_param);
      if (ret < 0) {
        DBGIF_LOG_ERROR("g_logmtx create failed.\n");
        return ret;
      }
    }

    g_loglock_if.lock = DbgIf_Log_Lock_Priv;
    g_loglock_if.unlock = DbgIf_Log_Unlock_Priv;
    DBGIF_LOG_NORMAL("Use default log lock interface.\n");
  };

  return 0;
}

int32_t DbgIf_Uninit(void) {
  if (g_logmtx) {
    alt_osal_delete_mutex(&g_logmtx);
    g_logmtx = NULL;
  }

  memset((void*)&g_loglock_if, 0, sizeof(loglock_if_t));
  return 0;
}
