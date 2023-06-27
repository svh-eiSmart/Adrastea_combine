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
#include <stdbool.h>

#include "dbg_if.h"
#include "apicmd_mqttPublish.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MQTTPUBLISH_REQ_DATALEN (sizeof(struct apicmd_mqttpublish_s))
#define MQTTPUBLISH_RES_DATALEN (sizeof(struct apicmd_mqttpublishres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct mqttpublish_req_s {
  char *topic;
  char *message;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mqttPublish_request
 *
 * Description:
 *   Send APICMDID_VFS_OPEN.
 *
 ****************************************************************************/

static int mqttPublish_request(MQTTSession_t *session, unsigned int qos, unsigned int retain,
                               const char *topic, unsigned short topicLen, const char *msg,
                               unsigned short msgLen) {
  int32_t ret;
  uint16_t resLen = 0;
  int32_t sendLen;
  FAR struct apicmd_mqttpublish_s *cmd = NULL;
  FAR struct apicmd_mqttpublishres_s *res = NULL;

  /* Allocate send and response command buffer */
  sendLen = topicLen + msgLen + 1;
  if (sendLen > MQTT_PUBLISH_REQSTR_MAXLEN) {
    DBGIF_LOG1_ERROR("[mqttPublish_request]publish str length too long: %ld\n", sendLen);
    return MQTT_FAILURE;
  }

  sendLen += MQTTPUBLISH_REQ_DATALEN - MQTT_PUBLISH_REQSTR_MAXLEN;
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MQTT_PUBLISH, sendLen,
                                          (FAR void **)&res, MQTTPUBLISH_RES_DATALEN)) {
    DBGIF_LOG_ERROR("mem alloc failed\n");
    return MQTT_FAILURE;
  }

  /* Fill the data */
  cmd->session = htonl((uint32_t)session);
  cmd->qos = (uint8_t)qos;
  cmd->retain = (uint8_t)retain;
  memcpy(cmd->publishData, topic, topicLen + 1);
  DBGIF_LOG1_DEBUG("topicLen + 1: %hu\n", topicLen + 1);
  cmd->topicLen = htons(topicLen + 1);
  memcpy(cmd->publishData + topicLen + 1, msg, msgLen);
  DBGIF_LOG1_DEBUG("msgLen: %hu\n", msgLen);
  cmd->messageLen = htons(msgLen);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, MQTTPUBLISH_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = MQTT_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != MQTTPUBLISH_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = MQTT_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (MQTT_FAILURE == ret) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[mqttpublish-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_mqttPublish() publish a message on the specific topic to MQTT portal.
 *
 * @param [in] session: The target session to be published, see @ref MQTTSession_t.
 * @param [in] qos: The qos of publishing message; see @ref MQTTQoS_e.
 * @param [in] retain: Need to retain the publishing message.
 * @param [in] topic: The specific topic to be published(must be null-terminated).
 * @param [in] msg: The message to be published(string or binary array)
 * @param [in] msgLen: The length of message to be published
 *
 * @return message Id on success, 0: not in used, 1-65535(MQTT-SN only);  MQTT_FAILURE on failure.
 *
 */

int altcom_mqttPublish(MQTTSession_t *session, MQTTQoS_e qos, unsigned int retain,
                       const char *topic, const char *msg, unsigned short msgLen) {
  unsigned short topicLen;
  /* Check parameters */
  if (NULL == session) {
    DBGIF_LOG_ERROR("Invalid session\n");
    return MQTT_FAILURE;
  }

  if (qos > QOS2) {
    DBGIF_LOG1_ERROR("Invalid qos, %u\n", qos);
    return MQTT_FAILURE;
  }

  if (NULL == topic) {
    DBGIF_LOG_ERROR("Null topic\n");
    return MQTT_FAILURE;
  }

  topicLen = strlen(topic);
  if (0 == topicLen || topicLen > MQTTCFG_MAX_TOPIC_LEN - 1) {
    DBGIF_LOG1_ERROR("Invalid topicLen = %hu\n", topicLen);
    return MQTT_FAILURE;
  }

  if (NULL == msg || 0 == msgLen || msgLen > MQTTCFG_MAX_MSG_LEN - 1) {
    DBGIF_LOG2_ERROR("Invalid mesage %p, msgLen = %hu\n", msg, msgLen);
    return MQTT_FAILURE;
  }

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    altcom_seterrno(ALTCOM_EINVAL);
    return MQTT_FAILURE;
  }

  /* Send request */
  return mqttPublish_request(session, qos, retain, topic, topicLen, msg, msgLen);
}
