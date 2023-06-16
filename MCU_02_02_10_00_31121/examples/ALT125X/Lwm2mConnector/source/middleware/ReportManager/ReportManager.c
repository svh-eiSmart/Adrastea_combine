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
/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Kernel Header */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

/* Infra Header */
#include "Infra/app_dbg.h"

/* Middleware Header */
#include "altcom.h"
#include "lte/lte_api.h"
#include "gps/altcom_gps.h"
#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "lwm2m/altcom_lwm2m.h"
#include "misc/altcom_misc.h"
#include "atcmd/altcom_atcmd.h"
#include "apicmdhdlr_lwm2mevt.h"
#include "osal/osal_stateMachine.h"
#include "osal/osal_timers.h"
#include "ReportManager/ReportManager.h"
#include "ReportManager/ReportManagerAPI.h"
#include "ReportManager/ReportManagerDb.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define THE_EXISTING_SERVER_ID (-1)
#define FACTORY_BS_NAME "mcusdk-bootstrap"

/****************************************************************************
 * External Data
 ****************************************************************************/
extern client_inst_t lwm2m_client;

/****************************************************************************
 * Private Data
 ****************************************************************************/
// interface CB function to Application
static reportManagerStartCB_t reportManagerNotifyResultCB;
static int32_t reportManagerTransTimeout;
static report_type_bitmask_t observObjectBitSM = (report_type_bitmask_t)0,
                             observingObjectBitSM = (report_type_bitmask_t)0;

char *gREPORTMGREventNames[] = {
    "EVT_REPORTMGR_START",
    "EVT_REPORTMGR_REGISTER_ACK",
    "EVT_REPORTMGR_DEREGISTER_ACK",
    "EVT_REPORTMGR_OBSERVE_START",
    "EVT_REPORTMGR_OBSERVE_STOP",
    "EVT_REPORTMGR_CONFIRM_NOTIFY_FAIL",
    "EVT_REPORTMGR_CONFIRM_NOTIFY_ACK",
    "EVT_REPORTMGR_REGISTER_UPDT_ACK",
    "EVT_REPORTMGR_REQ_TO_SEND_RPRT",
    "EVT_REPORTMGR_TRANSC_TIMEOUT",
    "EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE",
    "EVT_REPORTMGR_NOTIFY_DISCARD",
    "EVT_REPORTMGR_MAX",
};

char *gREPORTMGRStateNames[] = {
    "STATE_REPORTMGR_IDLE",
    "STATE_REPORTMGR_WAIT_4_OBSERVE_START",
    "STATE_REPORTMGR_WAIT_4_REGISTER_ACK",
    "STATE_REPORTMGR_READY_FOR_NOTIFY_REQ",
    "STATE_REPORTMGR_NOTIFY_WAS_SENT",
    "STATE_REPORTMGR_REG_UPDT_WAS_SENT",
    "STATE_REPORTMGR_MAX",
};

static stateMachine_t gREPORTMGRCtx = {0};
static osalTimer_t *reprtMngrTranscTimer = NULL;
static char *altairCsServerUriPtr = "coap://192.168.10.6:5685";

static bool expectAckToreport = false;
static bool useBootStrapServer = true; /* for bootstrap mode*/
;                                      // false; //Temp !!!b
static unsigned int liftimeValue = 36000;
static bool observeWithoutRead = false;
static bool notifyWithoutRegUpdate = false;

/****************************************************************************
 * Private Function
 ****************************************************************************/
static void dump_resource(lwm2m_uri_and_data_t *uriData) {
  char *instTypStr[] = {"Single", "Multiple"};
  if (!uriData) {
    APP_DBG_PRINT("Invalid parameter\r\n");
    return;
  }

  switch (uriData->valType) {
    case RSRC_VALUE_TYPE_FLOAT:
      APP_DBG_PRINT("/%lu/%lu/%lu/%lu, type: %lu, insttype: %s, val: %f\r\n", uriData->objectId,
                    uriData->instanceNum, uriData->resourceNum, uriData->resourceInstanceNum,
                    (uint32_t)uriData->valType, instTypStr[uriData->LWM2MInstncType],
                    uriData->resourceVal.value);
      break;

    case RSRC_VALUE_TYPE_INTEGER:
    case RSRC_VALUE_TYPE_TIME:
    case RSRC_VALUE_TYPE_BOOLEAN:
      APP_DBG_PRINT("/%lu/%lu/%lu/%lu, type: %lu, insttype: %s, val: %d\r\n", uriData->objectId,
                    uriData->instanceNum, uriData->resourceNum, uriData->resourceInstanceNum,
                    (uint32_t)uriData->valType, instTypStr[uriData->LWM2MInstncType],
                    (int32_t)uriData->resourceVal.value);
      break;

    case RSRC_VALUE_TYPE_STRING:
      APP_DBG_PRINT("/%lu/%lu/%lu/%lu, type: %lu, insttype: %s, val: %s\r\n", uriData->objectId,
                    uriData->instanceNum, uriData->resourceNum, uriData->resourceInstanceNum,
                    (uint32_t)uriData->valType, instTypStr[uriData->LWM2MInstncType],
                    uriData->resourceVal.strVal);
      break;

    case RSRC_VALUE_TYPE_OPAQUE:
      APP_DBG_PRINT("/%lu/%lu/%lu/%lu type: %lu, insttype: %s, len: %hu\r\n", uriData->objectId,
                    uriData->instanceNum, uriData->resourceNum, uriData->resourceInstanceNum,
                    (uint32_t)uriData->valType, instTypStr[uriData->LWM2MInstncType],
                    uriData->resourceVal.opaqueVal.opaqueValLen);
      break;

    default:
      APP_ERR_PRINT("Invalid value type %d", (int)uriData->valType);
      break;
  }
}

static void reportMngr_obsrvev_report_cb(client_inst_t client, lwm2m_event_t *event,
                                         void *userPriv) {
  APP_DBG_PRINT("LWM2m Events CB function to APP was called with evNumType=%d subtype %d \r\n",
                event->eventType, event->u.opev_sub_evt);
  if (event->eventType == EVENT_OBSERVE_START || event->eventType == EVENT_OBSERVE_STOP) {
    APP_DBG_PRINT(
        "Evts CB fnct 2 APP with obsrv %s obj=%hu inst=%hu rsrc=%hu rsrcInst=%hu token=%s\r\n",
        event->eventType == EVENT_OBSERVE_START ? "start" : "stop", event->u.observeInfo.objId,
        event->u.observeInfo.instId, event->u.observeInfo.rsrcId, event->u.observeInfo.rsrcInstId,
        event->u.observeInfo.token);
    if (event->eventType == EVENT_OBSERVE_START) {
      APP_DBG_PRINT(
          "condValidMask=0x%X minPeriod=%lu maxPeriod=%lu gtCond=%f ltCond=%f stepVal=%f\r\n",
          event->u.observeInfo.condValidMask, event->u.observeInfo.minPeriod,
          event->u.observeInfo.maxPeriod, event->u.observeInfo.gtCond, event->u.observeInfo.ltCond,
          event->u.observeInfo.stepVal);
    }

    lwm2m_event_t *obsrvEvent = malloc(sizeof(lwm2m_event_t));
    APP_ASSERT(NULL != obsrvEvent, "malloc fail \n");

    *obsrvEvent = *event;
    EVT_SEND(&gREPORTMGRCtx,
             event->eventType == EVENT_OBSERVE_START ? EVT_REPORTMGR_OBSERVE_START
                                                     : EVT_REPORTMGR_OBSERVE_STOP,
             obsrvEvent);
  } else {
    APP_ERR_PRINT("Unexpected LWM2M event type :%d.\r\n", event->eventType);
  }
}

static void reportMngr_opev_report_cb(client_inst_t client, lwm2m_event_t *event, void *userPriv) {
  APP_UNUSED(userPriv);
  APP_DBG_PRINT("LWM2m Events CB function to APP was called with evNumType=%d subtype %d \r\n",
                event->eventType, event->u.opev_sub_evt);
  if (event->eventType == EVENT_OPEV) {
    lwm2m_event_t *opEvent = malloc(sizeof(lwm2m_event_t));
    APP_ASSERT(NULL != opEvent, "malloc fail \n");

    *opEvent = *event;
    if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_REGISTER_FINISHED) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_REGISTER_ACK, opEvent);
    } else if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_DEREGISTER_FINISHED) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_DEREGISTER_ACK, opEvent);
    } else if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_CONFIRM_NOTIFY_FAIL) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_CONFIRM_NOTIFY_FAIL, opEvent);
    } else if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_RSP) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_CONFIRM_NOTIFY_ACK, opEvent);
    } else if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_REGISTER_UPDATE_FINISHED) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_REGISTER_UPDT_ACK, opEvent);
    } else if (event->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_DISCARD) {
      EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_NOTIFY_DISCARD, opEvent);
    } else {
      APP_ERR_PRINT("Unexpected LWM2M opev opev_sub_evt :%d.\r\n", event->u.opev_sub_evt);
      free(opEvent);
    }
  } else {
    APP_ERR_PRINT("Unexpected LWM2M event type :%d.\r\n", event->eventType);
  }
}

static int reportMngr_read_report_cb(client_inst_t client, int objectId, int instanceNum,
                                     int resourceNum, lwm2m_uri_and_data_t **uridata,
                                     lwm2m_uridata_free_cb_t *freecb, void *userPriv) {
  APP_UNUSED(userPriv);

  reportManagerReadIndCB_t cbPtr =
      (reportManagerReadIndCB_t)getObjectCbInDb(objectId, OBJ_CBTYPE_READ);
  if (cbPtr == NULL) {
    APP_ERR_PRINT("Read ind Event failed, for obj %d resourceNum %d since no CB was binded\r\n",
                  objectId, resourceNum);
    return -1;
  }

  APP_DBG_PRINT("App read cb ind func called with objId %d ", objectId);
  return cbPtr(objectId, instanceNum, resourceNum, uridata, freecb, userPriv);
}

static int reportMngr_write_report_cb(client_inst_t client, int objectId, int instanceNum,
                                      int resourceNum, LWM2MInstnc_type_e instncType,
                                      int resourceInst, char *resourceVal, uint16_t resourceLen,
                                      void *userPriv) {
  APP_UNUSED(userPriv);
  reportManagerWriteIndCB_t cbPtr;

  cbPtr = (reportManagerWriteIndCB_t)getObjectCbInDb(objectId, OBJ_CBTYPE_WRITE);
  if (cbPtr == NULL) {
    APP_ERR_PRINT("Write ind Event failed, for obj %d resourceNum %d since no CB was binded\r\n",
                  objectId, resourceNum);
    return -1;
  }

  // Call application binded CB function
  APP_DBG_PRINT("App write cb ind func called with objId %d ", objectId);
  return cbPtr(objectId, instanceNum, resourceNum, resourceInst, instncType, resourceVal,
               resourceLen, userPriv);
}

static int reportMngr_exe_report_cb(client_inst_t client, int objectId, int instanceNum,
                                    int resourceNum, int resourceInst, char *param,
                                    void *userPriv) {
  APP_UNUSED(userPriv);
  reportManagerExeIndCB_t cbPtr;

  cbPtr = (reportManagerExeIndCB_t)getObjectCbInDb(objectId, OBJ_CBTYPE_EXE);
  if (cbPtr == NULL) {
    APP_ERR_PRINT("exe ind Event failed, for obj %d resourceNum %d since no CB was binded\r\n",
                  objectId, resourceNum);
    return -1;
  }

  // Call application binded CB function
  APP_DBG_PRINT("App exe cb ind func called with objId %d ", objectId);
  return cbPtr(objectId, instanceNum, resourceNum, resourceInst, param, userPriv);
}

static state_t RPRTMGR_RPRTMGRRegister_Handler(void *pCtx, void *pParam) {
  APP_UNUSED(pCtx);
  lwm2m_event_t *lwm2mEvent = (lwm2m_event_t *)pParam;

  // stop timer for waiting to server ack/nack
  osal_TimerStop(reprtMngrTranscTimer, reportManagerTransTimeout);

  if ((lwm2mEvent->eventType == EVENT_OPEV) &&
      (lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_REGISTER_FINISHED)) {
    APP_DBG_PRINT("Event CB to RPRT mgr APP WITH REGISTER ACK \r\n");

  } else if ((lwm2mEvent->eventType == EVENT_OPEV) &&
             (lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_DEREGISTER_FINISHED)) {
    reportResult_t reportResult;
    APP_DBG_PRINT("Event CB to RPRT mgr APP WITH DE-REGISTER ACK \r\n");
    if (reportManagerNotifyResultCB) {
      reportResult.type = Report_result_type_register_init;
      reportResult.result = REPORT_MNGR_RET_CODE_FAIL;
      reportResult.reasonFail = REGISTER_STATE_MISMATCH;
      reportManagerNotifyResultCB(&reportResult);
    }

    free(pParam);
    observingObjectBitSM = 0;
    return STATE_REPORTMGR_IDLE;
  } else {
    APP_ERR_PRINT("Unepected Event  %d %d in CB to RPRT mgr\r\n", lwm2mEvent->eventType,
                  lwm2mEvent->u.opev_sub_evt);
  }

  APP_TRACE();
  free(pParam);
  return observingObjectBitSM ? STATE_REPORTMGR_READY_FOR_NOTIFY_REQ
                              : STATE_REPORTMGR_WAIT_4_OBSERVE_START;
}

static LWM2MError_e getLocationResrcVal(int32_t resrcNum, repDesc_t *reportToBeSend,
                                        resourceVal_t *rsrc, LWM2MRsrc_type_e *valType) {
  *valType = RSRC_VALUE_TYPE_FLOAT;
  switch (resrcNum) {
    case 0:
      rsrc->value = reportToBeSend->u.lwm2m_location.Latitude;
      break;

    case 1:
      rsrc->value = reportToBeSend->u.lwm2m_location.Longitude;
      break;

    case 2:
      rsrc->value = reportToBeSend->u.lwm2m_location.Altitude;
      break;

    case 3:
      rsrc->value = reportToBeSend->u.lwm2m_location.Radius;
      break;

    case 4:
      *valType = RSRC_VALUE_TYPE_OPAQUE;
      rsrc->opaqueVal = reportToBeSend->u.lwm2m_location.Velocity.rsrc[0].opaque;
      break;

    case 5:
      *valType = RSRC_VALUE_TYPE_TIME;
      rsrc->value = reportToBeSend->u.lwm2m_location.Timestamp;
      break;

    case 6:
      rsrc->value = reportToBeSend->u.lwm2m_location.Speed;
      break;

    default:
      return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}

static LWM2MError_e getCellIdResrcVal(int32_t resrcNum, repDesc_t *reportToBeSend,
                                      resourceVal_t *value, LWM2MRsrc_type_e *valType) {
  *valType = RSRC_VALUE_TYPE_INTEGER;
  switch (resrcNum) {
    case 3:
      value->value = reportToBeSend->u.lwm2m_cell_id.link_quality;
      break;

    case 8:
      value->value = reportToBeSend->u.lwm2m_cell_id.glob_cell_id;
      break;

    case 9:
      value->value = reportToBeSend->u.lwm2m_cell_id.mnc;
      break;

    case 10:
      value->value = reportToBeSend->u.lwm2m_cell_id.mcc;
      break;

    default:
      return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}

static LWM2MError_e getAppDataContainerResrcVal(int32_t instNum, int32_t *instId, int32_t rsrcNum,
                                                int32_t rsrcInstNum, int32_t *rsrcInstId,
                                                repDesc_t *reportToBeSend, resourceVal_t *value,
                                                LWM2MRsrc_type_e *valType) {
  *valType = RSRC_VALUE_TYPE_INTEGER;
  if (reportToBeSend->u.lwm2m_app_data.numInst - 1 >= instNum) {
    *instId = reportToBeSend->u.lwm2m_app_data.obj[instNum].instId;
    switch (rsrcNum) {
      case 0:
        if (reportToBeSend->u.lwm2m_app_data.obj[instNum].data.numInst - 1 >= rsrcInstNum) {
          value->opaqueVal =
              reportToBeSend->u.lwm2m_app_data.obj[instNum].data.rsrc[rsrcInstNum].opaque;
          *rsrcInstId = reportToBeSend->u.lwm2m_app_data.obj[instNum].data.rsrc[rsrcInstNum].instId;
          *valType = RSRC_VALUE_TYPE_OPAQUE;
        } else {
          return LWM2M_FAILURE;
        }

        break;

      case 1:
        value->value = (double)reportToBeSend->u.lwm2m_app_data.obj[instNum].dataPriority;
        *valType = RSRC_VALUE_TYPE_INTEGER;
        break;

      case 2:
        value->value = (double)reportToBeSend->u.lwm2m_app_data.obj[instNum].dataCreateTime;
        *valType = RSRC_VALUE_TYPE_TIME;
        break;

      case 3:
        snprintf(value->strVal, sizeof(value->strVal), "%s",
                 reportToBeSend->u.lwm2m_app_data.obj[instNum].dataDesc);
        *valType = RSRC_VALUE_TYPE_STRING;
        break;

      case 4:
        snprintf(value->strVal, sizeof(value->strVal), "%s",
                 reportToBeSend->u.lwm2m_app_data.obj[instNum].dataFormat);
        *valType = RSRC_VALUE_TYPE_STRING;
        break;

      case 5:
        value->value = (double)reportToBeSend->u.lwm2m_app_data.obj[instNum].appId;
        *valType = RSRC_VALUE_TYPE_INTEGER;
        break;

      default:
        return LWM2M_FAILURE;
    }
  } else {
    return LWM2M_FAILURE;
  }

  return LWM2M_SUCCESS;
}

static state_t RPRTMGR_RPRTMGRRegUpdtReq_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  lwm2m_serverinfo_t srvinfo;
  LWM2MError_e retcode;
  int updated = 0, registered = 0;

  APP_DBG_PRINT("report mgr send REGISTER UPDATE request\r\n");

  retcode = altcom_lwm2mGetServerInfo(lwm2m_client, &srvinfo);
  if (retcode == LWM2M_SUCCESS) {
    int i;

    for (i = 0; i < MAX_LWM2M_SERVER_NUM; i++) {
      if (srvinfo.info[i].valid != 0 && srvinfo.info[i].server_stat == SERVER_STAT_REG_SUCCESS) {
        APP_DBG_PRINT("Registered server(%lu) found!!!\r\n", srvinfo.info[i].server_id);
        if (altcom_lwm2mRegisterUpdt(lwm2m_client, srvinfo.info[i].server_id) == LWM2M_FAILURE) {
          APP_ERR_PRINT("altcom_lwm2mRegisterUpdt  FAILED!!!\r\n");
          reportResult.reasonFail = REGISTER_UPDT_API_FAILED;
          goto fail;
        }

        updated++;
      } else if (srvinfo.info[i].valid != 0 &&
                 srvinfo.info[i].server_stat == SERVER_STAT_NOT_REG_OR_BOOTSTRAP_NOT_STARTED) {
        if (i != 0) {
          APP_DBG_PRINT("Unregistered server(%lu) found!!!\r\n", srvinfo.info[i].server_id);
          if (altcom_lwm2mRegisterReq(lwm2m_client, srvinfo.info[i].server_id) == LWM2M_FAILURE) {
            APP_ERR_PRINT("altcom_lwm2mRegister FAILED!!!\r\n");
            reportResult.reasonFail = REGISTER_API_FAILED;
            goto fail;
          }

          registered++;
        }
      }
    }
  } else {
    goto fail;
  }

  if (!updated && !registered) {
    APP_DBG_PRINT("No server exist to be updated or registered!\r\n");
    goto fail;
  }

  // start timer for waiting to ack/nack register UPDATE request
  osal_TimerStart(reprtMngrTranscTimer, reportManagerTransTimeout);
  return STATE_REPORTMGR_REG_UPDT_WAS_SENT;

fail:
  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_register_update;
    reportResult.result = REPORT_MNGR_RET_CODE_FAIL;
    reportResult.reasonFail = REGISTER_UPDT_API_FAILED;
    reportManagerNotifyResultCB(&reportResult);
  }

  return STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
}

static int regAllAltcomEventsCBsUtil(void) {
  // register CB function that would handle events/urc coming from host
  if (altcom_lwm2mSetEventCallback(EVENT_OPEV, (void *)reportMngr_opev_report_cb,
                                   (void *)0xDEADBEEF) != LWM2M_SUCCESS) {
    return -1;
  }

  if (altcom_lwm2mSetEventCallback(EVENT_OBSERVE_START, (void *)reportMngr_obsrvev_report_cb,
                                   (void *)0xDEADBEEF) != LWM2M_SUCCESS) {
    return -1;
  }

  if (altcom_lwm2mSetEventCallback(EVENT_OBSERVE_STOP, (void *)reportMngr_obsrvev_report_cb,
                                   NULL) != LWM2M_SUCCESS) {
    return -1;
  }

  if (altcom_lwm2mSetEventCallback(EVENT_READ_REQ, (void *)reportMngr_read_report_cb,
                                   (void *)0xDEADBEEF) != LWM2M_SUCCESS) {
    return -1;
  }

  if (altcom_lwm2mSetEventCallback(EVENT_WRITE_REQ, (void *)reportMngr_write_report_cb,
                                   (void *)0xDEADBEEF) != LWM2M_SUCCESS) {
    return -1;
  }

  if (altcom_lwm2mSetEventCallback(EVENT_EXE_REQ, (void *)reportMngr_exe_report_cb,
                                   (void *)0xDEADBEEF) != LWM2M_SUCCESS) {
    return -1;
  }
  return 0;
}

static state_t RPRTMGR_RPRTMGRNotifyReq_Handler(void *pCtx, void *pParam) {
  int32_t numOfResrcIdx;
  unsigned int numOfResrcToRreport = 0;
  unsigned char uriAndDataIdx = 0;
  char *token = NULL;

  lwm2m_uri_and_data_t *uriAndDataAllocPtr = NULL;
  LWM2MRsrc_type_e valType;
  resourceVal_t value;
  stateMachine_t *ctx = (stateMachine_t *)pCtx;
  reportType_t reportType;

  REPORTMGRNotifyReq_t *reqContainer = (REPORTMGRNotifyReq_t *)pParam;
  Report_result result = REPORT_MNGR_RET_CODE_OK;
  Report_fail_reason_t reasonFail = UNKNOWN_REASON;
  reportResult_t reportResult;
  report_type_bitmask_t objObservCheck;

  APP_DBG_PRINT(
      "RPRT mgr APP WITH NOTIFY REQ TO BE SENT TO SERVER ReportType=%d "
      "confReg=%d\r\n",
      reqContainer->reportToBeSend->reportType, reqContainer->reportToBeSend->askForAckFromServer);
  APP_TRACE();

  bool askForAckFromServer = reqContainer->reportToBeSend->askForAckFromServer;

  /*  Start by setting the number of uri rsrc
      This number would be used for allocating uri and data object
      Here we also check the reported object been observed before
  */

  reportType = reqContainer->reportToBeSend->reportType;
  if (reportType == REPORT_TYPE_LOCATION) {
    objObservCheck = REPORT_TYPE_LWM2M_LOCATION;
    numOfResrcToRreport = LWM2M_LOCATION_MAX_RESOURCE_NUM - 1; /* Skip /6/0/4 temporarily */
  } else if (reportType == REPORT_TYPE_CELLID) {
    objObservCheck = REPORT_TYPE_LWM2M_LTE_CELL;
    numOfResrcToRreport = 4;
  } else if (reportType == REPORT_TYPE_APPDATA_CONTAINER) {
    objObservCheck = REPORT_TYPE_LWM2M_BIN_APP_DATA;
    numOfResrcToRreport =
        reqContainer->reportToBeSend->u.lwm2m_app_data.numInst *
        (LWM2M_DATA_CONTAINER_MAX_RESOURCE_NUM - 1); /* Skip /19/X/0 temporarily */

    /* Calculate /19/X/0/Y */
    uint16_t numInst;
    for (numInst = 0; numInst < reqContainer->reportToBeSend->u.lwm2m_app_data.numInst; numInst++) {
      numOfResrcToRreport +=
          (unsigned int)reqContainer->reportToBeSend->u.lwm2m_app_data.obj[numInst].data.numInst;
    }

    APP_ASSERT(0 != numOfResrcToRreport, "numOfResrcToRreport 0 fail\r\n");
  } else {
    APP_ERR_PRINT("Invalid report type %d\r\n", (int)reportType);
    result = REPORT_MNGR_RET_CODE_FAIL;
    reasonFail = UNKNOWN_REASON;
    goto err;
  }

  /* Check if object observed */
  if (!(observingObjectBitSM & objObservCheck)) {
    result = REPORT_MNGR_RET_CODE_FAIL;
    reasonFail = NO_OBSERVE_WAS_RECV;
    goto err;
  }

  /* Preparing the reporting memory */
  if (numOfResrcToRreport) {
    uriAndDataAllocPtr = malloc(numOfResrcToRreport * sizeof(lwm2m_uri_and_data_t));
    APP_ASSERT(NULL != uriAndDataAllocPtr, "malloc fail\r\n");
  }

  /* Copy reporting resources */
  if (reportType == REPORT_TYPE_LOCATION) {
    if (getObserverToken(OBJ_6, &token) || strlen(token) == 0) {
      APP_ERR_PRINT("Invalid token\r\n");
      goto err;
    }

    for (numOfResrcIdx = 0; numOfResrcIdx < LWM2M_LOCATION_MAX_RESOURCE_NUM; numOfResrcIdx++) {
      if (numOfResrcIdx == 4) {
        continue; /* Skip /6/0/4 temporarily */
      }

      uriAndDataAllocPtr[uriAndDataIdx].objectId = LWM2M_LOCATION_OBJECT_ID;
      uriAndDataAllocPtr[uriAndDataIdx].instanceNum = LWM2M_LOCATION_INSTANCE_NUM;
      uriAndDataAllocPtr[uriAndDataIdx].resourceNum = numOfResrcIdx;
      uriAndDataAllocPtr[uriAndDataIdx].resourceInstanceNum = 0;
      uriAndDataAllocPtr[uriAndDataIdx].LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
      if (getLocationResrcVal(numOfResrcIdx, reqContainer->reportToBeSend, &value, &valType) !=
          LWM2M_SUCCESS) {
        APP_ERR_PRINT(
            "Fail to send NOTIFY since failed to generate notify uri data numOfResrcIdx=%u\r\n",
            numOfResrcIdx);
        result = REPORT_MNGR_RET_CODE_FAIL;
        reasonFail = NOTIFY_DISCARD_DUE_2_RSRC_VAL_GET;
        goto err;
      }

      uriAndDataAllocPtr[uriAndDataIdx].resourceVal = value;
      uriAndDataAllocPtr[uriAndDataIdx].valType = valType;
      dump_resource(&uriAndDataAllocPtr[uriAndDataIdx]);
      uriAndDataIdx++;
    }
  } else if (reportType == REPORT_TYPE_CELLID) {
    if (getObserverToken(OBJ_4, &token) || strlen(token) == 0) {
      APP_ERR_PRINT("Invalid token\r\n");
      goto err;
    }

    for (numOfResrcIdx = 3; numOfResrcIdx < 11; numOfResrcIdx++) {
      if (numOfResrcIdx == 3 || (numOfResrcIdx >= 8 && numOfResrcIdx <= 10)) {
        uriAndDataAllocPtr[uriAndDataIdx].objectId = LWM2M_CONNECTIVITY_MONITORING_OBJECT_ID;
        uriAndDataAllocPtr[uriAndDataIdx].instanceNum = LWM2M_CELL_ID_INSTANCE_NUM;
        uriAndDataAllocPtr[uriAndDataIdx].resourceNum = numOfResrcIdx;
        uriAndDataAllocPtr[uriAndDataIdx].resourceInstanceNum = 0;
        uriAndDataAllocPtr[uriAndDataIdx].LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
        if (getCellIdResrcVal(numOfResrcIdx, reqContainer->reportToBeSend, &value, &valType) !=
            LWM2M_SUCCESS) {
          APP_ERR_PRINT(
              "Fail to send NOTIFY since failed to generate notify uri data numOfResrcIdx=%u\r\n",
              numOfResrcIdx);
          result = REPORT_MNGR_RET_CODE_FAIL;
          reasonFail = NOTIFY_DISCARD_DUE_2_RSRC_VAL_GET;
          goto err;
        }

        uriAndDataAllocPtr[uriAndDataIdx].resourceVal = value;
        uriAndDataAllocPtr[uriAndDataIdx].valType = valType;
        dump_resource(&uriAndDataAllocPtr[uriAndDataIdx]);
        uriAndDataIdx++;
      }
    }
  } else if (reportType == REPORT_TYPE_APPDATA_CONTAINER) {
    if (getObserverToken(OBJ_19, &token) || strlen(token) == 0) {
      APP_ERR_PRINT("Invalid token\r\n");
      goto err;
    }

    int32_t numObj, numRsrc;
    int32_t objInstId, rsrcInstId;

    for (numObj = 0; numObj < reqContainer->reportToBeSend->u.lwm2m_app_data.numInst; numObj++) {
      for (numRsrc = 0; numRsrc < LWM2M_DATA_CONTAINER_MAX_RESOURCE_NUM; numRsrc++) {
        if (numRsrc == 0) {
          int32_t numRsrcInst;

          for (numRsrcInst = 0;
               numRsrcInst <
               reqContainer->reportToBeSend->u.lwm2m_app_data.obj[numObj].data.numInst;
               numRsrcInst++) {
            getAppDataContainerResrcVal(numObj, &objInstId, numRsrc, numRsrcInst, &rsrcInstId,
                                        reqContainer->reportToBeSend, &value, &valType);
            uriAndDataAllocPtr[uriAndDataIdx].objectId = LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID;
            uriAndDataAllocPtr[uriAndDataIdx].instanceNum = objInstId;
            uriAndDataAllocPtr[uriAndDataIdx].resourceNum = numRsrc;
            uriAndDataAllocPtr[uriAndDataIdx].resourceInstanceNum = rsrcInstId;
            uriAndDataAllocPtr[uriAndDataIdx].LWM2MInstncType = RSRC_INSTNC_TYPE_MULTIPLE;
            uriAndDataAllocPtr[uriAndDataIdx].resourceVal = value;
            uriAndDataAllocPtr[uriAndDataIdx].valType = valType;
            dump_resource(&uriAndDataAllocPtr[uriAndDataIdx]);
            uriAndDataIdx++;
            continue;
          }
        } else {
          getAppDataContainerResrcVal(numObj, &objInstId, numRsrc, 0, &rsrcInstId,
                                      reqContainer->reportToBeSend, &value, &valType);
          uriAndDataAllocPtr[uriAndDataIdx].objectId = LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID;
          uriAndDataAllocPtr[uriAndDataIdx].instanceNum = objInstId;
          uriAndDataAllocPtr[uriAndDataIdx].resourceNum = numRsrc;
          uriAndDataAllocPtr[uriAndDataIdx].resourceInstanceNum = 0;
          uriAndDataAllocPtr[uriAndDataIdx].LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
          uriAndDataAllocPtr[uriAndDataIdx].resourceVal = value;
          uriAndDataAllocPtr[uriAndDataIdx].valType = valType;
          dump_resource(&uriAndDataAllocPtr[uriAndDataIdx]);
          uriAndDataIdx++;
        }
      }
    }
  } else {
    APP_ERR_PRINT("Invalid report type=%d\r\n", (int)reportType);
    result = REPORT_MNGR_RET_CODE_FAIL;
    reasonFail = UNKNOWN_REASON;
    goto err;
  }
  // this part below is relevant only to all LWM2M reports type (not unique UDP)

  APP_DBG_PRINT("Call to altcom_lwm2mSendNotify with numUriData=%d token=%s\r\n", uriAndDataIdx,
                token);
  if (altcom_lwm2mSendNotify(lwm2m_client, token, askForAckFromServer, uriAndDataIdx,
                             uriAndDataAllocPtr) == LWM2M_FAILURE) {
    APP_ERR_PRINT("altcom_lwm2mSendNotify FAILED!!!!!\r\n");
    result = REPORT_MNGR_RET_CODE_FAIL;
    reasonFail = NOTIFY_DISCARD_DUE_TO_NET_STATE;
  }

err:
  if (((expectAckToreport == false) || (result != REPORT_MNGR_RET_CODE_OK)) &&
      (reportManagerNotifyResultCB)) {
    reportResult.type = Report_result_type_notify_req;
    reportResult.result = result;
    reportResult.reasonFail = reasonFail;
    reportManagerNotifyResultCB(&reportResult);
  }

  free(reqContainer->reportToBeSend);
  free(reqContainer);
  if (uriAndDataAllocPtr != NULL) {
    free(uriAndDataAllocPtr);
  }

  if ((expectAckToreport == false) || (result != REPORT_MNGR_RET_CODE_OK)) {
    ctx->currState = STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
    return ctx->currState;
  }
  // start timer for waiting to server ack/nack
  osal_TimerStart(reprtMngrTranscTimer, reportManagerTransTimeout);

  return STATE_REPORTMGR_NOTIFY_WAS_SENT;
}

static state_t RPRTMGRINIT_RPRTMGROBSERV_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  lwm2m_event_t *lwm2mEvent = (lwm2m_event_t *)pParam;
  stateMachine_t *ctx = (stateMachine_t *)pCtx;

  // APP_UNUSED(pParam);
  APP_DBG_PRINT("Event CB to RPRT mgr APP OBSERVE %s for objId=%hu \r\n",
                lwm2mEvent->eventType == EVENT_OBSERVE_START ? "START" : "STOP",
                (lwm2mEvent->u.observeInfo.objId));

  // Save token recv to db for future use with notify request
  if (((observObjectBitSM & REPORT_TYPE_LWM2M_LOCATION) != 0) &&
      (lwm2mEvent->u.observeInfo.objId == LWM2M_LOCATION_OBJECT_ID)) {
    if (lwm2mEvent->eventType == EVENT_OBSERVE_START) {
      if (setObserverToken(OBJ_6, lwm2mEvent->u.observeInfo.token)) {
        APP_ERR_PRINT("Fail to save token given by observe event :%s\r\n",
                      lwm2mEvent->u.observeInfo.token);
      }

      observingObjectBitSM |= REPORT_TYPE_LWM2M_LOCATION;
    } else {
      observingObjectBitSM &= ~REPORT_TYPE_LWM2M_LOCATION;
    }

  } else if (((observObjectBitSM & REPORT_TYPE_LWM2M_LTE_CELL) != 0) &&
             (lwm2mEvent->u.observeInfo.objId == LWM2M_CONNECTIVITY_MONITORING_OBJECT_ID)) {
    if (lwm2mEvent->eventType == EVENT_OBSERVE_START) {
      if (setObserverToken(OBJ_4, lwm2mEvent->u.observeInfo.token)) {
        APP_ERR_PRINT("Fail to save token given by observe event :%s\r\n",
                      lwm2mEvent->u.observeInfo.token);
      }

      observingObjectBitSM |= REPORT_TYPE_LWM2M_LTE_CELL;
    } else {
      observingObjectBitSM &= ~REPORT_TYPE_LWM2M_LTE_CELL;
    }
  } else if (((observObjectBitSM & REPORT_TYPE_LWM2M_BIN_APP_DATA) != 0) &&
             (lwm2mEvent->u.observeInfo.objId == LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID)) {
    if (lwm2mEvent->eventType == EVENT_OBSERVE_START) {
      if (setObserverToken(OBJ_19, lwm2mEvent->u.observeInfo.token)) {
        APP_ERR_PRINT("Fail to save token given by observe event :%s\r\n",
                      lwm2mEvent->u.observeInfo.token);
      }

      observingObjectBitSM |= REPORT_TYPE_LWM2M_BIN_APP_DATA;
    } else {
      observingObjectBitSM &= ~REPORT_TYPE_LWM2M_BIN_APP_DATA;
    }
  } else {
    APP_ERR_PRINT("Event CB to RPRT mgr GOT OBSERVE START for unexpected objectId=%d \r\n",
                  (lwm2mEvent->u.observeInfo.objId));
    if (reportManagerNotifyResultCB) {
      reportResult.type = Report_result_type_observe_req;
      reportResult.result = REPORT_MNGR_RET_CODE_FAIL;
      reportResult.reasonFail = UNSUPPORTED_OBSERVE_OBJ;
      reportManagerNotifyResultCB(&reportResult);
    }
  }

  free(pParam);
  APP_TRACE();

  if (observingObjectBitSM) {
    if (ctx->currState == STATE_REPORTMGR_WAIT_4_OBSERVE_START) {
      return STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
    }
  } else {
    if (ctx->currState == STATE_REPORTMGR_READY_FOR_NOTIFY_REQ) {
      return STATE_REPORTMGR_WAIT_4_OBSERVE_START;
    }
  }

  return ctx->currState;
}
static state_t RPRTMGRINIT_RPRTMGRNotifyResp_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  lwm2m_event_t *lwm2mEvent = (lwm2m_event_t *)pParam;
  APP_UNUSED(pCtx);

  APP_TRACE();

  if ((lwm2mEvent->eventType == EVENT_OPEV) &&
      ((lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_RSP) ||
       (lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_CONFIRM_NOTIFY_FAIL) ||
       (lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_DISCARD))) {
    // Stop timer for waiting to server ack/nack
    osal_TimerStop(reprtMngrTranscTimer, reportManagerTransTimeout);
    if ((lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_RSP)) {
      APP_DBG_PRINT("Event CB to RPRT mgr APP, Notify req resp success\r\n");
      reportResult.result = REPORT_MNGR_RET_CODE_OK;
    } else if ((lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_CONFIRM_NOTIFY_FAIL)) {
      APP_ERR_PRINT("Event CB to RPRT mgr APP, Notify req resp failed\r\n");
      reportResult.result = REPORT_MNGR_RET_CODE_FAIL;
      reportResult.reasonFail = NOTIFY_CONFRIM_FAILED_BY_SERVER;
    } else if ((lwm2mEvent->u.opev_sub_evt == LWM2MOPEV_EVENT_NOTIFY_DISCARD)) {
      APP_ERR_PRINT("Event CB to RPRT mgr APP, Notify req was discarded\r\n");
      reportResult.result = REPORT_MNGR_RET_CODE_FAIL;
      reportResult.reasonFail = NOTIFY_DISCARD_DUE_TO_NET_STATE;
    }
    if (reportManagerNotifyResultCB) {
      reportResult.type = Report_result_type_notify_req;
      reportManagerNotifyResultCB(&reportResult);
    }
  }
  APP_TRACE();
  free(pParam);
  return STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
}

static state_t RPRTMGRINIT_RPRTMGRRegUpdtTimeout_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  APP_UNUSED(pCtx);
  APP_DBG_PRINT("Event CB to RPRT mgr APP, REGISTER UPDTE req resp Timeout\r\n");
  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_register_update;
    reportResult.result = REPORT_MNGR_RET_TIMEOUT;
    reportManagerNotifyResultCB(&reportResult);
  }

  APP_TRACE();
  return STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
}

static state_t RPRTMGRINIT_RPRTMGRRegUpdtResp_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  APP_UNUSED(pCtx);
  APP_DBG_PRINT("Event CB to RPRT mgr APP, REGISTER UPDTE req resp OK\r\n");

  // stop timer for waiting to server ack/nack
  osal_TimerStop(reprtMngrTranscTimer, reportManagerTransTimeout);

  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_register_update;
    reportResult.result = REPORT_MNGR_RET_CODE_OK;
    reportManagerNotifyResultCB(&reportResult);
  }

  APP_TRACE();
  free(pParam);  // free object allocated size lwm2m_event_t
  return observingObjectBitSM ? STATE_REPORTMGR_READY_FOR_NOTIFY_REQ
                              : STATE_REPORTMGR_WAIT_4_OBSERVE_START;
}
// Usually timeout on regtister would occur if device is not defined/configured in server
static state_t RPRTMGRINIT_RPRTMGRRegisterRespTimeout_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  APP_UNUSED(pCtx);

  APP_DBG_PRINT("Event CB to RPRT mgr APP, Register req resp Timeout\r\n");

  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_register_init;
    reportResult.result = REPORT_MNGR_RET_TIMEOUT;
    reportManagerNotifyResultCB(&reportResult);
  }
  APP_TRACE();
  return STATE_REPORTMGR_IDLE;
}

static state_t RPRTMGRINIT_RPRTMGRNotifyRespTimeout_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  APP_UNUSED(pCtx);

  APP_DBG_PRINT("Event CB to RPRT mgr APP, Notify req resp Timeout\r\n");
  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_notify_req;
    reportResult.result = REPORT_MNGR_RET_TIMEOUT;
    reportManagerNotifyResultCB(&reportResult);
  }

  APP_TRACE();
  return STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
}

static state_t RPRTMGRINIT_RPRTMGRSTART_Handler(void *pCtx, void *pParam) {
  reportResult_t reportResult;
  REPORTMGRState_e nextState = STATE_REPORTMGR_IDLE;  // In case of fail
  Report_result result = REPORT_MNGR_RET_CODE_FAIL;

  APP_UNUSED(pCtx);
  // APP_UNUSED(pParam);
  report_type_bitmask_t *observObjectBitPtr = (report_type_bitmask_t *)pParam;
  APP_TRACE();

  // Start registration flow */
  APP_DBG_PRINT("report mgr start was recv send HOST enable\n\r");
  APP_DBG_PRINT("report mgr send HOST enable\n\r");
  if (altcom_lwm2mEnableHost(lwm2m_client) == LWM2M_FAILURE) {
    APP_ERR_PRINT("altcom_lwm2mEnableHost  FAILED!!!\r\n");
    reportResult.reasonFail = ENABLE_HOST_FAILED;
    goto end;
  }
  APP_DBG_PRINT("send OPEV enable\n\r");
  if (altcom_lwm2mEnableOpev(
          lwm2m_client, (APICMDID_ENABLE_REGISTER_OPEV | APICMDID_ENABLE_REGISTERUPDT_OPEV |
                         APICMDID_ENABLE_NOTIFY_ACK_OPEV | APICMDID_ENABLE_NOTIFY_FAIL_OPEV |
                         APICMDID_ENABLE_NOTIFY_DISCARD_OPEV | APICMDID_ENABLE_DEREGISTER_OPEV)) ==
      LWM2M_FAILURE) {
    APP_ERR_PRINT("altcom_lwm2mEnableOpev  FAILED!!!\r\n");
    reportResult.reasonFail = ENABLE_OPEV_FAILED;
    goto end;
  } else {
    // register CB function that would handle events/urc coming from host
    reportResult.reasonFail = EVENT_CALLBACK_HOOK_FAIL;
    if (regAllAltcomEventsCBsUtil() != 0) {
      goto end;
    }
  }
  APP_DBG_PRINT("report mgr send REGISTER request\n\r");
  if (altcom_lwm2mRegisterReq(lwm2m_client, THE_EXISTING_SERVER_ID /* server_id*/) ==
      LWM2M_FAILURE) {
    lwm2m_serverinfo_t serverinfo;
    unsigned char numOfServer;

    APP_ERR_PRINT("altcom_lwm2mRegisterReq  FAILED try to check reason !!!\r\n");
    if (altcom_lwm2mGetServerInfo(lwm2m_client, &serverinfo) == LWM2M_FAILURE) {
      APP_ERR_PRINT("altcom_lwm2mRegisterReq  FAILED to get server state !!!\r\n");
      reportResult.reasonFail = SERVER_STATE_API_FAILED;
      goto end;
    }
    // check the server state we assume ONLY one is relevant
    bool found = false;

    for (numOfServer = 0; numOfServer < MAX_LWM2M_SERVER_NUM; numOfServer++) {
      if ((serverinfo.info[numOfServer].valid) &&
          (serverinfo.info[numOfServer].server_stat == SERVER_STAT_REG_SUCCESS)) {
        found = true;
        APP_DBG_PRINT(
            "altcom_lwm2mRegisterReq FAILED server is already register sending de-register!!!\r\n");
        if (altcom_lwm2mDeregisterReq(lwm2m_client,
                                      serverinfo.info[numOfServer].server_id /* server_id*/) !=
            LWM2M_FAILURE) {
          nextState = STATE_REPORTMGR_WAIT_4_REGISTER_ACK;
        } else {
          APP_ERR_PRINT("altcom_lwm2mDeregisterReq FAILED !!!\r\n");
          reportResult.reasonFail = REGISTER_STATE_MISMATCH;
          goto end;
        }
      }
    }
    if (found == false) {
      reportResult.reasonFail = REGISTER_API_FAILED;
      goto end;
    }
  } else {
    nextState = STATE_REPORTMGR_WAIT_4_REGISTER_ACK;
  }
  // start timer for waiting to ack/nack register request
  osal_TimerStart(reprtMngrTranscTimer, reportManagerTransTimeout);
  free(observObjectBitPtr);
  return nextState;

end:
  free(observObjectBitPtr);
  if (reportManagerNotifyResultCB) {
    reportResult.type = Report_result_type_register_init;
    reportResult.result = result;
    reportManagerNotifyResultCB(&reportResult);
  }

  return nextState;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void reportManagerRegister(void) {
  APP_DBG_PRINT("report mgr send REGISTER request\n\r");
  if (altcom_lwm2mRegisterReq(lwm2m_client, 0 /* server_id*/) == LWM2M_FAILURE) {
    APP_DBG_PRINT("altcom_lwm2mRegisterReq  FAILED!!!\r\n");
  }
  APP_DBG_PRINT("altcom_lwm2mRegisterReq  was sent OK!!!\r\n");
}

// For the time being this function ONLY replace the LWM2M script the was executed
// It actually setup and create the LWM2M object 0 LWM2M Security & 1 LwM2M Server format files
ReportMngrConfig_result reportManagerConfig(report_type_bitmask_t retportTypeBitMask, int force) {
#define NUM_OF_RSRC_AND_DATA_ELEM_OF_SECURITY_OBJECT_ID 4
#define NUM_OF_RSRC_AND_DATA_ELEM_OF_SERVER_OBJECT_ID 2
#define LWM2M_CLIENT (lwm2m_client == CLIENT_INST_OPERATOR ? "" : "2")

  lwm2m_resource_and_data_t *resrcAndDataPtr = NULL;
  char cfgNameStr[MISC_MAX_CFGNAME_LEN], cfgValStr[MISC_MAX_CFGVALUE_LEN];
  Misc_Error_e miscRet;

  // check if MAP was set already
#define CFG_FACTORY_BS_NAME_FMT "LWM2M%s.Config.FactoryBsName"
  if (force) {
    APP_DBG_PRINT("Force bootstrap!\r\n");
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_FACTORY_BS_NAME_FMT, LWM2M_CLIENT);
    miscRet = altcom_SetACfg(cfgNameStr, "N");
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, "N",
                  miscRet != MISC_SUCCESS ? " failed" : "");
    if (miscRet != MISC_SUCCESS) {
      goto fail;
    }
  } else {
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_FACTORY_BS_NAME_FMT, LWM2M_CLIENT);
    miscRet = altcom_GetACfg(cfgNameStr, cfgValStr);
    if (miscRet == MISC_SUCCESS) {
      APP_DBG_PRINT("\"%s\" = \"%s\"\r\n", cfgNameStr, cfgValStr);
    } else {
      APP_DBG_PRINT("Get \"%s\" failed\r\n", cfgNameStr);
      goto fail;
    }

    if (strncmp(FACTORY_BS_NAME, cfgValStr, strlen(FACTORY_BS_NAME)) == 0) {
      APP_DBG_PRINT("\r\nMAP LWM2M configuration was done, no need to config again\r\n");
      return REPORT_MNGR_CONFIG_OK;
    }
  }

  // Set default PDN to connect to lwm2m server
  // config -s LWM2M.Config.DefaultPdnServer 0";
#define CFG_DEFAULT_PDN_SERVER_FMT "LWM2M%s.Config.DefaultPdnServer"
#define CFG_DEFAULT_PDN_SERVER_VAL "0"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_DEFAULT_PDN_SERVER_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_DEFAULT_PDN_SERVER_VAL);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");
  if (miscRet != MISC_SUCCESS) {
    goto fail;
  }

#define CFG_HOSTOBJ_OBSRV_WITHOUT_READ_FMT "LWM2M%s.HostObjects.HostObserveWithoutRead"
#define CFG_HOSTOBJ_OBSRV_WITHOUT_READ_VAL "true"
  if (observeWithoutRead == true) {
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_HOSTOBJ_OBSRV_WITHOUT_READ_FMT, LWM2M_CLIENT);
    snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_HOSTOBJ_OBSRV_WITHOUT_READ_VAL);
    miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                  miscRet != MISC_SUCCESS ? " failed" : "");
    if (miscRet != MISC_SUCCESS) {
      goto fail;
    }
  }

#define CFG_NOTIFY_WITHOUT_REG_UPD_FMT "LWM2M%s.HostObjects.ToOnlineByNotifyWithoutRegUpd"
#define CFG_NOTIFY_WITHOUT_REG_UPD_VAL "true"
  if (notifyWithoutRegUpdate == true) {
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_NOTIFY_WITHOUT_REG_UPD_FMT, LWM2M_CLIENT);
    snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_NOTIFY_WITHOUT_REG_UPD_VAL);
    miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                  miscRet != MISC_SUCCESS ? " failed" : "");
    if (miscRet != MISC_SUCCESS) {
      goto fail;
    }
  }

  if (altcom_lwm2mSendBootstrapCmdStart(lwm2m_client, FACTORY_BS_NAME) == LWM2M_FAILURE) {
    APP_DBG_PRINT("\r\naltcom_lwm2mSendBootstrapCmdStart failed\r\n");
    goto fail;
  }

  if (altcom_lwm2mSendBootstrapCmdDelete(lwm2m_client) == LWM2M_FAILURE) {
    APP_DBG_PRINT("\r\altcom_lwm2mSendBootstrapCmdDelete failed\r\n");
    goto fail;
  }

  resrcAndDataPtr =
      malloc(NUM_OF_RSRC_AND_DATA_ELEM_OF_SECURITY_OBJECT_ID * sizeof(lwm2m_resource_and_data_t));
  APP_ASSERT(NULL != resrcAndDataPtr, "malloc fail\r\n");

  //	AT%LWM2MBSCMD="CREATE",0,0,0,"coap://18.207.240.143:5683",1,"false",2,3,10,0

  // Fill the resource and data members
  resrcAndDataPtr[0].resourceNum = LWM2M_SECURITY_SERVER_URI_RSRC_NUM;
  resrcAndDataPtr[0].valType = RSRC_VALUE_TYPE_STRING;
  strncpy(&resrcAndDataPtr[0].strVal[0], altairCsServerUriPtr, MAX_LWM2M_STRING_TYPE_LEN - 1);
  resrcAndDataPtr[0].strVal[MAX_LWM2M_STRING_TYPE_LEN - 1] = 0;  // null term
  if (useBootStrapServer == false) {
    resrcAndDataPtr[1].resourceNum = LWM2M_SECURITY_BOOTSTRAP_SERVER_RSRC_NUM;
    resrcAndDataPtr[1].valType = RSRC_VALUE_TYPE_STRING;
    /* A standard LwM2M Server (false) or a LwM2M Bootstrap-Server (true) */
    strncpy(&resrcAndDataPtr[1].strVal[0], "false", MAX_LWM2M_STRING_TYPE_LEN - 1);
    resrcAndDataPtr[1].strVal[MAX_LWM2M_STRING_TYPE_LEN - 1] = 0;  // null term
  } else {
    resrcAndDataPtr[1].resourceNum = LWM2M_SECURITY_BOOTSTRAP_SERVER_RSRC_NUM;
    resrcAndDataPtr[1].valType = RSRC_VALUE_TYPE_STRING;
    /* A standard LwM2M Server (false) or a LwM2M Bootstrap-Server (true) */
    strncpy(&resrcAndDataPtr[1].strVal[0], "true", MAX_LWM2M_STRING_TYPE_LEN - 1);
    resrcAndDataPtr[1].strVal[MAX_LWM2M_STRING_TYPE_LEN - 1] = 0;  // null term
  }
  resrcAndDataPtr[2].resourceNum = LWM2M_SECURITY_SECURITY_MODE_RSRC_NUM;
  resrcAndDataPtr[2].valType = RSRC_VALUE_TYPE_INTEGER;
  resrcAndDataPtr[2].value = LWM2M_SECURITY_MODE_NO_SEC;

  resrcAndDataPtr[3].resourceNum = LWM2M_SECURITY_SHORT_SERVER_ID_RSRC_NUM;
  resrcAndDataPtr[3].valType = RSRC_VALUE_TYPE_INTEGER;
  resrcAndDataPtr[3].value = LWM2M_SHORT_SERVER_ID_NUM;

  if (altcom_lwm2mSendBootstrapCmdCreate(
          lwm2m_client, LWM2M_SECURITY_OBJECT_ID, LWM2M_SECURITY_INSTANCE_NUM,
          NUM_OF_RSRC_AND_DATA_ELEM_OF_SECURITY_OBJECT_ID, resrcAndDataPtr) == LWM2M_FAILURE) {
    APP_DBG_PRINT("\r\naltcom_lwm2mSendBootstrapCmdCreate failed\r\n");
    goto fail;
  }

  // In secured it work against bootstrap and not DM server.
  // So bootstrap server should provide/set object 1
  //  and object 0 as well
  if (useBootStrapServer == false) {
    //	AT%LWM2MBSCMD="CREATE",1,0,0,0,1,240
    resrcAndDataPtr[0].resourceNum = LWM2M_SERVER_SHORT_SERVER_ID_RSRC_NUM;
    resrcAndDataPtr[0].valType = RSRC_VALUE_TYPE_INTEGER;
    resrcAndDataPtr[0].value = LWM2M_SHORT_SERVER_ID_NUM;

    resrcAndDataPtr[1].resourceNum = LWM2M_SERVER_LIFTIME_RSRC_NUM;
    resrcAndDataPtr[1].valType = RSRC_VALUE_TYPE_INTEGER;
    resrcAndDataPtr[1].value = liftimeValue;
    // resource #6 storing mode default is true so I don't set it

    if (altcom_lwm2mSendBootstrapCmdCreate(
            lwm2m_client, LWM2M_SERVER_OBJECT_ID, LWM2M_SERVER_INSTANCE_NUM,
            NUM_OF_RSRC_AND_DATA_ELEM_OF_SERVER_OBJECT_ID, resrcAndDataPtr) == LWM2M_FAILURE) {
      APP_DBG_PRINT("\r\naltcom_lwm2mSendBootstrapCmdCreate failed\r\n");
      goto fail;
    }
  }

  free(resrcAndDataPtr);
  resrcAndDataPtr = NULL;
  if (altcom_lwm2mSendBootstrapCmdDone(lwm2m_client) == LWM2M_FAILURE) {
    goto fail;
  }

  // Use IMEI as endpoint name  //def value just for sure
  // config -s LWM2M.Config.NameMode 1
#define CFG_NAME_MODE_FMT "LWM2M%s.Config.NameMode"
#define CFG_NAME_MODE_VAL "1"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_NAME_MODE_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_NAME_MODE_VAL);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");

  //#Disable sending of REGISTER to server on each
  // config -s LWM2M.Config.AutoConnect false
#define CFG_AUTO_CONNECT_FMT "LWM2M%s.Config.AutoConnect"
#define CFG_AUTO_CONNECT_VAL "false"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_AUTO_CONNECT_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_AUTO_CONNECT_VAL);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");

  //#Ask for confirmation for each notification
  // config -s LWM2M.Config.ConfirmNotify 1

#define CFG_CONFIRM_NOTIFY_FMT "LWM2M%s.Config.ConfirmNotify"
#define CFG_CONFIRM_NOTIFY_VAL expectAckToreport == true ? "1" : "0"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_CONFIRM_NOTIFY_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_CONFIRM_NOTIFY_VAL);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");

#define CFG_SUPPORTED_OBJ_FMT "LWM2M%s.Config.SupportedObjects"
#define CFG_SUPPORTED_OBJ_VAL "\"0;1;3;5"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_SUPPORTED_OBJ_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_SUPPORTED_OBJ_VAL);
  if (retportTypeBitMask & REPORT_TYPE_LWM2M_LOCATION) {
    char *objStr = ";6";
    strncat(cfgValStr, objStr, strlen(objStr));
  }

  if (retportTypeBitMask & REPORT_TYPE_LWM2M_LTE_CELL) {
    char *objStr = ";4";
    strncat(cfgValStr, objStr, strlen(objStr));
  }

  if (retportTypeBitMask & REPORT_TYPE_LWM2M_BIN_APP_DATA) {
    char *objStr = ";19";
    strncat(cfgValStr, objStr, strlen(objStr));
  }

  strncat(cfgValStr, "\"", 1);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");

  int index;

/* Configure /3/0/4, /3/0/5 executable resource to host */
// config -s lwm2m_resources_info/3_fmt.%d.source host
#define CFG_RSRC_INFO_3_SRC_FMT "lwm2m_resources_info%s/3_fmt.%d.source"
#define CFG_RSRC_INFO_3_SRC_VAL "host"
  for (index = 4; index < 6; index++) {
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_RSRC_INFO_3_SRC_FMT, LWM2M_CLIENT, index);
    snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_RSRC_INFO_3_SRC_VAL);
    miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                  miscRet != MISC_SUCCESS ? " failed" : "");
  }

//#Configure location object id (#6) resources to be host
// config -s lwm2m_resources_info/6_fmt.0.source host
#define CFG_RSRC_INFO_6_SRC_FMT "lwm2m_resources_info%s/6_fmt.%d.source"
#define CFG_RSRC_INFO_6_SRC_VAL "host"
  if (retportTypeBitMask & REPORT_TYPE_LWM2M_LOCATION) {
    for (index = 0; index < 7; index++) {
      snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_RSRC_INFO_6_SRC_FMT, LWM2M_CLIENT, index);
      snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_RSRC_INFO_6_SRC_VAL);
      miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
      APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                    miscRet != MISC_SUCCESS ? " failed" : "");
    }
  }

//#Change connectivity monitor object id (#4) resources to be host
// config -s lwm2m_resources_info/4_fmt.0.source host
#define CFG_RSRC_INFO_4_SRC_FMT "lwm2m_resources_info%s/4_fmt.%d.source"
#define CFG_RSRC_INFO_4_SRC_VAL "host"
  if (retportTypeBitMask & REPORT_TYPE_LWM2M_LTE_CELL) {
    for (index = 8; index < 11; index++) {
      snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_RSRC_INFO_4_SRC_FMT, LWM2M_CLIENT, index);
      snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_RSRC_INFO_4_SRC_VAL);
      miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
      APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                    miscRet != MISC_SUCCESS ? " failed" : "");
    }

    // config -s lwm2m_resources_info/4_fmt.3.source host
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_RSRC_INFO_4_SRC_FMT, LWM2M_CLIENT, 3);
    snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_RSRC_INFO_4_SRC_VAL);
    miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                  miscRet != MISC_SUCCESS ? " failed" : "");

    // config -s lwm2m_resources_info/4_fmt.supported_resources.resources "8;9;10;3"
#define CFG_RSRC_INFO_4_SUPPORT_RSRC_FMT \
  "lwm2m_resources_info%s/4_fmt.supported_resources.resources"
#define CFG_RSRC_INFO_4_SUPPORT_RSRC_VAL "\"8;9;10;3\""
    snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_RSRC_INFO_4_SUPPORT_RSRC_FMT, LWM2M_CLIENT);
    snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_RSRC_INFO_4_SUPPORT_RSRC_VAL);
    miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
    APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                  miscRet != MISC_SUCCESS ? " failed" : "");
  }

  //#Allow host to work slowly by increasing timer
  // config -s LWM2M.HostObjects.HostResponseTimeout 15000
#define CFG_HOSTOBJ_RESP_TIMEOUT_FMT "LWM2M%s.HostObjects.HostResponseTimeout"
#define CFG_HOSTOBJ_RESP_TIMEOUT_VAL "15000"
  snprintf(cfgNameStr, MISC_MAX_CFGNAME_LEN, CFG_HOSTOBJ_RESP_TIMEOUT_FMT, LWM2M_CLIENT);
  snprintf(cfgValStr, MISC_MAX_CFGVALUE_LEN, "%s", CFG_HOSTOBJ_RESP_TIMEOUT_VAL);
  miscRet = altcom_SetACfg(cfgNameStr, cfgValStr);
  APP_DBG_PRINT("Set \"%s\" = \"%s\"%s\r\n", cfgNameStr, cfgValStr,
                miscRet != MISC_SUCCESS ? " failed" : "");

  return REPORT_MNGR_CONFIG_UPDATED;

fail:
  if (resrcAndDataPtr) {
    free(resrcAndDataPtr);
  }

  return REPORT_MNGR_CONFIG_FAIL;
}

int reportMngrBindReadInd(uint32_t objectId, reportManagerReadIndCB_t reportManagerReadCB) {
  if (updateObjectCbInDb(objectId, OBJ_CBTYPE_READ, (void *)reportManagerReadCB) == 0) {
    return 0;
  } else {
    APP_DBG_PRINT("updateObjectCbInDb failed ! \r\n");
    return -1;
  }
}

int reportMngrBindWriteInd(uint32_t objectId, reportManagerWriteIndCB_t reportManagerWriteCB) {
  if (updateObjectCbInDb(objectId, OBJ_CBTYPE_WRITE, (void *)reportManagerWriteCB) == 0) {
    return 0;
  } else {
    APP_DBG_PRINT("updateObjectCbInDb failed ! \r\n");
    return -1;
  }
}

int reportMngrBindExeInd(uint32_t objectId, reportManagerExeIndCB_t reportManagerExeCB) {
  if (updateObjectCbInDb(objectId, OBJ_CBTYPE_EXE, (void *)reportManagerExeCB) == 0) {
    return 0;
  } else {
    APP_DBG_PRINT("updateObjectCbInDb failed ! \r\n");
    return -1;
  }
}

int reportManagerInit(reportManagerStartCB_t reportManagerResultCB, int32_t timeout,
                      report_type_bitmask_t observObjectBit, bool isRegisterRequired) {
  // memset(&locationDB, 0, sizeof(locationDB));
  int ret;

  reportManagerNotifyResultCB = reportManagerResultCB;
  reportManagerTransTimeout = timeout;
  observObjectBitSM =
      (report_type_bitmask_t)((uint32_t)observObjectBit & 0xff);  // only 1st 8 bits are LWM2M

  if (gREPORTMGRCtx.taskRunning == true) {
    goto sendStartToReadySM;
  }
  ret = SM_StateMachineInit(&gREPORTMGRCtx, "ReportMngr", STATE_REPORTMGR_MAX, EVT_REPORTMGR_MAX,
                            gREPORTMGRStateNames, gREPORTMGREventNames);
  if (ret != 0) {
    APP_ERR_PRINT("reportManagerInit - SM_StateMachineInit fail:%d.\r\n", ret);
    return ret;
  }
  // Create transaction timer
  if (reprtMngrTranscTimer == NULL) {
    stateMachineEvt_t event;
    event.eventId = EVT_REPORTMGR_TRANSC_TIMEOUT;
    event.param = (void *)&gREPORTMGRCtx;
    reprtMngrTranscTimer =
        osal_eventTimerCreate(&gREPORTMGRCtx, "reprtMngrTranscTimer", timeout, &event);
    if (!reprtMngrTranscTimer) {
      APP_DBG_PRINT("reprtMngrTranscTimer timer create failed\r\n");
      return -3;
    } else {
      APP_DBG_PRINT("reprtMngrTranscTimer timer successfully created %u msec\r\n", timeout);
    }
  }
  /* Construct state transition table */
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_IDLE, EVT_REPORTMGR_START,
                                   RPRTMGRINIT_RPRTMGRSTART_Handler);

  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_WAIT_4_REGISTER_ACK,
                                   EVT_REPORTMGR_REGISTER_ACK, RPRTMGR_RPRTMGRRegister_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_WAIT_4_REGISTER_ACK,
                                   EVT_REPORTMGR_DEREGISTER_ACK, RPRTMGR_RPRTMGRRegister_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_WAIT_4_REGISTER_ACK,
                                   EVT_REPORTMGR_TRANSC_TIMEOUT,
                                   RPRTMGRINIT_RPRTMGRRegisterRespTimeout_Handler);

  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_WAIT_4_OBSERVE_START,
                                   EVT_REPORTMGR_OBSERVE_START, RPRTMGRINIT_RPRTMGROBSERV_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_READY_FOR_NOTIFY_REQ,
                                   EVT_REPORTMGR_REQ_TO_SEND_RPRT,
                                   RPRTMGR_RPRTMGRNotifyReq_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_READY_FOR_NOTIFY_REQ,
                                   EVT_REPORTMGR_OBSERVE_STOP, RPRTMGRINIT_RPRTMGROBSERV_Handler);

  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_READY_FOR_NOTIFY_REQ,
                                   EVT_REPORTMGR_OBSERVE_START, RPRTMGRINIT_RPRTMGROBSERV_Handler);

  /* Notify was sent wait for the typical resp ack , nack OR discard */
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_NOTIFY_WAS_SENT,
                                   EVT_REPORTMGR_CONFIRM_NOTIFY_ACK,
                                   RPRTMGRINIT_RPRTMGRNotifyResp_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_NOTIFY_WAS_SENT,
                                   EVT_REPORTMGR_CONFIRM_NOTIFY_FAIL,
                                   RPRTMGRINIT_RPRTMGRNotifyResp_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_NOTIFY_WAS_SENT,
                                   EVT_REPORTMGR_NOTIFY_DISCARD,
                                   RPRTMGRINIT_RPRTMGRNotifyResp_Handler);
  /* Take care of case where no resp is recv and timeout passed */
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_NOTIFY_WAS_SENT,
                                   EVT_REPORTMGR_TRANSC_TIMEOUT,
                                   RPRTMGRINIT_RPRTMGRNotifyRespTimeout_Handler);
  /* Support of unexpected observe sue to register request initiated by MAP client */
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_NOTIFY_WAS_SENT,
                                   EVT_REPORTMGR_OBSERVE_START, RPRTMGRINIT_RPRTMGROBSERV_Handler);

  /* Register update was sent wait for the resp*/
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_IDLE,
                                   EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE,
                                   RPRTMGR_RPRTMGRRegUpdtReq_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_READY_FOR_NOTIFY_REQ,
                                   EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE,
                                   RPRTMGR_RPRTMGRRegUpdtReq_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_WAIT_4_OBSERVE_START,
                                   EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE,
                                   RPRTMGR_RPRTMGRRegUpdtReq_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_REG_UPDT_WAS_SENT,
                                   EVT_REPORTMGR_REGISTER_UPDT_ACK,
                                   RPRTMGRINIT_RPRTMGRRegUpdtResp_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_REG_UPDT_WAS_SENT,
                                   EVT_REPORTMGR_REGISTER_ACK,
                                   RPRTMGRINIT_RPRTMGRRegUpdtResp_Handler);
  SM_StateMachineRegisterEvHandler(&gREPORTMGRCtx, STATE_REPORTMGR_REG_UPDT_WAS_SENT,
                                   EVT_REPORTMGR_TRANSC_TIMEOUT,
                                   RPRTMGRINIT_RPRTMGRRegUpdtTimeout_Handler);

sendStartToReadySM:

  if ((isRegisterRequired == true)) {
    // In case of modem RESET or BOOTUP saved token are irrelevant
    // Or if No old cntxt found meaning it is the 1st interaction with server/MAP
    report_type_bitmask_t *observObjectBitPtr = malloc(sizeof(report_type_bitmask_t));
    APP_ASSERT(NULL != observObjectBitPtr, "malloc fail\r\n");

    *observObjectBitPtr = (report_type_bitmask_t)0;
    EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_START, observObjectBitPtr);
  } else {
    if (regAllAltcomEventsCBsUtil() != 0) {
      return -4;
    }

    // case init after wake and registration occured already
    gREPORTMGRCtx.currState = STATE_REPORTMGR_READY_FOR_NOTIFY_REQ;
    if (reportManagerNotifyResultCB) {
      reportResult_t reportResult;
      reportResult.type = Report_result_type_register_init;
      reportResult.result = REPORT_MNGR_RET_CODE_OK;
      reportManagerNotifyResultCB(&reportResult);
    }
  }

  return 0;
}

int reportManagerInitRefresh(void) {
  APP_DBG_PRINT("reportManagerInitRefreshApi was called !!\n");

  if (gREPORTMGRCtx.currState != STATE_REPORTMGR_READY_FOR_NOTIFY_REQ &&
      gREPORTMGRCtx.currState != STATE_REPORTMGR_WAIT_4_OBSERVE_START &&
      gREPORTMGRCtx.currState != STATE_REPORTMGR_IDLE) {
    APP_DBG_PRINT("Fail to send Register update at state %d\r\n", gREPORTMGRCtx.currState);
    return 1;
  }
  EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE, NULL);
  return 0;
}

static int checkReportType(repDesc_t *reportToBeSend) {
  if ((uint32_t)reportToBeSend->reportType >= REPORT_TYPE_MAX) {
    APP_DBG_PRINT("Invalid report type %d !!\r\n", (int)reportToBeSend->reportType);
    return -1;
  }

  return 0;
}

int reportManagerSendReports(repDesc_t *reportToBeSend /*Desc_t (*reportToBeSend)[]*/) {
  APP_DBG_PRINT("reportManagerSendReports was called !!\r\n");
  if (gREPORTMGRCtx.currState != STATE_REPORTMGR_READY_FOR_NOTIFY_REQ) {
    APP_DBG_PRINT("Fail to send Notify with report at state %d\r\n", gREPORTMGRCtx.currState);
    return 1;
  }

  if (checkReportType(reportToBeSend) != 0) {
    APP_DBG_PRINT("Contains invalid report type!!\r\n");
    return 1;
  }

  REPORTMGRNotifyReq_t *reqContainer = malloc(sizeof(REPORTMGRNotifyReq_t));
  APP_ASSERT(NULL != reqContainer, "malloc fail\r\n");

  memset(reqContainer, 0x0, sizeof(REPORTMGRNotifyReq_t));
  reqContainer->reportToBeSend = malloc(sizeof(repDesc_t));
  APP_ASSERT(NULL != reqContainer->reportToBeSend, "malloc fail\r\n");

  reqContainer->reportToBeSend->reportType = reportToBeSend->reportType;
  if (reportToBeSend->reportType == REPORT_TYPE_LOCATION) {
    reqContainer->reportToBeSend->u.lwm2m_location = reportToBeSend->u.lwm2m_location;
  } else if (reportToBeSend->reportType == REPORT_TYPE_CELLID) {
    reqContainer->reportToBeSend->u.lwm2m_cell_id = reportToBeSend->u.lwm2m_cell_id;
  } else if (reportToBeSend->reportType == REPORT_TYPE_APPDATA_CONTAINER) {
    reqContainer->reportToBeSend->u.lwm2m_app_data = reportToBeSend->u.lwm2m_app_data;
  } else {
    APP_DBG_PRINT("Contains invalid report type!!\r\n");
    return 1;
  }

  EVT_SEND(&gREPORTMGRCtx, EVT_REPORTMGR_REQ_TO_SEND_RPRT, reqContainer);
  return 0;
}
