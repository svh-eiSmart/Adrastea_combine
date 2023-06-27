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
#include "altcom_mqtt.h"
#include "apicmd_mqttSubscribe.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MQTTSUBSCRIBE_REQ_DATALEN (sizeof(struct apicmd_mqttsubscribe_s))
#define MQTTSUBSCRIBE_RES_DATALEN (sizeof(struct apicmd_mqttsubscriberes_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mqttSubscribe_request
 *
 * Description:
 *   Send APICMDID_MQTT_SUBSCRIBE.
 *
 ****************************************************************************/

static int mqttSubscribe_request(MQTTSession_t *session, MQTTQoS_e qos, const char *topic) {
  int32_t ret;
  uint16_t resLen = 0;
  int32_t sendLen;
  FAR struct apicmd_mqttsubscribe_s *cmd = NULL;
  FAR struct apicmd_mqttsubscriberes_s *res = NULL;

  /* Allocate send and response command buffer */
  sendLen = strlen(topic) + 1;
  if (sendLen > MQTTCFG_MAX_TOPIC_LEN) {
    DBGIF_LOG1_ERROR("[mqttSubscribe_request]topic too long: %ld\n", sendLen);
    return MQTT_FAILURE;
  }

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MQTT_SUBSCRIBE,
                                          MQTTSUBSCRIBE_REQ_DATALEN, (FAR void **)&res,
                                          MQTTSUBSCRIBE_RES_DATALEN)) {
    return MQTT_FAILURE;
  }

  /* Fill the data */
  cmd->session = htonl((uint32_t)session);
  cmd->qos = (uint8_t)qos;
  strncpy(cmd->topic, topic, MQTTCFG_MAX_TOPIC_LEN - 1);
  DBGIF_LOG1_DEBUG("topic: %s\n", cmd->topic);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, MQTTSUBSCRIBE_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = MQTT_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != MQTTSUBSCRIBE_RES_DATALEN) {
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

  DBGIF_LOG1_DEBUG("[mqttsubscribe-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_mqttSubscribe() subscribe a specific topic to MQTT portal and hookup a notifying
 * callback.
 *
 * @param [in] session: The target session to be subscribed, see @ref MQTTSession_t.
 * @param [in] qos: The qos of subscribing message; see @ref MQTTQoS_e.
 * @param [in] topic: A specific topic to be subscribed(must be null-terminated).
 *
 * @return message Id on success, 0: not in used, 1-65535(MQTT-SN only);  MQTT_FAILURE on failure.
 */

int altcom_mqttSubscribe(MQTTSession_t *session, MQTTQoS_e qos, const char *topic) {
  /* Check parameters */
  if (NULL == session) {
    DBGIF_LOG_ERROR("Invalid session\n");
    return MQTT_FAILURE;
  }

  if (NULL == topic || 0 == strlen(topic)) {
    DBGIF_LOG_ERROR("Invalid topic\n");
    return MQTT_FAILURE;
  }

  if (qos > QOS2) {
    DBGIF_LOG1_ERROR("Invalid qos, %u\n", qos);
    return MQTT_FAILURE;
  }

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return MQTT_FAILURE;
  }

  /* Send request */
  return mqttSubscribe_request(session, qos, topic);
}
