/****************************************************************************
 *
 *  (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.
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
#include <string.h>
#include "dbg_if.h"
#include "apiutil.h"
#include "altcom_osal.h"
#include "buffpoolwrapper.h"
#include "apicmd.h"
#include "altcom_atcmd.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct atcmdUrcCbItem_s {
  char *urcStr;
  uint16_t urcLen;
  AtcmdUrcCbFunc_t callback;
  void *cbParam;
  struct atcmdUrcCbItem_s *next;
  struct atcmdUrcCbItem_s *prev;
};

struct atcmdUrcCbCtrl_s {
  struct atcmdUrcCbItem_s *msgCbTbl;
  altcom_sys_mutex_t tblMtx;
};

static struct atcmdUrcCbCtrl_s gAtcmdUrcCbCtrl;
static bool isMsgCbTblInit = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: atcmdCheckInitCbCtrl
 *
 * Description:
 *   Check and initialize callback related structures.
 *
 * Input Parameters:
 *  None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void atcmdCheckInitCbCtrl(void) {
  int32_t ret;
  if (!isMsgCbTblInit) {
    gAtcmdUrcCbCtrl.msgCbTbl = NULL;
    ret = altcom_sys_create_mutex(&gAtcmdUrcCbCtrl.tblMtx, NULL);
    DBGIF_ASSERT(0 == ret, "tblMtx init failed!\n");
    if (ret != 0) {
      return;
    }

    isMsgCbTblInit = true;
  }
}

/****************************************************************************
 * Name: atcmdUrcEvt_job
 *
 * Description:
 *   This function is an API callback for URC notification.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void atcmdUrcEvt_job(void *arg) {
  char *evt;
  struct atcmdUrcCbItem_s *cbTable;
  AtcmdUrcCbFunc_t callback;
  void *cbParam;

  /* search and callback */

  atcmdCheckInitCbCtrl();
  evt = (char *)arg;
  altcom_sys_lock_mutex(&gAtcmdUrcCbCtrl.tblMtx);
  cbTable = gAtcmdUrcCbCtrl.msgCbTbl;
  callback = NULL;
  DBGIF_LOG1_DEBUG("Evt %s\n", evt);

  for (; NULL != cbTable; cbTable = cbTable->next) {
    DBGIF_LOG2_DEBUG("cbTable->urcLen: %hu, cbTable->urcStr %s\n", cbTable->urcLen,
                     cbTable->urcStr);
    if (strstr(evt, cbTable->urcStr)) {
      callback = cbTable->callback;
      cbParam = cbTable->cbParam;
      break;
    }
  }

  altcom_sys_unlock_mutex(&gAtcmdUrcCbCtrl.tblMtx);
  if (callback) {
    DBGIF_LOG1_DEBUG("Callback %p\n", (void *)callback);
    callback(evt, cbParam);
  } else {
    DBGIF_LOG_DEBUG("Orphan callback\n");
  }

  altcom_free_cmd((uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_atcmdUrcEvt
 *
 * Description:
 *   This function is an API command handler for URC event notification.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_ATCMD_URCEVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_atcmdUrcEvt(uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_ATCMDCONN_URCEVT, atcmdUrcEvt_job);
}

/****************************************************************************
 * Name: atcmdHelper_RegisterCallback
 *
 *   This function is an internal helper function to register user callback which called on URC
 *event arrival; The application developer no need to call this function
 *
 * Input Parameters:
 *  numUrcStr - Number of registering URC strings
 *  urcStr - The registering URC strings array
 *  urcCb - Callback function to the given topic, NULL value imply to deregister callback.
 *  cbParam -The parameter ptr to the given callback function.
 *
 * Returned Value:
 *  ATCMD_SUCCESS - Callback function registered success.
 *  ATCMD_FAILURE - Failed to register callback.
 *
 ****************************************************************************/

ATCMDError_e atcmdHelper_RegisterCallback(const char *urcStr, AtcmdUrcCbFunc_t urcCb,
                                          void *cbParam) {
  struct atcmdUrcCbItem_s *cbTable;

  atcmdCheckInitCbCtrl();
  DBGIF_LOG_DEBUG("atcmdHelper_RegisterCallback enter\n");
  altcom_sys_lock_mutex(&gAtcmdUrcCbCtrl.tblMtx);

  /* search & replace callback if exist */
  cbTable = gAtcmdUrcCbCtrl.msgCbTbl;
  for (; NULL != cbTable; cbTable = cbTable->next) {
    if (!strncmp(cbTable->urcStr, urcStr, cbTable->urcLen)) {
      break;
    }
  }

  /* Manipulate callback element */
  if (cbTable) {
    cbTable->cbParam = cbParam;
    if (urcCb) {
      /* this case is to replace callback */
      cbTable->callback = urcCb;
    } else {
      /* this case is to remove callback */
      if (cbTable->prev) {
        cbTable->prev->next = cbTable->next;
      } else {
        gAtcmdUrcCbCtrl.msgCbTbl = cbTable->next;
      }

      if (cbTable->next) {
        cbTable->next->prev = cbTable->prev;
      }

      BUFFPOOL_FREE((void *)cbTable->urcStr);
      BUFFPOOL_FREE((void *)cbTable);
    }
  } else {
    /* this case is to append a new element */
    cbTable = (struct atcmdUrcCbItem_s *)BUFFPOOL_ALLOC(sizeof(struct atcmdUrcCbItem_s));
    DBGIF_ASSERT(NULL != cbTable, "cbItem alloc failed\n");
    cbTable->urcLen = strlen(urcStr);
    cbTable->urcStr = (char *)BUFFPOOL_ALLOC(cbTable->urcLen + 1);
    DBGIF_ASSERT(NULL != cbTable->urcStr, "urcStr alloc failed\n");
    memcpy(cbTable->urcStr, urcStr, cbTable->urcLen);
    cbTable->urcStr[cbTable->urcLen] = '\0';
    cbTable->callback = urcCb;
    cbTable->cbParam = cbParam;
    if (gAtcmdUrcCbCtrl.msgCbTbl) {
      cbTable->prev = NULL;
      cbTable->next = gAtcmdUrcCbCtrl.msgCbTbl;
      gAtcmdUrcCbCtrl.msgCbTbl->prev = cbTable;
      gAtcmdUrcCbCtrl.msgCbTbl = cbTable;
    } else {
      cbTable->prev = cbTable->next = NULL;
      gAtcmdUrcCbCtrl.msgCbTbl = cbTable;
    }

    gAtcmdUrcCbCtrl.msgCbTbl = cbTable;
  }

  altcom_sys_unlock_mutex(&gAtcmdUrcCbCtrl.tblMtx);
  DBGIF_LOG_DEBUG("atcmdHelper_RegisterCallback leave\n");
  return ATCMD_SUCCESS;
}

/****************************************************************************
 * Name: atcmdHelper_ClearAllCallback
 *
 * Description:
 *   This function is an internal helper function to clear all registered callback from user;
 *   The application developer no need to call this function.
 * Input Parameters:
 *  None.
 *
 * Returned Value:
 *  None.
 *
 ****************************************************************************/

void atcmdHelper_ClearAllCallback(void) {
  struct atcmdUrcCbItem_s *cbTable;

  atcmdCheckInitCbCtrl();
  DBGIF_LOG_DEBUG("atcmdHelper_ClearAllCallback enter\n");
  altcom_sys_lock_mutex(&gAtcmdUrcCbCtrl.tblMtx);

  while (NULL != gAtcmdUrcCbCtrl.msgCbTbl) {
    cbTable = gAtcmdUrcCbCtrl.msgCbTbl;
    gAtcmdUrcCbCtrl.msgCbTbl = gAtcmdUrcCbCtrl.msgCbTbl->next;
    BUFFPOOL_FREE((void *)cbTable->urcStr);
    BUFFPOOL_FREE((void *)cbTable);
  }

  altcom_sys_unlock_mutex(&gAtcmdUrcCbCtrl.tblMtx);
  DBGIF_LOG_DEBUG("atcmdHelper_ClearAllCallback leave\n");
}
