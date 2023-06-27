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
#include "altcom_osal.h"
#include "buffpoolwrapper.h"
#include "altcom_mqtt.h"
#include "apicmd_mqttMessageEvt.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct mqttMessageCbItem_s {
  MQTTSession_t *session;
  MQTTEvtCbFunc_t callback;
  void *cbParam;
  struct mqttMessageCbItem_s *next;
  struct mqttMessageCbItem_s *prev;
};

struct mqttMessageCbCtrl_s {
  struct mqttMessageCbItem_s *msgCbTbl;
  altcom_sys_mutex_t tblMtx;
};

static struct mqttMessageCbCtrl_s gMqttMsgCbCtrl;
static bool isMsgCbTblInit = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mqttCheckInitCbCtrl
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

void mqttCheckInitCbCtrl(void) {
  int32_t ret;
  if (!isMsgCbTblInit) {
    gMqttMsgCbCtrl.msgCbTbl = NULL;
    ret = altcom_sys_create_mutex(&gMqttMsgCbCtrl.tblMtx, NULL);
    DBGIF_ASSERT(0 == ret, "tblMtx init failed!\n");
    if (ret != 0) {
      return;
    }

    isMsgCbTblInit = true;
  }
}

/****************************************************************************
 * Name: mqttMessageEvt_job
 *
 * Description:
 *   This function is an API callback for MQTT message notification.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void mqttMessageEvt_job(FAR void *arg) {
  FAR struct apicmd_mqttmessageevtres_s *evt;
  MQTTSession_t *session;
  MqttResultData_t result;

  /* search and callback */
  struct mqttMessageCbItem_s *cbTable;
  MQTTEvtCbFunc_t callback;
  void *cbParam;

  mqttCheckInitCbCtrl();
  evt = (FAR struct apicmd_mqttmessageevtres_s *)arg;
  altcom_sys_lock_mutex(&gMqttMsgCbCtrl.tblMtx);
  cbTable = gMqttMsgCbCtrl.msgCbTbl;
  callback = NULL;
  session = (MQTTSession_t *)ntohl((uint32_t)evt->session);
  for (; NULL != cbTable; cbTable = cbTable->next) {
    if (session == cbTable->session) {
      callback = cbTable->callback;
      cbParam = cbTable->cbParam;
      break;
    }
  }

  altcom_sys_unlock_mutex(&gMqttMsgCbCtrl.tblMtx);
  if (callback) {
    DBGIF_LOG1_DEBUG("Callback %p\n", (void *)callback);
    memset(&result, 0x0, sizeof(MqttResultData_t));
    result.resultCode = (int)ntohl(evt->resultCode);
    result.errorCode = (int)ntohl(evt->errorCode);
    result.messageId = (int)ntohl(evt->messageId);
    result.messageData.qos = (MQTTQoS_e)evt->qos;
    result.messageData.retained = evt->retained;
    result.messageData.dup = evt->dup;
    result.messageData.id = ntohs(evt->id);
    result.messageData.topic = ntohs(evt->topicLen) > 1 ? evt->evtData : NULL;
    result.messageData.topicLen = ntohs(evt->topicLen);
    result.messageData.message = ntohs(evt->topicLen) > 1 && ntohs(evt->msgLen) > 1
                                     ? evt->evtData + ntohs(evt->topicLen)
                                     : NULL;
    result.messageData.msgLen = ntohs(evt->msgLen);
    callback(session, (MqttEvent_e)evt->evtType, &result, cbParam);
  } else {
    DBGIF_LOG_DEBUG("orphan callback\n");
  }

  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_mqttMessageEvt
 *
 * Description:
 *   This function is an API command handler for MQTT message notification.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_MQTT_MESSAGEEVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_mqttMessageEvt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_MQTT_MESSAGEEVT, mqttMessageEvt_job);
}

/****************************************************************************
 * Name: mqttHelper_RegisterCallback
 *
 *   This function is an internal helper function to register user callback which called on event
 *arrival; The application developer no need to call this function
 *
 * Input Parameters:
 *  sesion - MQTT session handle.
 *  callback - Callback function to the given topic, NULL value imply to deregister callback.
 *  cbParam -The parameter ptr to the given callback function.
 *
 * Returned Value:
 *  MQTT_SUCCESS - Callback function registered success.
 *  MQTT_FAILURE - Failed to register callback.
 *
 ****************************************************************************/

MQTTError_e mqttHelper_RegisterCallback(MQTTSession_t *session, MQTTEvtCbFunc_t callback,
                                        void *cbParam) {
  DBGIF_ASSERT(NULL != session, "Invalid session ptr");

  struct mqttMessageCbItem_s *cbTable;

  mqttCheckInitCbCtrl();
  DBGIF_LOG_DEBUG("mqttHelper_RegisterCallback enter\n");
  altcom_sys_lock_mutex(&gMqttMsgCbCtrl.tblMtx);

  /*search & replace callback if exist*/
  cbTable = gMqttMsgCbCtrl.msgCbTbl;
  for (; NULL != cbTable; cbTable = cbTable->next) {
    if (cbTable->session == session) {
      break;
    }
  }

  /*Manipulate callback element */
  if (cbTable) {
    cbTable->cbParam = cbParam;
    if (callback) {
      /* this case is to replace callback */
      cbTable->callback = callback;
    } else {
      /* this case is to remove callback */
      if (cbTable->prev) {
        cbTable->prev->next = cbTable->next;
      } else {
        gMqttMsgCbCtrl.msgCbTbl = cbTable->next;
      }

      if (cbTable->next) {
        cbTable->next->prev = cbTable->prev;
      }

      BUFFPOOL_FREE((void *)cbTable);
    }
  } else {
    /* this case is to append a new element */
    cbTable = (struct mqttMessageCbItem_s *)BUFFPOOL_ALLOC(sizeof(struct mqttMessageCbItem_s));
    DBGIF_ASSERT(NULL != cbTable, "cbItem alloc failed\n");
    cbTable->session = session;
    cbTable->callback = callback;
    cbTable->cbParam = cbParam;
    if (gMqttMsgCbCtrl.msgCbTbl) {
      cbTable->prev = NULL;
      cbTable->next = gMqttMsgCbCtrl.msgCbTbl;
      gMqttMsgCbCtrl.msgCbTbl->prev = cbTable;
      gMqttMsgCbCtrl.msgCbTbl = cbTable;
    } else {
      cbTable->prev = cbTable->next = NULL;
      gMqttMsgCbCtrl.msgCbTbl = cbTable;
    }

    gMqttMsgCbCtrl.msgCbTbl = cbTable;
  }

  altcom_sys_unlock_mutex(&gMqttMsgCbCtrl.tblMtx);
  DBGIF_LOG_DEBUG("mqttHelper_RegisterCallback leave\n");
  return MQTT_SUCCESS;
}

/****************************************************************************
 * Name: mqttHelper_ClearAllCallback
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

void mqttHelper_ClearAllCallback(void) {
  struct mqttMessageCbItem_s *cbTable;

  mqttCheckInitCbCtrl();
  DBGIF_LOG_DEBUG("mqttHelper_ClearAllCallback enter\n");
  altcom_sys_lock_mutex(&gMqttMsgCbCtrl.tblMtx);

  while (NULL != gMqttMsgCbCtrl.msgCbTbl) {
    cbTable = gMqttMsgCbCtrl.msgCbTbl;
    gMqttMsgCbCtrl.msgCbTbl = gMqttMsgCbCtrl.msgCbTbl->next;
    BUFFPOOL_FREE((void *)cbTable);
  }

  altcom_sys_unlock_mutex(&gMqttMsgCbCtrl.tblMtx);
  DBGIF_LOG_DEBUG("mqttHelper_ClearAllCallback leave\n");
}
