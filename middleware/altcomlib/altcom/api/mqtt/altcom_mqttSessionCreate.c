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
#include <string.h>
#include <stdbool.h>

#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_mqtt.h"
#include "apicmd_mqttSessionCreate.h"
#include "apicmdhdlr_mqttMessageEvt.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MQTTSESSIONCREATE_REQ_DATALEN (sizeof(struct apicmd_mqttsessioncreate_s))
#define MQTTSESSIONCREATE_RES_DATALEN (sizeof(struct apicmd_mqttsessioncreateres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static MQTTSession_t *mqttSessionCreate_request(MQTTSessionParams_t *sessionParam) {
  int32_t ret = 0;
  uint16_t resLen = 0;
  int32_t sendLen;
  FAR struct apicmd_mqttsessioncreate_s *cmd = NULL;
  FAR struct apicmd_mqttsessioncreateres_s *res = NULL;

  /* Allocate send and response command buffer */
  sendLen = strlen(sessionParam->nodes.url) + strlen(sessionParam->nodes.clientId) +
            strlen(sessionParam->nodes.username) + strlen(sessionParam->nodes.password) +
            strlen(sessionParam->willMsg.topic) + strlen(sessionParam->willMsg.msg) +
            6; /* Add 6 means 6 null terminate symbols */
  if (sendLen > MQTT_CREATESESSION_REQSTR_MAXLEN) {
    DBGIF_LOG1_ERROR("[mqttSessionCreate_request] strData length too long: %ld\n", sendLen);
    return NULL;
  }

  sendLen += MQTTSESSIONCREATE_REQ_DATALEN - MQTT_CREATESESSION_REQSTR_MAXLEN;
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MQTT_SESSIONCREATE, sendLen,
                                          (FAR void **)&res, MQTTSESSIONCREATE_RES_DATALEN)) {
    DBGIF_LOG_ERROR("mem alloc failed\n");
    return NULL;
  }

  /* Fill the data */
  cmd->existSession = htonl((uint32_t)sessionParam->existSession);
  cmd->authMode = sessionParam->tls.authMode;
  cmd->authContext = sessionParam->tls.authContext;

  cmd->pdnSessionId = sessionParam->ip.PdnSessionId;
  cmd->ipType = sessionParam->ip.ipType;
  cmd->destPort = htonl(sessionParam->ip.destPort);
  cmd->sourcePort = htonl(sessionParam->ip.sourcePort);

  cmd->enableWillMsg = sessionParam->willMsg.enable;
  cmd->willQoS = sessionParam->willMsg.qos;
  cmd->willRetain = sessionParam->willMsg.retain;

  cmd->protocolType = sessionParam->protocol.type;
  cmd->keepAliveTime = htonl(sessionParam->protocol.keepAliveTime);
  cmd->cleanSessionFlag = sessionParam->protocol.cleanSessionFlag;
  cmd->sessionType = sessionParam->sessionType;

  char *str = cmd->strData;
  uint16_t strLen;

  /* URL */
  strLen = strlen(sessionParam->nodes.url);
  if (MQTTCFG_MAX_URL_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_URL_LEN, "%s", sessionParam->nodes.url);
    DBGIF_LOG2_DEBUG("url: %s, len: %d\n", str, strLen);
  } else {
    DBGIF_LOG1_ERROR("Invalid url len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->urlLen = htons(strLen + 1);
  str += strLen + 1;

  /* CleintId */
  strLen = strlen(sessionParam->nodes.clientId);
  if (MQTTCFG_MAX_CLIENTID_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_CLIENTID_LEN, "%s", sessionParam->nodes.clientId);
    DBGIF_LOG2_DEBUG("clientId: %s, len: %d\n", str, strLen);
  } else if (0 == strLen) {
    DBGIF_LOG_DEBUG("Empty clientId\n");
  } else {
    DBGIF_LOG1_ERROR("Invalid clientId len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->clientIdLen = htons(strLen + 1);
  str += strLen + 1;

  /* username */
  strLen = strlen(sessionParam->nodes.username);
  if (MQTTCFG_MAX_CLIENTID_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_CLIENTID_LEN, "%s", sessionParam->nodes.username);
    DBGIF_LOG2_DEBUG("username: %s, len: %d\n", str, strLen);
  } else if (0 == strLen) {
    DBGIF_LOG_DEBUG("Empty username\n");
  } else {
    DBGIF_LOG1_ERROR("Invalid username len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->usernameLen = htons(strLen + 1);
  str += strLen + 1;

  /* password */
  strLen = strlen(sessionParam->nodes.password);
  if (MQTTCFG_MAX_CLIENTID_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_CLIENTID_LEN, "%s", sessionParam->nodes.password);
    DBGIF_LOG2_DEBUG("password: %s, len: %d\n", str, strLen);
  } else if (0 == strLen) {
    DBGIF_LOG_DEBUG("Empty password\n");
  } else {
    DBGIF_LOG1_ERROR("Invalid password len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->passwordLen = htons(strLen + 1);
  str += strLen + 1;

  /* will topic */
  strLen = strlen(sessionParam->willMsg.topic);
  if (MQTTCFG_MAX_WILLTOPIC_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_WILLTOPIC_LEN, "%s", sessionParam->willMsg.topic);
    DBGIF_LOG2_DEBUG("will topic: %s, len: %d\n", str, strLen);
  } else if (0 == strLen) {
    DBGIF_LOG_DEBUG("Empty will topic\n");
  } else {
    DBGIF_LOG1_ERROR("Invalid will topic len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->willTopicLen = htons(strLen + 1);
  str += strLen + 1;

  /* will message */
  strLen = strlen(sessionParam->willMsg.msg);
  if (MQTTCFG_MAX_WILLMSG_LEN - 1 > strLen && 0 != strLen) {
    strLen = snprintf(str, MQTTCFG_MAX_WILLMSG_LEN, "%s", sessionParam->willMsg.msg);
    DBGIF_LOG2_DEBUG("will msg: %s, len: %d\n", str, strLen);
  } else if (0 == strLen) {
    DBGIF_LOG_DEBUG("Empty will msg\n");
  } else {
    DBGIF_LOG1_ERROR("Invalid will msg len: %d\n", strLen);
    goto errout_with_cmdfree;
  }

  cmd->willMsgLen = htons(strLen + 1);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, MQTTSESSIONCREATE_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = (int32_t)NULL;
    goto errout_with_cmdfree;
  }

  if (resLen != MQTTSESSIONCREATE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = (int32_t)NULL;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[mqttsessioncreate-res]ret: %p\n", (MQTTSession_t *)ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (MQTTSession_t *)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_mqttSessionCreate() create a new client session to deal with mqtt portal.
 *
 * @param [in] sessionParam: The session initialize parameters to create a new client session
 * see @ref MQTTSessionParams_t.
 *
 * @return On success, a non-NULL session handle returned, see @ref MQTTSession_t;
 * On failure, NULL returned.
 */

MQTTSession_t *altcom_mqttSessionCreate(MQTTSessionParams_t *sessionParam) {
  /* Check parameters */
  if (NULL == sessionParam) {
    DBGIF_LOG_ERROR("Invalid session parameter\n");
    return NULL;
  }
  /* TODO: Check more conditions here */

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return NULL;
  }

  /* Send request */
  return mqttSessionCreate_request(sessionParam);
}