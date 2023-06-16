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
#include <string.h>
#include <stdbool.h>

#include "alt_osal.h"
#include "dbg_if.h"
#include "aws/altcom_aws.h"
#include "util/apiutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_AWS_QUEUE_NUM 5

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
bool gAwsInit = false;
MQTTSession_t *gAwsSession = NULL;
awsMsgCbFunc_t gAwsCallback = NULL;
awsConnEvtCbFunc_t gAwsEvtCallback = NULL;
void *gAwsUserPriv = NULL;
alt_osal_queue_handle gAwsEvtQue[MQTT_EVT_PUBCONF + 1];
bool gInProgress[MQTT_EVT_PUBCONF + 1] = {false};
alt_osal_mutex_handle gAwsMtx;
alt_osal_semaphore_handle gAwsSem;

/****************************************************************************
 * External Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mqttCallback(MQTTSession_t *session, MqttEvent_e evt, MqttResultData_t *result,
                  void *userPriv) {
  MQTTError_e retCode;

  DBGIF_ASSERT(gAwsSession == session, "Invalid AWS session handle\n");
  switch (evt) {
    case MQTT_EVT_CONCONF:
    case MQTT_EVT_DISCONF:
    case MQTT_EVT_SUBCONF:
    case MQTT_EVT_UNSCONF:
    case MQTT_EVT_PUBCONF:
      DBGIF_LOG1_DEBUG("API event %d arrival, \n", evt);
      alt_osal_lock_mutex(&gAwsMtx, ALT_OSAL_TIMEO_FEVR);
      if (gInProgress[evt]) {
        gInProgress[evt] = false;
        retCode = (MQTTError_e)result->resultCode;
        alt_osal_send_mqueue(&gAwsEvtQue[evt], (FAR int8_t *)&retCode, sizeof(MQTTError_e),
                             ALT_OSAL_TIMEO_FEVR);
        alt_osal_post_semaphore(&gAwsSem);
      }

      alt_osal_unlock_mutex(&gAwsMtx);
      break;

    case MQTT_EVT_PUBRCV:
      DBGIF_LOG_DEBUG("msg event arrival\n");
      if (gAwsCallback) {
        AWSMessageData msgData;

        msgData.qos = (AWSQoS_e)result->messageData.qos;
        msgData.topic = result->messageData.topic;
        msgData.topicLen = result->messageData.topicLen;
        msgData.message = result->messageData.message;
        msgData.msgLen = result->messageData.msgLen;
        gAwsCallback(&msgData, userPriv);
      }

      break;

    case MQTT_EVT_CONNFAIL:
      DBGIF_LOG_DEBUG("AWS connection failure!\n");
      if (gAwsEvtCallback) {
        gAwsEvtCallback(userPriv);
      }

      break;

    default:
      DBGIF_LOG1_ERROR("Invalid evt code: %d\n", evt);
  }
}

AWSError_e awsCheckApiStatus(MqttEvent_e evt, int32_t timeout_ms) {
  DBGIF_ASSERT(evt <= MQTT_EVT_PUBCONF, "invalid evt %lu\n", (uint32_t)evt);

  MQTTError_e ret;

  DBGIF_LOG1_ERROR("Check API status, wait for %ld ms\n", timeout_ms);
  alt_osal_unlock_mutex(&gAwsMtx);
  alt_osal_wait_semaphore(&gAwsSem, timeout_ms);
  alt_osal_lock_mutex(&gAwsMtx, ALT_OSAL_TIMEO_FEVR);
  if (gInProgress[evt] == true) {
    DBGIF_LOG1_ERROR("API timeout[%lu]\n", (uint32_t)evt);
    gInProgress[evt] = false;
    ret = MQTT_FAILURE;
  } else {
    if (alt_osal_recv_mqueue(&gAwsEvtQue[evt], (int8_t *)&ret, sizeof(MQTTError_e),
                             ALT_OSAL_TIMEO_FEVR) != sizeof(MQTTError_e)) {
      DBGIF_LOG2_ERROR("[%s:%d] Receive failed from queue.\n", __FUNCTION__, __LINE__);
      ret = MQTT_FAILURE;
    }
  }

  alt_osal_unlock_mutex(&gAwsMtx);
  return (AWSError_e)ret;
}

/**
 * Name: altcom_awsInitialize
 *
 *   altcom_awsInitialize() initialize a client to deal with aws.
 *
 *   @param [in] initParam: The initial parameter to create a new client, include cert profile,
 * endpoint, client ID...etc.
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsInitialize(awsInitParam_t *initParam) {
  MQTTSessionParams_t params;
  MQTTSession_t *tmpSession;
  int32_t ret;

  /* Check parameters */
  if (!initParam) {
    DBGIF_LOG_ERROR("Invalid endpoint URL\n");
    return AWS_FAILURE;
  }

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return AWS_FAILURE;
  }

  if (!gAwsInit) {
    alt_osal_queue_attribute que_param;

    memset((void *)&que_param, 0, sizeof(alt_osal_queue_attribute));
    que_param.numof_queue = MAX_AWS_QUEUE_NUM;
    que_param.queue_size = sizeof(MQTTError_e);
    for (MqttEvent_e i = MQTT_EVT_CONCONF; i <= MQTT_EVT_PUBCONF; i++) {
      ret = alt_osal_create_mqueue(&gAwsEvtQue[i], &que_param);
      DBGIF_ASSERT(0 == ret, "gAwsEvtQue create fail\n");
    }

    alt_osal_semaphore_attribute sem_param;

    memset((void *)&sem_param, 0, sizeof(alt_osal_semaphore_attribute));
    sem_param.max_count = 1;
    sem_param.initial_count = 0;
    ret = alt_osal_create_semaphore(&gAwsSem, &sem_param);
    DBGIF_ASSERT(0 == ret, "gAwsSem create fail\n");

    alt_osal_mutex_attribute mtx_param;

    memset((void *)&mtx_param, 0, sizeof(alt_osal_mutex_attribute));
    ret = alt_osal_create_mutex(&gAwsMtx, &mtx_param);
    DBGIF_ASSERT(0 == ret, "gAwsMtx create fail\n");

    gAwsSession = NULL;
    gAwsCallback = NULL;
    gAwsEvtCallback = NULL;
    gAwsUserPriv = NULL;
    gAwsInit = true;
  }

  if (gAwsSession) {
    altcom_mqttDisconnect(gAwsSession);
    vTaskDelay(1000);
    gAwsCallback = NULL;
    gAwsEvtCallback = NULL;
    gAwsUserPriv = NULL;
  }

  memset(&params, 0, sizeof(MQTTSessionParams_t));
  params.existSession = NULL != gAwsSession ? gAwsSession : NULL;
  params.sessionType = MQTT_SESSION_TYPE_AWS;
  params.tls.authMode = MQTTLICENT_AUTH_MUTUAL;
  params.ip.PdnSessionId = 0;
  params.ip.ipType = 0;
  params.ip.destPort = 8883;
  if (initParam->clientId) {
    strncpy(params.nodes.clientId, initParam->clientId, sizeof(params.nodes.clientId) - 1);
  }

  if (initParam->endpointUrl) {
    strncpy(params.nodes.url, initParam->endpointUrl, sizeof(params.nodes.url) - 1);
  }

  params.protocol.keepAliveTime = initParam->keepAliveTime;
  params.tls.authContext = initParam->certProfileId;
  params.protocol.cleanSessionFlag = initParam->cleanSessionFlag;

  gAwsCallback = initParam->callback;
  gAwsEvtCallback = initParam->evtCallback;
  gAwsUserPriv = initParam->userPriv;
  tmpSession = altcom_mqttSessionCreate(&params);
  if (!tmpSession) {
    return AWS_FAILURE;
  }

  if (!gAwsSession) {
    gAwsSession = tmpSession;
  } else {
    DBGIF_ASSERT(gAwsSession == tmpSession, "session handle inconsist!\n");
  }

  return AWS_SUCCESS;
}
