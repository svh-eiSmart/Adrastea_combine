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
#include "aws/altcom_aws.h"
#include "osal/altcom_osal.h"
#include "util/apiutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_CONCONF_TIMEOUT_MS 20000

/****************************************************************************
 * External Data
 ****************************************************************************/
extern bool gAwsInit;
extern MQTTSession_t *gAwsSession;
extern void *gAwsUserPriv;
extern altcom_sys_mq_t gAwsEvtQue[];

/****************************************************************************
 * External Function
 ****************************************************************************/
extern AWSError_e awsCheckApiStatus(altcom_sys_mq_t *, int32_t);
extern void mqttCallback(MQTTSession_t *, MqttEvent_e, MqttResultData_t *, void *);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * Name: altcom_awsConnect
 *
 *   altcom_awsConnect() connect the client to aws.
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsConnect(void) {
  MQTTError_e ret;

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return AWS_FAILURE;
  }

  if (!gAwsInit) {
    DBGIF_LOG_ERROR("AWS APIs not initialized\n");
    return AWS_FAILURE;
  }

  if (!gAwsSession) {
    DBGIF_LOG_ERROR("Invalid AWS session handle\n");
    return AWS_FAILURE;
  }

  ret = altcom_mqttConnect(gAwsSession, mqttCallback, gAwsUserPriv);
  if (ret != MQTT_SUCCESS) {
    DBGIF_LOG1_ERROR("altcom_mqttConnect fail, ret = %d\n", ret);
    return AWS_FAILURE;
  }

  return awsCheckApiStatus(&gAwsEvtQue[MQTT_EVT_CONCONF], MAX_CONCONF_TIMEOUT_MS);
}
