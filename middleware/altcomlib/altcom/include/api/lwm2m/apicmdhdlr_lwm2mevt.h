/****************************************************************************
 * modules/lte/altcom/include/api/lte/apicmdhdlr_repevt.h
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

#ifndef __MODULES_LWM2M_ALTCOM_INCLUDE_API_LWM2M_APICMDHDLR_EVT_H
#define __MODULES_LWM2M_ALTCOM_INCLUDE_API_LWM2M_APICMDHDLR_EVT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "evthdl_if.h"
#include "altcom_cc.h"
#include "lwm2m/altcom_lwm2m.h"

/**
 * @brief Definition of LWM2m2 opev message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_OpevUrc_T {
  int16_t eveNum;  // of type lwm2m_opev_type_t
  int16_t serverId;
} end_packed_struct;
typedef struct lwm2m_OpevUrc_T lwm2m_OpevUrc_t;

/**
 * @brief Definition of LWM2m2 write req message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_writeReq_T {
  int32_t seqNum;
  int16_t serverId;
  int16_t objectId;
  int16_t instanceNum;
  int16_t resourceNum;
  int16_t instncType;
  int16_t resourceInst;
  int16_t valueLen;
  char *valueAsString;
} end_packed_struct;
typedef struct lwm2m_writeReq_T lwm2m_writeReq_t;

/**
 * @brief Definition of LWM2m2 read req message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_readReq_T {
  int32_t seqNum;
  int16_t serverId;
  int16_t objectId;
  int16_t instanceNum;
  int16_t resourceNum;
} end_packed_struct;
typedef struct lwm2m_readReq_T lwm2m_readReq_t;

/**
 * @brief Definition of LWM2m2 execute req message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_exeReq_T {
  uint16_t seqNum;
  uint16_t serverId;
  uint16_t objId;
  uint16_t instId;
  uint16_t rsrcId;
  uint16_t rsrcInstId;
  char param;
} end_packed_struct;
typedef struct lwm2m_exeReq_T lwm2m_exeReq_t;

/**
 * @brief Definition of LWM2m2 observe start req message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_obserStartReq_T {
  int32_t seqNum;
  int32_t serverId;
  char token[LWM2M_OBSERVE_TOKEN_STR_LEN + 1];  // Up to 8 bytes hexdecimal + null termination
  int32_t format;
  // uri detailed
  uint16_t objId;
  uint16_t instId;
  uint16_t rsrcId;
  uint16_t rsrcInstId;
  // observe condition
  uint8_t condValidMask;
  uint32_t minPeriod;
  uint32_t maxPeriod;
  double gtCond;
  double ltCond;
  double stepVal;
} end_packed_struct;
typedef struct lwm2m_obserStartReq_T lwm2m_obserStartReq_t;

/**
 * @brief Definition of LWM2m2 observe stop req message.
 * This is notified by event_report_cb_t callback function
 */
begin_packed_struct struct lwm2m_obserStopReq_T {
  int32_t seqNum;
  int32_t serverId;
  char token[LWM2M_OBSERVE_TOKEN_STR_LEN + 1];  // Up to 8 bytes hexdecimal + null termination
  int32_t format;
  // uri detailed
  uint16_t objId;
  uint16_t instId;
  uint16_t rsrcId;
  uint16_t rsrcInstId;
} end_packed_struct;
typedef struct lwm2m_obserStopReq_T lwm2m_obserStopReq_t;

/**
 * @brief Definition of event structure
 */

begin_packed_struct struct lwm2m_event_int_T {
  uint8_t client;
  uint8_t eventType; /**< Event type */
  union {
    lwm2m_OpevUrc_t lwm2m_OpevUrc;             /**< Operation event data */
    lwm2m_readReq_t lwm2m_readReq;             /**< Read req event data */
    lwm2m_writeReq_t lwm2m_writeReq;           /**< Write req event data */
    lwm2m_exeReq_t lwm2m_exeReq;               /**< Executable req event type */
    lwm2m_obserStartReq_t lwm2m_obserStartReq; /**< Observe start req event data */
    lwm2m_obserStopReq_t lwm2m_obserStopReq;   /**< Observe stop req event data */
  } u;
} end_packed_struct;
typedef struct lwm2m_event_int_T lwm2m_event_int_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/****************************************************************************
 * Name: apicmdhdlr_lwm2mMessageEvt
 *
 * Description:
 *   This function is an API command handler for event report.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_REPORT_EVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_lwm2mMessageEvt(FAR uint8_t *evt, uint32_t evlen);

#endif /* __MODULES_LWM2M_ALTCOM_INCLUDE_API_LWM2M_APICMDHDLR_EVT_H */
