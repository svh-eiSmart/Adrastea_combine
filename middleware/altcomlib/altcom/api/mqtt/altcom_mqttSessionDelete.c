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
#include "apicmd_mqttSessionDelete.h"
#include "apicmdhdlr_mqttMessageEvt.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MQTTSESSIONDELETE_REQ_DATALEN (sizeof(struct apicmd_mqttsessiondelete_s))
#define MQTTSESSIONDELETE_RES_DATALEN (sizeof(struct apicmd_mqttsessiondeleteres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mqttSessionDelete_request
 *
 * Description:
 *   Send APICMDID_MQTT_SESSIONDELETE.
 *
 ****************************************************************************/

static MQTTError_e mqttSessionDelete_request(MQTTSession_t *session) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_mqttsessiondelete_s *cmd = NULL;
  FAR struct apicmd_mqttsessiondeleteres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_MQTT_SESSIONDELETE,
                                          MQTTSESSIONDELETE_REQ_DATALEN, (FAR void **)&res,
                                          MQTTSESSIONDELETE_RES_DATALEN)) {
    return MQTT_FAILURE;
  }

  /* Fill the data */
  cmd->session = htonl((uint32_t)session);

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, MQTTSESSIONDELETE_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);
  /* Check GW return */
  if (ret < 0) {
    ret = MQTT_FAILURE;
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (resLen != MQTTSESSIONDELETE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = MQTT_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != MQTT_SUCCESS) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    ret = MQTT_FAILURE;
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[mqttsessiondelete-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (MQTTError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_mqttSessionDelete() delete an exist client session.
 *
 * @param [in] session: The target session to be deleted, see @ref MQTTSession_t.
 *
 * @return MQTT_SUCCESS on success;  MQTT_FAILURE on failure.
 */

MQTTError_e altcom_mqttSessionDelete(MQTTSession_t *session) {
  MQTTError_e ret;
  /* Check parameters */
  if (NULL == session) {
    DBGIF_LOG_ERROR("Invalid session\n");
    return MQTT_FAILURE;
  }
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return MQTT_FAILURE;
  }

  /* Send request */
  ret = mqttSessionDelete_request(session);
  mqttHelper_RegisterCallback(session, NULL, NULL);
  return ret;
}
