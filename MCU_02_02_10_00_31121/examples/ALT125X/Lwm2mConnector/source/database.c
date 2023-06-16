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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "Infra/app_dbg.h"
#include "lwm2m/altcom_lwm2m.h"

#include "ReportManager/ReportManagerAPI.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MAX_OBJ19_INSTNUM 1
#define MAX_OBJ19_0_0_INSTNUM 2
#define MAX_OBJ19_0_0_SIZE 10

#if defined(__GNUC__)
#define CTIME_R(time, buf) ctime_r(time, buf)
#elif defined(__ICCARM__)
#define CTIME_R(time, buf) !ctime_s(buf, sizeof(buf), time)
#else
#error time function not supported in this toolchain
#endif

/****************************************************************************
 * Private Data/Function Type
 ****************************************************************************/
typedef int (*databaseReadIndCB_t)(int instanceNum, int resourceNum, lwm2m_uri_and_data_t **uridata,
                                   lwm2m_uridata_free_cb_t *freecb, void *userPriv);
typedef int (*databaseWriteIndCB_t)(int instanceNum, int resourceNum, int resourceInst,
                                    LWM2MInstnc_type_e instncType, char *resourceVal,
                                    uint16_t resourceLen, void *userPriv);
typedef int (*databaseExeIndCB_t)(int instanceNum, int resourceNum, int resourceInst, char *param,
                                  void *userPriv);
static int reboot_example_cb(char *param, void *userPriv);
static int factory_reset_example_cb(char *param, void *userPriv);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static lwm2m_device_t mydevice = {.reboot = reboot_example_cb,
                                  .factory_reset = factory_reset_example_cb};
static location_data_t mylocation = {.Latitude = 24.84180529,
                                     .Longitude = 121.0191201,
                                     .Altitude = 5.0,
                                     .Radius = 3.0,
                                     .Velocity = {0, NULL},
                                     .Timestamp = 1598951180,
                                     .Speed = 100};

static cell_id_t myconninfo = {.link_quality = 30, .glob_cell_id = 11223, .mcc = 1, .mnc = 1};

static app_data_container_t mydatacontainer;
static uint8_t sampledata1[MAX_OBJ19_0_0_SIZE] = {0x48, 0x65, 0x6C, 0x6C, 0x6F,
                                                  0x57, 0x6F, 0x72, 0x6C, 0x64}; /* HelloWorld */
static uint8_t sampledata2[MAX_OBJ19_0_0_SIZE] = {0x57, 0x6F, 0x72, 0x6C, 0x64,  /* WorldHello */
                                                  0x48, 0x65, 0x6C, 0x6C, 0x6F};

/****************************************************************************
 * Private Function
 ****************************************************************************/
static uint32_t ascstr2hex(const char *src, uint8_t *dst) {
  char hexstr[3] = {0};
  int srcidx, dstidx;

  srcidx = dstidx = 0;
  while (src[srcidx] != '\0' && src[srcidx + 1] != '\0') {
    hexstr[0] = src[srcidx++];
    hexstr[1] = src[srcidx++];
    dst[dstidx++] = (uint8_t)strtoul(hexstr, NULL, 16);
  }

  return dstidx;
}

static int reboot_example_cb(char *param, void *userPriv) {
  APP_DBG_PRINT("[DB] Exe %s, param \"%s\", userPriv: 0x%p\r\n", TOSTR(reboot_example_cb), param,
                userPriv);
  return 0;
}

static int factory_reset_example_cb(char *param, void *userPriv) {
  APP_DBG_PRINT("[DB] Exe %s, param \"%s\", userPriv: 0x%p\r\n", TOSTR(factory_reset_example_cb),
                param, userPriv);
  return 0;
}

static int dbObjfree(void *uridata, uint32_t instCnt) {
  if (!uridata || instCnt == 0) {
    APP_ERR_PRINT("[DB] invalid uridata: 0x%p, instCnt: %lu to free\r\n", uridata, instCnt);
    return -1;
  }

  APP_DBG_PRINT("[DB] free uridata: 0x%p, instCnt: %lu\r\n", uridata, instCnt);
  free(uridata);
  return 0;
}

static int dbObj6Read(int instId, int rsrcId, lwm2m_uri_and_data_t **uridata,
                      lwm2m_uridata_free_cb_t *freecb, void *userPriv) {
  APP_DBG_PRINT("[DB] Read /6/%d/%d\r\n", instId, rsrcId);
  if (rsrcId >= 0 && rsrcId <= 6) {
    *uridata = malloc(sizeof(lwm2m_uri_and_data_t));
    APP_ASSERT(*uridata != NULL, "malloc failed\r\n");

    (*uridata)->objectId = 6;
    (*uridata)->instanceNum = instId;
    (*uridata)->resourceNum = rsrcId;
    (*uridata)->resourceInstanceNum = 0;
    (*uridata)->LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
    (*uridata)->valType = RSRC_VALUE_TYPE_FLOAT;
  }

  switch (rsrcId) {
    case 0:
      (*uridata)->resourceVal.value = mylocation.Latitude;
      APP_DBG_PRINT("[DB] Get Latitude %f\r\n", (*uridata)->resourceVal.value);
      break;

    case 1:
      (*uridata)->resourceVal.value = mylocation.Longitude;
      APP_DBG_PRINT("[DB] Get Longitude %f\r\n", (*uridata)->resourceVal.value);
      break;

    case 2:
      (*uridata)->resourceVal.value = mylocation.Altitude;
      APP_DBG_PRINT("[DB] Get Altitude %f\r\n", (*uridata)->resourceVal.value);
      break;

    case 3:
      (*uridata)->resourceVal.value = mylocation.Radius;
      APP_DBG_PRINT("[DB] Get Radius %f\r\n", (*uridata)->resourceVal.value);
      break;

    case 4:
      (*uridata)->valType = RSRC_VALUE_TYPE_OPAQUE;
      (*uridata)->resourceVal.opaqueVal = mylocation.Velocity.rsrc->opaque;
      APP_DBG_PRINT("[DB] Get Velocity, length: %hu, ptr: 0x%p\r\n",
                    (uint16_t)((*uridata)->resourceVal.opaqueVal.opaqueValLen),
                    (void *)((*uridata)->resourceVal.opaqueVal.opaqueValPtr));
      break;

    case 5:
      (*uridata)->valType = RSRC_VALUE_TYPE_TIME;
      (*uridata)->resourceVal.value = (double)mylocation.Timestamp;

      time_t dataCreateTime;
      char timeStr[32];

      dataCreateTime = (time_t)((*uridata)->resourceVal.value);
      if (CTIME_R(&dataCreateTime, timeStr)) {
        APP_DBG_PRINT("[DB] Get Timestamp \"%s\"\r\n", timeStr);
      } else {
        APP_ERR_PRINT("[DB] Something wrong in dataCreateTime or timeStr\r\n");
      }

      break;

    case 6:
      (*uridata)->resourceVal.value = mylocation.Speed;
      APP_DBG_PRINT("[DB] Get Speed %d\r\n", (int)((*uridata)->resourceVal.value));
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid rsrcId %d\r\n", rsrcId);
      return -1;
  }

  *freecb = (lwm2m_uridata_free_cb_t)dbObjfree;
  return 1;
}

static int dbObj19Read(int instId, int rsrcId, lwm2m_uri_and_data_t **uridata,
                       lwm2m_uridata_free_cb_t *freecb, void *userPriv) {
  /* TODO:// Add multiple instance handling */
  int i;
  app_data_container_inst_t *tmpobj = NULL;
  int instCnt = 1;

  APP_DBG_PRINT("[DB] Read /19/%d/%d\r\n", instId, rsrcId);
  for (i = 0; i < mydatacontainer.numInst; i++) {
    if (mydatacontainer.obj[i].instId == instId) {
      tmpobj = &mydatacontainer.obj[i];
      break;
    }
  }

  if (!tmpobj) {
    APP_ERR_PRINT("[DB] /19/%d not found!\r\n", instId);
    return 0;
  }

  if (rsrcId >= 1 && rsrcId <= 5) {
    *uridata = malloc(sizeof(lwm2m_uri_and_data_t));
    APP_ASSERT(*uridata != NULL, "malloc failed\r\n");

    (*uridata)->objectId = 19;
    (*uridata)->instanceNum = tmpobj->instId;
    (*uridata)->resourceNum = rsrcId;
    (*uridata)->resourceInstanceNum = 0;
    (*uridata)->LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
  }

  switch (rsrcId) {
    case 0: {
      *uridata = malloc(sizeof(lwm2m_uri_and_data_t) * tmpobj->data.numInst);
      APP_ASSERT(*uridata != NULL, "malloc failed\r\n");

      for (instCnt = 0; instCnt < tmpobj->data.numInst; instCnt++) {
        (*uridata)[instCnt].objectId = 19;
        (*uridata)[instCnt].instanceNum = tmpobj->instId;
        (*uridata)[instCnt].resourceNum = rsrcId;
        (*uridata)[instCnt].resourceInstanceNum = tmpobj->data.rsrc[instCnt].instId;
        (*uridata)[instCnt].LWM2MInstncType = RSRC_INSTNC_TYPE_MULTIPLE;
        (*uridata)[instCnt].valType = RSRC_VALUE_TYPE_OPAQUE;
        (*uridata)[instCnt].resourceVal.opaqueVal = tmpobj->data.rsrc[instCnt].opaque;
        APP_DBG_PRINT("[DB] Get data, length: %hu, ptr: 0x%p\r\n",
                      (uint16_t)((*uridata)[instCnt].resourceVal.opaqueVal.opaqueValLen),
                      (void *)((*uridata)[instCnt].resourceVal.opaqueVal.opaqueValPtr));
      }

      break;
    }

    case 1: {
      (*uridata)->valType = RSRC_VALUE_TYPE_INTEGER;
      (*uridata)->resourceVal.value = (double)tmpobj->dataPriority;
      APP_DBG_PRINT("[DB] Get dataPriority %d\r\n", (int)((*uridata)->resourceVal.value));
      break;
    }

    case 2: {
      (*uridata)->valType = RSRC_VALUE_TYPE_TIME;
      (*uridata)->resourceVal.value = (double)tmpobj->dataCreateTime;

      time_t dataCreateTime;
      char timeStr[32];

      dataCreateTime = (int)(*uridata)->resourceVal.value;
      if (CTIME_R(&dataCreateTime, timeStr)) {
        APP_DBG_PRINT("[DB] Get dataCreateTime \"%s\"\r\n", timeStr);
      } else {
        APP_ERR_PRINT("[DB] Something wrong in dataCreateTime or timeStr\r\n");
      }

      break;
    }

    case 3: {
      (*uridata)->valType = RSRC_VALUE_TYPE_STRING;
      snprintf((*uridata)->resourceVal.strVal, MAX_LWM2M_STRING_TYPE_LEN, "%s", tmpobj->dataDesc);
      APP_DBG_PRINT("[DB] Get dataDesc \"%s\"\r\n", (*uridata)->resourceVal.strVal);
      break;
    }

    case 4: {
      (*uridata)->valType = RSRC_VALUE_TYPE_STRING;
      snprintf((*uridata)->resourceVal.strVal, MAX_LWM2M_STRING_TYPE_LEN, "%s", tmpobj->dataFormat);
      APP_DBG_PRINT("[DB] Get dataFormat \"%s\"\r\n", (*uridata)->resourceVal.strVal);
      break;
    }

    case 5: {
      (*uridata)->valType = RSRC_VALUE_TYPE_INTEGER;
      (*uridata)->resourceVal.value = (double)tmpobj->appId;
      APP_DBG_PRINT("[DB] Get appId %hu\r\n", (uint16_t)((*uridata)->resourceVal.value));
      break;
    }

    default:
      APP_ERR_PRINT("[DB] Invalid rsrcId %d\r\n", rsrcId);
      return -1;
  }

  *freecb = (lwm2m_uridata_free_cb_t)dbObjfree;
  return instCnt;
}

static int dbObj19Write(int instId, int rsrcId, int rsrcInst, LWM2MInstnc_type_e instncType,
                        char *resourceVal, uint16_t resourceLen, void *userPriv) {
  /* TODO:// Add multiple instance handling */
  int i;
  app_data_container_inst_t *tmpobj = NULL;
  // TODO: Add database write here
  APP_DBG_PRINT("[DB] Write /19/%d/%d/%d, type: %d, rsrcLen: %hu\r\n", instId, rsrcId, rsrcInst,
                (int)instncType, resourceLen);
  for (i = 0; i < mydatacontainer.numInst; i++) {
    if (mydatacontainer.obj[i].instId == instId) {
      tmpobj = &mydatacontainer.obj[i];
      break;
    }
  }

  if (!tmpobj) {
    APP_ERR_PRINT("[DB] /19/%d not found!\r\n", instId);
    return -1;
  }

  switch (rsrcId) {
    case 0: {
      free(tmpobj->data.rsrc[rsrcInst].opaque.opaqueValPtr);
      tmpobj->data.rsrc[rsrcInst].opaque.opaqueValPtr = malloc(resourceLen);
      APP_ASSERT(tmpobj->data.rsrc[rsrcInst].opaque.opaqueValPtr != NULL, "malloc failed\r\n");

      tmpobj->data.rsrc[rsrcInst].opaque.opaqueValLen = resourceLen;
      uint32_t resLen;

      resLen = ascstr2hex(resourceVal, tmpobj->data.rsrc[rsrcInst].opaque.opaqueValPtr);
      APP_ASSERT(((resLen << 1) + 1) != (uint32_t)resourceLen,
                 "resLen = %lu, resourceLen = %hd\r\n", resLen, resourceLen);
      break;
    }

    case 1: {
      tmpobj->dataPriority = (uint8_t)atoi(resourceVal);
      APP_DBG_PRINT("[DB] Set dataPriority to %d\r\n", (int)tmpobj->dataPriority);
      break;
    }

    case 2: {
      char timeStr[32];

      tmpobj->dataCreateTime = (time_t)atoi(resourceVal);
      if (CTIME_R(&tmpobj->dataCreateTime, timeStr)) {
        APP_DBG_PRINT("[DB] Set dataCreateTime to \"%s\"\r\n", timeStr);
      } else {
        APP_ERR_PRINT("[DB] Something wrong in dataCreateTime or timeStr\r\n");
      }

      break;
    }

    case 3: {
      snprintf(tmpobj->dataDesc, MAX_DATA_DESC_LEN, "%s", resourceVal);
      APP_DBG_PRINT("[DB] Set dataDesc to \"%s\"\r\n", tmpobj->dataDesc);
      break;
    }

    case 4: {
      snprintf(tmpobj->dataFormat, MAX_DATA_FORMAT_LEN, "%s", resourceVal);
      APP_DBG_PRINT("[DB] Set dataFormat to \"%s\"\r\n", tmpobj->dataFormat);
      break;
    }

    case 5: {
      tmpobj->appId = (uint16_t)atoi(resourceVal);
      APP_DBG_PRINT("[DB] Set appId to %d\r\n", (int)tmpobj->appId);
      break;
    }

    default:
      APP_ERR_PRINT("[DB] Invalid rsrcId %d\r\n", rsrcId);
      return -1;
  }

  return 0;
}

static int dbObj4Read(int instId, int rsrcId, lwm2m_uri_and_data_t **uridata,
                      lwm2m_uridata_free_cb_t *freecb, void *userPriv) {
  APP_DBG_PRINT("[DB] Read /4/%d/%d\r\n", instId, rsrcId);

  if (rsrcId == 3 || (rsrcId >= 8 && rsrcId <= 10)) {
    *uridata = malloc(sizeof(lwm2m_uri_and_data_t));
    APP_ASSERT(*uridata != NULL, "malloc failed\r\n");

    (*uridata)->objectId = 4;
    (*uridata)->instanceNum = instId;
    (*uridata)->resourceNum = rsrcId;
    (*uridata)->resourceInstanceNum = 0;
    (*uridata)->LWM2MInstncType = RSRC_INSTNC_TYPE_SINGLE;
    (*uridata)->valType = RSRC_VALUE_TYPE_INTEGER;
  }

  switch (rsrcId) {
    case 3:
      (*uridata)->resourceVal.value = myconninfo.link_quality;
      APP_DBG_PRINT("[DB] Get link_quality %d\r\n", (int)((*uridata)->resourceVal.value));
      break;

    case 8:
      (*uridata)->resourceVal.value = myconninfo.glob_cell_id;
      APP_DBG_PRINT("[DB] Get glob_cell_id %d\r\n", (int)((*uridata)->resourceVal.value));
      break;

    case 9:
      (*uridata)->resourceVal.value = myconninfo.mnc;
      APP_DBG_PRINT("[DB] Get mnc %d\r\n", (int)((*uridata)->resourceVal.value));
      break;

    case 10:
      (*uridata)->resourceVal.value = myconninfo.mnc;
      APP_DBG_PRINT("[DB] Get mcc %d\r\n", (int)((*uridata)->resourceVal.value));
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid rsrcId %d\r\n", rsrcId);
      return -1;
  }

  *freecb = (lwm2m_uridata_free_cb_t)dbObjfree;
  return 1;
}

static int dbObj3Exe(int instId, int rsrcId, int rsrcInst, char *param, void *userPriv) {
  switch (rsrcId) {
    case 4:
      return mydevice.reboot(param, userPriv);
    case 5:
      return mydevice.factory_reset(param, userPriv);
    default:
      return -1;
  }
}

/****************************************************************************
 * Public Function
 ****************************************************************************/
int database_init(void) {
  /* Initialize obj 6 */
  APP_DBG_PRINT("Initialize /6\r\n");
  /* Assign /6/0/4 instId */
  mylocation.Velocity.numInst = 1;
  mylocation.Velocity.rsrc = malloc(sizeof(opaque_rsrc_inst_t));
  APP_ASSERT(mylocation.Velocity.rsrc != NULL, "malloc failed\r\n");

  /* Assign /6/0/4 instId */
  mylocation.Velocity.rsrc->instId = 0;
  /* Assign /6/0/4 opaqque length */
  mylocation.Velocity.rsrc->opaque.opaqueValLen = sizeof(sampledata1);
  /* Allocate for /6/0/4 opaque data */
  mylocation.Velocity.rsrc[0].opaque.opaqueValPtr = malloc(sizeof(sampledata1));
  APP_ASSERT(mylocation.Velocity.rsrc[0].opaque.opaqueValPtr != NULL, "malloc failed\r\n");

  /* Copy sample data into /6/0/4 opaque data */
  memcpy((void *)mylocation.Velocity.rsrc[0].opaque.opaqueValPtr, (void *)sampledata1,
         sizeof(sampledata1));
  /* Assign /6/0/5 */
  mylocation.Timestamp = time(NULL);

  /* Initialize obj 19 */
  APP_DBG_PRINT("Initialize /19\r\n");
  mydatacontainer.numInst = MAX_OBJ19_INSTNUM;
  mydatacontainer.obj =
      (app_data_container_inst_t *)malloc(MAX_OBJ19_INSTNUM * sizeof(app_data_container_inst_t));
  APP_ASSERT(mydatacontainer.obj != NULL, "malloc failed\r\n");

  int i, j;

  for (i = 0; i < MAX_OBJ19_INSTNUM; i++) {
    mydatacontainer.obj[i].instId = i;
    /* Assign /19/i/0 */
    /* Assign /19/i/0 inst number*/
    mydatacontainer.obj[i].data.numInst = MAX_OBJ19_0_0_INSTNUM;

    /* Allocate for /19/i inst ptr*/
    mydatacontainer.obj[i].data.rsrc = malloc(MAX_OBJ19_0_0_INSTNUM * sizeof(opaque_rsrc_inst_t));
    APP_ASSERT(mydatacontainer.obj[i].data.rsrc != NULL, "malloc failed\r\n");

    for (j = 0; j < MAX_OBJ19_0_0_INSTNUM; j++) {
      /* Assign /19/i/0/j instId */
      mydatacontainer.obj[i].data.rsrc[j].instId = j;
      /* Assign /19/i/0/j opaqque length */
      mydatacontainer.obj[i].data.rsrc[j].opaque.opaqueValLen =
          j & 0x01 ? sizeof(sampledata2) : sizeof(sampledata1);
      /* Allocate for /19/i/0/j opaque data */
      mydatacontainer.obj[i].data.rsrc[j].opaque.opaqueValPtr =
          malloc(j & 0x01 ? sizeof(sampledata2) : sizeof(sampledata1));
      APP_ASSERT(mydatacontainer.obj[i].data.rsrc[j].opaque.opaqueValPtr != NULL,
                 "malloc failed\r\n");

      /* Copy sample data into /19/i/0/j opaque data */
      memcpy((void *)mydatacontainer.obj[i].data.rsrc[j].opaque.opaqueValPtr,
             j & 0x01 ? (void *)sampledata2 : (void *)sampledata1,
             j & 0x01 ? sizeof(sampledata2) : sizeof(sampledata1));
    }

    /* Assign /19/i/1 */
    mydatacontainer.obj[i].dataPriority = 1;
    /* Assign /19/i/2 */
    mydatacontainer.obj[i].dataCreateTime = time(NULL);
    /* Assign /19/i/3 */
    snprintf(mydatacontainer.obj[i].dataDesc, MAX_DATA_DESC_LEN, "Test data /19/0/%d resource", i);
    /* Assign /19/i/4 */
    strncpy(mydatacontainer.obj[i].dataFormat, "Binary-Data", MAX_DATA_FORMAT_LEN);
    /* Assign /19/i/5 */
    mydatacontainer.obj[i].appId = (uint16_t)0xBEEF;
  }

  return 0;
}

int database_read(int objId, int instId, int rsrcId, lwm2m_uri_and_data_t **uridata,
                  lwm2m_uridata_free_cb_t *freecb, void *userPriv) {
  databaseReadIndCB_t cbPtr;
  switch (objId) {
    case 4:
      cbPtr = dbObj4Read;
      break;

    case 6:
      cbPtr = dbObj6Read;
      break;

    case 19:
      cbPtr = dbObj19Read;
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid objId %d=lu\r\n", objId);
      return -1;
  }

  return cbPtr(instId, rsrcId, uridata, freecb, userPriv);
}

int database_write(int objId, int instId, int rsrcId, int rsrcInst, LWM2MInstnc_type_e instncType,
                   char *resourceVal, uint16_t resourceLen, void *userPriv) {
  databaseWriteIndCB_t cbPtr;
  switch (objId) {
    case 19:
      cbPtr = dbObj19Write;
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid objId %lu\r\n", objId);
      return -1;
  }

  return cbPtr(instId, rsrcId, rsrcInst, instncType, resourceVal, resourceLen, userPriv);
}

int database_exe(int objId, int instId, int rsrcId, int rsrcInst, char *param, void *userPriv) {
  databaseExeIndCB_t cbPtr;
  switch (objId) {
    case 3:
      cbPtr = dbObj3Exe;
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid objId %lu\r\n", objId);
      return -1;
  }

  return cbPtr(instId, rsrcId, rsrcInst, param, userPriv);
}

int database_read_obj(int objId, void *objPtr) {
  if (!objPtr) {
    APP_ERR_PRINT("[DB] Invalid objPtr\r\n");
    return -1;
  }

  APP_DBG_PRINT("[DB] Read objId %d\r\n", objId);
  switch (objId) {
    case 4:
      *((cell_id_t *)objPtr) = myconninfo;
      break;

    case 6:
      *((location_data_t *)objPtr) = mylocation;
      break;

    case 19:
      *((app_data_container_t *)objPtr) = mydatacontainer;
      break;

    default:
      APP_ERR_PRINT("[DB] Invalid objId %d\r\n", objId);
      return -1;
  }

  return 0;
}