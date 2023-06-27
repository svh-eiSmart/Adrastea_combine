/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "altcom_osal_opt.h"
#include "dbg_opt.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
static uint32_t g_altcomLogLevel = DBGIF_DEFAULT_OUTPUT_LV;
static altcom_sys_mutex_t g_logmtx = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void DbgIf_Log(uint32_t lv, const char* fmt, ...) {
  va_list args;

  if (g_logmtx) {
    altcom_sys_lock_mutex(&g_logmtx);
  }

  if (lv > g_altcomLogLevel) {
    if (g_logmtx) {
      altcom_sys_unlock_mutex(&g_logmtx);
    }

    return;
  }

  printf("[%lu] ", lv);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  if (g_logmtx) {
    altcom_sys_unlock_mutex(&g_logmtx);
  }
}

uint32_t DbgIf_GetLogLevel(void) {
  uint32_t level;

  if (g_logmtx) {
    altcom_sys_lock_mutex(&g_logmtx);
    level = g_altcomLogLevel;
    altcom_sys_unlock_mutex(&g_logmtx);
  } else {
    return g_altcomLogLevel;
  }

  return level;
}

void DbgIf_SetLogLevel(uint32_t level) {
  if (g_logmtx) {
    altcom_sys_lock_mutex(&g_logmtx);
    g_altcomLogLevel = level;
    altcom_sys_unlock_mutex(&g_logmtx);
  } else {
    g_altcomLogLevel = level;
  }
}

altcom_sys_mutex_t DbgIf_GetLogMtx(void) { return g_logmtx; }

void DbgIf_SetLogMtx(altcom_sys_mutex_t mtx) { g_logmtx = mtx; }