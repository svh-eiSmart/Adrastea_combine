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

#ifndef __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMDHDLR_MESSAGEEVT_H
#define __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMDHDLR_MESSAGEEVT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "evthdl_if.h"
#include "altcom_mqtt.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
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

enum evthdlrc_e apicmdhdlr_mqttMessageEvt(FAR uint8_t *evt, uint32_t evlen);

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

MQTTError_e mqttHelper_RegisterCallback(MQTTSession_t *session, MQTTEvtCbFunc_t cbFunc,
                                        void *cbParam);

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

void mqttHelper_ClearAllCallback(void);

#endif /* __MODULES_ALTCOM_INCLUDE_API_MQTT_APICMDHDLR_MESSAGEEVT_H */
