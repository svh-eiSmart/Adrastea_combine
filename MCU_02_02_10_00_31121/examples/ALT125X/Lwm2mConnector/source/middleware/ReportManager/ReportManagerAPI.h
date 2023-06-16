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

#ifndef REPORTMANAGERAPI_H
#define REPORTMANAGERAPI_H

/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include "lte/lte_api.h"

/* Kernel Header */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

#include "lwm2m/altcom_lwm2m.h"

#define MAX_DATA_DESC_LEN 32
#define MAX_DATA_FORMAT_LEN 32
typedef enum {
  REPORT_MNGR_CONFIG_OK,
  REPORT_MNGR_CONFIG_FAIL,
  REPORT_MNGR_CONFIG_UPDATED,
} ReportMngrConfig_result;

typedef enum {
  REPORT_MNGR_RET_CODE_OK,
  REPORT_MNGR_RET_CODE_FAIL,
  REPORT_MNGR_RET_TIMEOUT
} Report_result;

typedef enum {
  Report_result_type_register_init,
  Report_result_type_register_update,
  Report_result_type_observe_req,
  Report_result_type_notify_req,
} ReportResultType_t;

typedef enum {
  REGISTER_TO_SERVER_FAlLED,
  NO_OBSERVE_WAS_RECV,
  ENABLE_HOST_FAILED,
  ENABLE_OPEV_FAILED,
  EVENT_CALLBACK_HOOK_FAIL,
  REGISTER_API_FAILED,
  SERVER_STATE_API_FAILED,
  REGISTER_STATE_MISMATCH,
  REGISTER_UPDT_API_FAILED,
  NOTIFY_DISCARD_DUE_2_RSRC_VAL_GET,
  NOTIFY_DISCARD_DUE_2_RAI_SET,
  NOTIFY_DISCARD_DUE_TO_NET_STATE,
  NOTIFY_CONFRIM_FAILED_BY_SERVER,
  UNSUPPORTED_OBSERVE_OBJ,
  UNKNOWN_REASON
} Report_fail_reason_t;

typedef struct {
  ReportResultType_t type;
  Report_result result;
  Report_fail_reason_t reasonFail;  // relevant in case that result !=
} reportResult_t;

typedef void (*reportManagerStartCB_t)(reportResult_t *reportResult);

typedef enum {
  REPORT_TYPE_LOCATION,
  REPORT_TYPE_CELLID,
  REPORT_TYPE_APPDATA_CONTAINER,
  REPORT_TYPE_MAX,
} reportType_t;

typedef struct {
  int instId;
  opaqueVal_t opaque;
} opaque_rsrc_inst_t;

typedef struct {
  int numInst;
  opaque_rsrc_inst_t *rsrc;
} opaque_rsrc_t;

typedef struct {
  uint8_t link_quality;  /**< Link quality (0-255) */
  uint32_t glob_cell_id; /**< Serving cell ID (0...268435455) */
  uint16_t mnc;          /**< Mobile Network Code (00-999) */
  uint16_t mcc;          /**< Mobile Country Code (000-999) */
} cell_id_t;

typedef struct {
  float Latitude;
  float Longitude;
  float Altitude;
  float Radius;
  opaque_rsrc_t Velocity;  // optional, opaque format which is application/octet-stream????
  time_t Timestamp;  // Unix Time. A signed integer representing the number of seconds since Jan 1st,
                  // 1970 in the UTC time zone
  float Speed;
} location_data_t;

typedef struct {
  int instId;
  opaque_rsrc_t data;
  uint8_t dataPriority;
  time_t dataCreateTime;
  char dataDesc[MAX_DATA_DESC_LEN];
  char dataFormat[MAX_DATA_FORMAT_LEN];
  uint16_t appId;
} app_data_container_inst_t;

typedef struct {
  uint16_t numInst;
  app_data_container_inst_t *obj;
} app_data_container_t;

typedef struct {
  int (*reboot)(char *param, void *userPriv);
  int (*factory_reset)(char *param, void *userPriv);
} lwm2m_device_t;

typedef struct reportDesc_t {
  reportType_t reportType;
  bool askForAckFromServer;
  union {
    location_data_t lwm2m_location;
    cell_id_t lwm2m_cell_id;
    app_data_container_t lwm2m_app_data;
  } u;
} repDesc_t;

typedef enum {
  REPORT_TYPE_LWM2M_LOCATION = 0x1,
  REPORT_TYPE_LWM2M_LTE_CELL = 0x2,
  REPORT_TYPE_LWM2M_BIN_APP_DATA = 0x4,
} report_type_bitmask_t;

/**
 * @brief
 * Definition of LWM2M resource type supported.
 */

typedef enum {
  READ_VALUE_SINGLE_TYPE_FLOAT = 0, /**< Type resource is single float */
  READ_VALUE_SINGLE_TYPE_INTEGER,   /**< Type resource is single integer */
  READ_VALUE_SINGLE_TYPE_TIME,      /**< Type resource is single TIME */
  READ_VALUE_SINGLE_TYPE_STRING,    /**< Type resource is single string */
  READ_VALUE_SINGLE_TYPE_BOOLEAN,   /**< Type resource is single Boolean */
  READ_VALUE_SINGLE_TYPE_OPAQUE,    /**< Type resource is single Opaque */
  READ_VALUE_MULT_TYPE_FLOAT = 0,   /**< Type resource is multiply float */
  READ_VALUE_MULT_TYPE_INTEGER,     /**< Type resource is  multiply integer */
  READ_VALUE_MULT_TYPE_TIME,        /**< Type resource is  multiply TIME */
  READ_VALUE_MULT_TYPE_STRING,      /**< Type resource is  multiply string */
  READ_VALUE_MULT_TYPE_BOOLEAN,     /**< Type resource is  multiply Boolean */
  READ_VALUE_MULT_TYPE_OPAQUE,      /**< Type resource is  multiply Opaque */
} readRsrc_type_e;

#define MAX_READ_STRING_TYPE_LEN \
  MAX_LWM2M_STRING_TYPE_LEN /**< Global limit of string type length */

union readResourceVal_T {
  double value;                          /**< resource value */
  char strVal[MAX_READ_STRING_TYPE_LEN]; /**< resource string value */
  opaqueVal_t opaqueVal;                 /**< resource opaque value */
};
typedef union readResourceVal_T readResourceVal_t;

typedef int (*reportManagerReadIndCB_t)(int objectId, int instanceNum, int resourceNum,
                                        lwm2m_uri_and_data_t **uridata,
                                        lwm2m_uridata_free_cb_t *freecb, void *userPriv);
typedef int (*reportManagerWriteIndCB_t)(int objectId, int instanceNum, int resourceNum,
                                         int resourceInst, LWM2MInstnc_type_e instncType,
                                         char *resourceVal, uint16_t resourceLen, void *userPriv);
typedef int (*reportManagerExeIndCB_t)(int objectId, int instanceNum, int resourceNum,
                                       int resourceInst, char *param, void *userPriv);

Report_result reportManagerInitApi(reportManagerStartCB_t reportManagerResultCB,
                                   int32_t timeoutInMsec, report_type_bitmask_t retportTypeBitMask,
                                   bool isRegisterRequired);

Report_result reportManagerSendReportsApi(repDesc_t *reportToBeSend);

Report_result reportManagerInitRefreshApi(void);

ReportMngrConfig_result reportManagerConfigApi(report_type_bitmask_t retportTypeBitMask, int force);

int reportMngrBindReadIndApi(int objectId, reportManagerReadIndCB_t reportManagerReadCB);

int reportMngrBindWriteIndApi(int objectId, reportManagerWriteIndCB_t reportManagerWriteCB);

int reportMngrBindExeIndApi(int objectId, reportManagerExeIndCB_t reportManagerExeCB);

#endif /* REPORTMANAGERAPI_H */
