/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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

#include <stdint.h>
#include <errno.h>

#include "altcom.h"
#include "apiutil.h"
#include "altcombuilder.h"
#include "director.h"
#include "dbg_if.h"

/****************************************************************************
 * External Data
 ****************************************************************************/
extern uint32_t g_altcomLogLevel;

/****************************************************************************
 * Public Data
 ****************************************************************************/
bool g_altcom_initialized = false;
bool g_altcom_cbreg_only = false;
altcom_sys_mutex_t g_altcom_apicallback_mtx;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_initialize() initialize the ALTCOM library resources.
 *
 * @param [in] initCfg Initialization configuration, see @ref altcom_init_t.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t altcom_initialize(altcom_init_t *initCfg) {
  int32_t ret;

  if (!initCfg) {
    DBGIF_LOG_ERROR("Invalid initialize configuration.\n");
    return -1;
  }

  /* Set initialized status */
  ret = altcom_check_initialized_and_set();
  if (ret < 0) {
    DBGIF_LOG_ERROR("Already initialized.\n");
  } else {
    ret = director_construct(&g_altcombuilder, (void *)initCfg);
    if (ret < 0) {
      DBGIF_LOG1_ERROR("director_construct() error, reason: %ld.\n", ret);
      altcom_set_finalized();
    } else {
      ret = 0;
    }
  }

  return ret;
}

/**
 * @brief altcom_app_ready() indicates that the application is ready to work
 * and will start to replay the postponed events.
 *
 */

void altcom_app_ready(void) {
  DBGIF_LOG_DEBUG("Application ready! Start to replay the postponed event list\n");
  apicmdgw_replay_postponed_event();
  DBGIF_LOG_DEBUG("Application ready! Change the reporting APIs mode to Reg + Cmd\n");
  altcom_SetCallbackReg(false);
  apicmdgw_recvagain();
}

/**
 * @brief altcom_get_log_level() obtains the current ALTCOM log message level
 *
 * @return Enumeration of current log message level.
 */

dbglevel_e altcom_get_log_level(void) { return (dbglevel_e)DbgIf_GetLogLevel(); }

/**
 * @brief altcom_set_log_level() sets the target log message level of ALTCOM at runtime.
 *
 * @param [in] level: The target log message level of ALTCOM.
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_set_log_level(dbglevel_e level) {
  uint32_t lvl;

  lvl = (uint32_t)level;
  if (lvl > DBGIF_LV_DBG) {
    DBGIF_LOG1_ERROR("Invalid log level %lu\n", lvl);
    return -1;
  }

  DbgIf_SetLogLevel(lvl);
  return 0;
}