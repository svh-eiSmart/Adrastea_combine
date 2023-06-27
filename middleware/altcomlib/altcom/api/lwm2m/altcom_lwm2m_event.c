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

#include "dbg_if.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apiutil.h"
#include "apicmdhdlrbs.h"
#include "altcom_osal.h"
#include "apicmd.h"
#include "apicmdhdlr_lwm2mevt.h"
/****************************************************************************
 * External Functions
 ****************************************************************************/
extern LWM2MError_e altcom_lwm2mServerResp(client_inst_t client, int seqNum, char *respCode);
extern LWM2MError_e altcom_lwm2mReadResp(client_inst_t client, int seqNum, int objectId,
                                         int instanceNum, int resourceNum, int resourceInst,
                                         bool readResult, LWM2MRsrc_type_e valType,
                                         resourceVal_t *resourceVal);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
lwm2m_opev_report_cb_t g_lwm2m_opev_report_cb = NULL;
void *g_lwm2m_opev_report_cbpriv = NULL;
lwm2m_obsrvev_report_cb_t g_lwm2m_obsrvstart_report_cb = NULL;
void *g_lwm2m_obsrvstart_report_cbpriv = NULL;
lwm2m_obsrvev_report_cb_t g_lwm2m_obsrvstop_report_cb = NULL;
void *g_lwm2m_obsrvstop_report_cbpriv = NULL;
lwm2m_object_read_req_cb_t g_lwm2m_read_req_cb = NULL;
void *g_lwm2m_readreq_cbpriv = NULL;
lwm2m_object_write_req_cb_t g_lwm2m_write_req_cb = NULL;
void *g_lwm2m_writereq_cbpriv = NULL;
lwm2m_object_exe_req_cb_t g_lwm2m_exe_req_cb = NULL;
void *g_lwm2m_exereq_cbpriv = NULL;

static void repevt_observe_start_report(FAR lwm2m_event_int_t *event) {
  int seqNum;
  uint16_t objId, instId, rsrcId, rsrcInstId;
  uint8_t condValidMask;
  uint32_t minPeriod, maxPeriod;
  double gtCond, ltCond, stepVal;
  client_inst_t client;
  lwm2m_obsrvev_report_cb_t callback;
  void *cbpriv;
  lwm2m_event_t lwm2m_event;

  client = (client_inst_t)event->client;
  seqNum = ntohl(event->u.lwm2m_obserStartReq.seqNum);
  objId = ntohs(event->u.lwm2m_obserStartReq.objId);
  instId = ntohs(event->u.lwm2m_obserStartReq.instId);
  rsrcId = ntohs(event->u.lwm2m_obserStartReq.rsrcId);
  rsrcInstId = ntohs(event->u.lwm2m_obserStartReq.rsrcInstId);
  condValidMask = event->u.lwm2m_obserStartReq.condValidMask;
  minPeriod = ntohl(event->u.lwm2m_obserStartReq.minPeriod);
  maxPeriod = ntohl(event->u.lwm2m_obserStartReq.maxPeriod);
  gtCond = ntohd(event->u.lwm2m_obserStartReq.gtCond);
  ltCond = ntohd(event->u.lwm2m_obserStartReq.ltCond);
  stepVal = ntohd(event->u.lwm2m_obserStartReq.stepVal);

  DBGIF_LOG2_DEBUG("Got observe start req for objId=%hu instId=%hu\r\n", objId, instId);
  DBGIF_LOG2_DEBUG("Got observe start req for rsrcId=%hu rsrcInstId=%hu\r\n", rsrcId, rsrcInstId);
  DBGIF_LOG3_DEBUG("Got observe start req for condValidMask=0x%X minPeriod=%lu maxPeriod=%lu\r\n",
                   (unsigned int)condValidMask, minPeriod, maxPeriod);
  DBGIF_LOG3_DEBUG("Got observe start req for gtCond=%f ltCond=%f stepVal=%f\r\n", gtCond, ltCond,
                   stepVal);
  DBGIF_LOG3_DEBUG("Got observe start req client=%lu seqNum=%d with token=%s\r\n", (uint32_t)client,
                   seqNum, event->u.lwm2m_obserStartReq.token);
  // 1st send resp ack  to LWM2M server.
  altcom_lwm2mServerResp(client, seqNum, "2.05");

  // Signal to upper layer the event occured
  ALTCOM_GET_CALLBACK(g_lwm2m_obsrvstart_report_cb, callback, g_lwm2m_obsrvstart_report_cbpriv,
                      cbpriv);
  if (callback) {
    memset(&lwm2m_event, 0x0, sizeof(lwm2m_event_t));
    lwm2m_event.client = client;
    lwm2m_event.eventType = EVENT_OBSERVE_START;
    lwm2m_event.u.observeInfo.objId = objId;
    lwm2m_event.u.observeInfo.instId = instId;
    lwm2m_event.u.observeInfo.rsrcId = rsrcId;
    lwm2m_event.u.observeInfo.rsrcInstId = rsrcInstId;
    lwm2m_event.u.observeInfo.condValidMask = condValidMask;
    lwm2m_event.u.observeInfo.minPeriod = minPeriod;
    lwm2m_event.u.observeInfo.maxPeriod = maxPeriod;
    lwm2m_event.u.observeInfo.gtCond = gtCond;
    lwm2m_event.u.observeInfo.ltCond = ltCond;
    lwm2m_event.u.observeInfo.stepVal = stepVal;
    size_t tklen = strlen(event->u.lwm2m_obserStartReq.token) + 1;
    strncpy(lwm2m_event.u.observeInfo.token, event->u.lwm2m_obserStartReq.token, tklen);
    callback(client, &lwm2m_event, cbpriv);
  }
}

static void repevt_observe_stop_report(FAR lwm2m_event_int_t *event) {
  int seqNum;
  uint16_t objId, instId, rsrcId, rsrcInstId;
  client_inst_t client;
  lwm2m_obsrvev_report_cb_t callback;
  void *cbpriv;
  lwm2m_event_t lwm2m_event;

  client = (client_inst_t)event->client;
  seqNum = ntohl(event->u.lwm2m_obserStopReq.seqNum);
  objId = ntohs(event->u.lwm2m_obserStopReq.objId);
  instId = ntohs(event->u.lwm2m_obserStopReq.instId);
  rsrcId = ntohs(event->u.lwm2m_obserStopReq.rsrcId);
  rsrcInstId = ntohs(event->u.lwm2m_obserStopReq.rsrcInstId);

  DBGIF_LOG2_DEBUG("Got observe stop req for objId=%hu instId=%hu\r\n", objId, instId);
  DBGIF_LOG2_DEBUG("Got observe stop req for rsrcId=%hu rsrcInstId=%hu\r\n", rsrcId, rsrcInstId);
  DBGIF_LOG3_DEBUG("Got observe stop req client=%lu seqNum=%d with token=%s\r\n", (uint32_t)client,
                   seqNum, event->u.lwm2m_obserStopReq.token);
  // 1st send resp ack  to LWM2M server.
  altcom_lwm2mServerResp(client, seqNum, "2.05");

  // Signal to upper layer the event occured
  ALTCOM_GET_CALLBACK(g_lwm2m_obsrvstop_report_cb, callback, g_lwm2m_obsrvstop_report_cbpriv,
                      cbpriv);
  if (callback) {
    memset(&lwm2m_event, 0x0, sizeof(lwm2m_event_t));
    lwm2m_event.client = client;
    lwm2m_event.eventType = EVENT_OBSERVE_STOP;
    lwm2m_event.u.observeInfo.objId = objId;
    lwm2m_event.u.observeInfo.instId = instId;
    lwm2m_event.u.observeInfo.rsrcId = rsrcId;
    lwm2m_event.u.observeInfo.rsrcInstId = rsrcInstId;

    size_t tklen = strlen(event->u.lwm2m_obserStartReq.token) + 1;
    strncpy(lwm2m_event.u.observeInfo.token, event->u.lwm2m_obserStopReq.token, tklen);
    callback(client, &lwm2m_event, cbpriv);
  }
}

static void repevt_opev_report(FAR lwm2m_event_int_t *event) {
  int evNum, servId;
  client_inst_t client;
  lwm2m_opev_report_cb_t callback;
  void *cbpriv;
  lwm2m_event_t lwm2m_event;

  client = (client_inst_t)event->client;
  evNum = ntohs(event->u.lwm2m_OpevUrc.eveNum);
  servId = ntohs(event->u.lwm2m_OpevUrc.serverId);

  DBGIF_LOG3_DEBUG("Got opev client=%lu evNum=%d servId=%d\r\n", (uint32_t)client, evNum, servId);
  if (evNum < LWM2MOPEV_EVENT_ALL) {
    ALTCOM_GET_CALLBACK(g_lwm2m_opev_report_cb, callback, g_lwm2m_opev_report_cbpriv, cbpriv);
    if (callback) {      
      memset(&lwm2m_event, 0, sizeof(lwm2m_event_t));
      lwm2m_event.client = client;
      lwm2m_event.eventType = EVENT_OPEV;
      lwm2m_event.u.opev_sub_evt = (lwm2m_opev_type_t)evNum;
      callback(client, &lwm2m_event, cbpriv);
    } else {
      DBGIF_LOG2_ERROR("Event was dumped due to missing CB opev  evNum=%d servId=%d r\n", evNum,
                       servId);
    }
  } else {
    DBGIF_LOG1_ERROR("Unexpected LWM2M opev num :%d.\n", evNum);
  }
}

static void repevt_writeReq_report(FAR lwm2m_event_int_t *event) {
  int objectId, instanceNum, resourceNum;
  client_inst_t client;
  int resourceInst;
  LWM2MInstnc_type_e instncType;
  lwm2m_object_write_req_cb_t callback;
  void *cbpriv;

  client = (client_inst_t)event->client;
  objectId = ntohs(event->u.lwm2m_writeReq.objectId);
  instanceNum = ntohs(event->u.lwm2m_writeReq.instanceNum);
  resourceNum = ntohs(event->u.lwm2m_writeReq.resourceNum);
  instncType = (LWM2MInstnc_type_e)ntohs(event->u.lwm2m_writeReq.instncType);
  resourceInst = ntohs(event->u.lwm2m_writeReq.resourceInst);

  int16_t valueLen = ntohs(event->u.lwm2m_writeReq.valueLen);

  DBGIF_LOG3_DEBUG("write req ev serverId=%hd seqNum=%ld objectId=%d \r\n",
                   ntohs(event->u.lwm2m_writeReq.serverId), ntohl(event->u.lwm2m_writeReq.seqNum),
                   objectId);
  DBGIF_LOG3_DEBUG("write req ev instanceNum=%d resourceNum=%d valueLen=%hd\r\n", instanceNum,
                   resourceNum, valueLen);

  char *placeHolterPtr = (char *)&event->u.lwm2m_writeReq.valueAsString;
  bool writeResult = false;

  ALTCOM_GET_CALLBACK(g_lwm2m_write_req_cb, callback, g_lwm2m_writereq_cbpriv, cbpriv);
  if (callback) {
    int ret = callback(client, objectId, instanceNum, resourceNum, resourceInst, instncType,
                       placeHolterPtr, valueLen, cbpriv);
    if (ret == 0) {
      writeResult = true;
    }
  }

  // send resp ack  to LWM2M server.
  if (writeResult == true) {
    altcom_lwm2mServerResp(client, ntohl(event->u.lwm2m_writeReq.seqNum), "2.04");
  } else {
    altcom_lwm2mServerResp(client, ntohl(event->u.lwm2m_writeReq.seqNum),
                           "4.06" /* Not Acceptable*/);
  }
}

static void repevt_exeReq_report(FAR lwm2m_event_int_t *event) {
  int objectId, instanceNum, resourceNum;
  client_inst_t client;
  int resourceInst;
  lwm2m_object_exe_req_cb_t callback;
  void *cbpriv;

  client = (client_inst_t)event->client;
  objectId = ntohs(event->u.lwm2m_exeReq.objId);
  instanceNum = ntohs(event->u.lwm2m_exeReq.instId);
  resourceNum = ntohs(event->u.lwm2m_exeReq.rsrcId);
  resourceInst = ntohs(event->u.lwm2m_exeReq.rsrcInstId);

  DBGIF_LOG3_DEBUG("exc req ev serverId=%hd seqNum=%ld objectId=%d \r\n",
                   (uint16_t)ntohs(event->u.lwm2m_exeReq.serverId),
                   (uint16_t)ntohs(event->u.lwm2m_exeReq.seqNum), objectId);

  DBGIF_LOG3_DEBUG("exe req ev instanceNum=%d resourceNum=%d param=\"%s\"\r\n", instanceNum,
                   resourceNum, &event->u.lwm2m_exeReq.param);

  bool exeResult = false;
  ALTCOM_GET_CALLBACK(g_lwm2m_exe_req_cb, callback, g_lwm2m_exereq_cbpriv, cbpriv);
  if (callback) {
    int ret = callback(client, objectId, instanceNum, resourceNum, resourceInst,
                       &event->u.lwm2m_exeReq.param, cbpriv);
    if (ret == 0) {
      exeResult = true;
    }
  }

  // send resp ack to LWM2M server.
  if (exeResult == true) {
    altcom_lwm2mServerResp(client, (int)ntohs(event->u.lwm2m_exeReq.seqNum), "2.04");
  } else {
    altcom_lwm2mServerResp(client, (int)ntohs(event->u.lwm2m_exeReq.seqNum),
                           "4.00" /* Bad Request*/);
  }
}

static void repevt_readReq_report(FAR lwm2m_event_int_t *event) {
  int objectId, instanceNum, resourceNum;
  client_inst_t client;
  LWM2MRsrc_type_e valType = RSRC_VALUE_TYPE_FLOAT;
  int resourceInst = 0;
  lwm2m_object_read_req_cb_t callback;
  resourceVal_t resourceVal;
  void *cbpriv;
  bool readResult = true;

  memset(&resourceVal, 0, sizeof(resourceVal_t));

  DBGIF_LOG2_DEBUG("read req ev serverId=%hd seqNum=%ld\r\n",
                   ntohs(event->u.lwm2m_readReq.serverId), ntohl(event->u.lwm2m_readReq.seqNum));
  client = (client_inst_t)event->client;
  objectId = ntohs(event->u.lwm2m_readReq.objectId);
  instanceNum = ntohs(event->u.lwm2m_readReq.instanceNum);
  resourceNum = ntohs(event->u.lwm2m_readReq.resourceNum);
  DBGIF_LOG3_DEBUG("Got read read req client=%lu objectId=%d instanceNum=%d\r\n", (uint32_t)client,
                   objectId, instanceNum);
  DBGIF_LOG1_DEBUG("Got read read req resourceNum=%d\r\n", resourceNum);

  ALTCOM_GET_CALLBACK(g_lwm2m_read_req_cb, callback, g_lwm2m_readreq_cbpriv, cbpriv);
  if (callback) {
    int ret = callback(client, objectId, instanceNum, resourceNum, &resourceInst, &valType,
                       &resourceVal, cbpriv);
    if (ret != 0) {
      readResult = false;
    }
  } else {
    readResult = false;
  }

  altcom_lwm2mReadResp(client, ntohl(event->u.lwm2m_readReq.seqNum), objectId, instanceNum,
                       resourceNum, resourceInst, readResult, valType, &resourceVal);
}

static void repevt_job(FAR void *arg) {
  FAR lwm2m_event_int_t *evt = (FAR lwm2m_event_int_t *)arg;

  DBGIF_ASSERT(NULL != evt, "invalid event\r\n");

  switch ((lwm2m_event_type_t)evt->eventType) {
    case EVENT_OPEV:
      repevt_opev_report(evt);
      break;
    case EVENT_READ_REQ:
      repevt_readReq_report(evt);
      break;
    case EVENT_WRITE_REQ:
      repevt_writeReq_report(evt);
      break;
    case EVENT_EXE_REQ:
      repevt_exeReq_report(evt);
      break;
    case EVENT_OBSERVE_START:
      repevt_observe_start_report(evt);
      break;
    case EVENT_OBSERVE_STOP:
      repevt_observe_stop_report(evt);
      break;
    default:
      DBGIF_LOG1_ERROR("Unknown LWM2M event type :%d.\n", (int)evt->eventType);
      break;
  }

  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
enum evthdlrc_e apicmdhdlr_lwm2mMessageEvt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_LWM2M_URC, repevt_job);
}

/**
 * @brief altcom_lwm2mGetServerInfo get the server information from MAP side.
 *
 * @param [in] event_type: Callback function to handle the Specify the target client instance to
 * operate.
 * @param [in] callback: Callback function to handle the Specify the target client instance to
 * operate.
 * @param [in]] userPriv: Server information struct ptr
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 */

LWM2MError_e altcom_lwm2mSetEventCallback(lwm2m_event_type_t event_type, void *callback,
                                          void *userPriv) {
  void **regcb;
  void **regpriv;
  int ret = 0;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  switch (event_type) {
    case EVENT_READ_REQ:
      regcb = (void *)&g_lwm2m_read_req_cb;
      regpriv = &g_lwm2m_readreq_cbpriv;
      break;

    case EVENT_OBSERVE_START:
      regcb = (void *)&g_lwm2m_obsrvstart_report_cb;
      regpriv = &g_lwm2m_obsrvstart_report_cbpriv;
      break;

    case EVENT_OPEV:
      regcb = (void *)&g_lwm2m_opev_report_cb;
      regpriv = &g_lwm2m_opev_report_cbpriv;
      break;

    case EVENT_WRITE_REQ:
      regcb = (void *)&g_lwm2m_write_req_cb;
      regpriv = &g_lwm2m_writereq_cbpriv;
      break;
    case EVENT_EXE_REQ:
      regcb = (void *)&g_lwm2m_exe_req_cb;
      regpriv = &g_lwm2m_exereq_cbpriv;
      break;

    case EVENT_OBSERVE_STOP:
      regcb = (void *)&g_lwm2m_obsrvstop_report_cb;
      regpriv = &g_lwm2m_obsrvstop_report_cbpriv;
      break;

    default:
      DBGIF_LOG1_ERROR("Unknown LWM2M event type :%d.\n", (int)event_type);
      return LWM2M_FAILURE;
  }

  ALTCOM_CLR_CALLBACK(*regcb, *regpriv);
  if (callback) {
    ALTCOM_REG_CALLBACK(ret, *regcb, callback, *regpriv, userPriv);
  }

  if (ret) {
    DBGIF_LOG2_ERROR("LWM2M callback registration failed with event: %d, ret: %d.\r\n",
                     (int)event_type, ret);
    return LWM2M_FAILURE;
  } else {
    return LWM2M_SUCCESS;
  }
}
