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

#ifndef REPORTMANAGER_H
#define REPORTMANAGER_H

/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Kernel Header */
#include <FreeRTOS.h>
#include <ReportManager/ReportManagerAPI.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

#define LWM2M_SECURITY_OBJECT_ID 0
#define LWM2M_SERVER_OBJECT_ID 1
#define LWM2M_ACCESS_CONTROL_OBJECT_ID 2
#define LWM2M_LOCATION_OBJECT_ID 6
#define LWM2M_CONNECTIVITY_MONITORING_OBJECT_ID 4
#define LWM2M_APPLICATION_DATA_CONTAINER_OBJECT_ID 19
#define LWM2M_LOCATION_MAX_RESOURCE_NUM 7
#define LWM2M_DATA_CONTAINER_MAX_RESOURCE_NUM 6

#define LWM2M_LOCATION_INSTANCE_NUM 0
#define LWM2M_SECURITY_INSTANCE_NUM 0
#define LWM2M_SERVER_INSTANCE_NUM 0
#define LWM2M_CELL_ID_INSTANCE_NUM 0
#define LWM2M_SMART_LABEL_DATA_INSTANCE_NUM 0

#define LWM2M_SECURITY_SERVER_URI_RSRC_NUM 0
#define LWM2M_SECURITY_BOOTSTRAP_SERVER_RSRC_NUM 1
#define LWM2M_SECURITY_SECURITY_MODE_RSRC_NUM 2
#define LWM2M_SECURITY_SHORT_SERVER_ID_RSRC_NUM 10

#define LWM2M_SERVER_SHORT_SERVER_ID_RSRC_NUM 0
#define LWM2M_SERVER_LIFTIME_RSRC_NUM 1

#define LWM2M_SHORT_SERVER_ID_NUM 0

typedef enum Lwm2SecurityMode {
  LWM2M_SECURITY_MODE_PRESHARED_KEY = 0,
  LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY = 1,
  LWM2M_SECURITY_MODE_CERT = 2,
  LWM2M_SECURITY_MODE_NO_SEC = 3,
  LWM2M_SECURITY_MODE_CERT_WITH_EST = 4
} eLwm2SecurityMode;

typedef enum {
  EVT_REPORTMGR_START,
  EVT_REPORTMGR_REGISTER_ACK,
  EVT_REPORTMGR_DEREGISTER_ACK,
  EVT_REPORTMGR_OBSERVE_START,
  EVT_REPORTMGR_OBSERVE_STOP,
  EVT_REPORTMGR_CONFIRM_NOTIFY_FAIL,
  EVT_REPORTMGR_CONFIRM_NOTIFY_ACK,
  EVT_REPORTMGR_REGISTER_UPDT_ACK,
  EVT_REPORTMGR_REQ_TO_SEND_RPRT,
  EVT_REPORTMGR_TRANSC_TIMEOUT,
  EVT_REPORTMGR_REQ_TO_SEND_REG_UPDATE,
  EVT_REPORTMGR_NOTIFY_DISCARD,  // client network was down so it was discard(storing mode off)
  EVT_REPORTMGR_MAX
} REPORTMGREvt_e;

typedef enum {
  STATE_REPORTMGR_IDLE,
  STATE_REPORTMGR_WAIT_4_OBSERVE_START,
  STATE_REPORTMGR_WAIT_4_REGISTER_ACK,
  STATE_REPORTMGR_READY_FOR_NOTIFY_REQ,
  STATE_REPORTMGR_NOTIFY_WAS_SENT,
  STATE_REPORTMGR_REG_UPDT_WAS_SENT,
  STATE_REPORTMGR_MAX
} REPORTMGRState_e;

typedef struct {
  REPORTMGREvt_e event;
  void *param;
} REPORTMGREvt_t;

typedef struct {
  repDesc_t *reportToBeSend;

} REPORTMGRNotifyReq_t;

int reportMngrBindReadInd(uint32_t objectId, reportManagerReadIndCB_t reportManagerReadCB);

int reportMngrBindWriteInd(uint32_t objectId, reportManagerWriteIndCB_t reportManagerWriteCB);

int reportMngrBindExeInd(uint32_t objectId, reportManagerExeIndCB_t reportManagerExeCB);

ReportMngrConfig_result reportManagerConfig(report_type_bitmask_t retportTypeBitMask, int force);

int reportManagerInit(reportManagerStartCB_t reportManagerResultCB, int32_t timeout,
                      report_type_bitmask_t observObjectBit, bool isRegisterRequired);

int reportManagerSendReports(repDesc_t *reportToBeSend /*Desc_t (*reportToBeSend)[]*/);
int reportManagerInitRefresh(void);
#endif
