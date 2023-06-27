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

#ifndef __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMD_MQTTSESSIONCREATE_H
#define __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMD_MQTTSESSIONCREATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_mqtt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MQTT_CREATESESSION_REQSTR_MAXLEN                  \
  ((MQTTCFG_MAX_URL_LEN + MQTTCFG_MAX_CLIENTID_LEN * 3) + \
   (MQTTCFG_MAX_WILLTOPIC_LEN + MQTTCFG_MAX_WILLMSG_LEN))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure describes the data structure of the API command */
begin_packed_struct struct apicmd_mqttsessioncreate_s {
  /* Exist session handle */
  uint32_t existSession;
  /* MQTTParamNodes_t */
  uint16_t urlLen;
  uint16_t clientIdLen;
  uint16_t usernameLen;
  uint16_t passwordLen;
  /* MQTTParamTLS_t */
  uint8_t authMode;
  uint8_t authContext;
  /* MQTTParamIP_t */
  uint8_t pdnSessionId;
  uint8_t ipType;
  uint32_t destPort;
  uint32_t sourcePort;
  /* MQTTParamWillMsg_t */
  uint8_t enableWillMsg;
  uint8_t willQoS;
  uint8_t willRetain;
  uint16_t willTopicLen;
  uint16_t willMsgLen;
  /* MQTTParamProtocol_t */
  uint8_t protocolType;
  uint32_t keepAliveTime;
  uint8_t cleanSessionFlag;
  uint8_t sessionType;
  char strData[MQTT_CREATESESSION_REQSTR_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_mqttsessioncreateres_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMD_MQTTSESSIONCREATE_H */
