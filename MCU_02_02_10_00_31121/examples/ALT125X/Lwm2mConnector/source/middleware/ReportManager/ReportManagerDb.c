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
/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "apiutil.h"
#include "Infra/app_dbg.h"

#include "lwm2m/altcom_lwm2m.h"
#include "ReportManagerDb.h"
#define INVALID_OBJID (0xFFFFFFFF)

typedef struct {
  uint32_t objectId;
  void *objectWriteCbPtr;
  void *objectReadCbPtr;
  void *objectExeCbPtr;
} objectsDescDb_t;
#define MAX_NUM_OF_OBJECTS_SUPPORTED 10

static char observerToken[MAX_NUM_OF_OBJECTS_SUPPORTED][LWM2M_OBSERVE_TOKEN_STR_LEN + 1];
bool isObserverTokenInit = false;

static uint16_t objNumMapVal[MAX_NUM_OF_OBJECTS_SUPPORTED] = {
    3, 4, 6, 19, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};

static objectsDescDb_t objectsDescDb[MAX_NUM_OF_OBJECTS_SUPPORTED];
bool isObjectDescDBInit = false;

void *getObjectCbInDb(uint32_t objectId, objectCallBackFuncType_t objectCallBackFuncType) {
  unsigned char index, found = 0;

  // 1st find if the object is included
  for (index = 0; index < MAX_NUM_OF_OBJECTS_SUPPORTED; index++) {
    if (objectsDescDb[index].objectId == objectId) {
      found = 1;
      break;
    }
  }

  if (found == 0) {
    return NULL;
  } else {
    if (objectCallBackFuncType == OBJ_CBTYPE_READ) {
      return objectsDescDb[index].objectReadCbPtr;
    } else if (objectCallBackFuncType == OBJ_CBTYPE_WRITE) {
      return objectsDescDb[index].objectWriteCbPtr;
    } else if (objectCallBackFuncType == OBJ_CBTYPE_EXE) {
      return objectsDescDb[index].objectExeCbPtr;
    } else {
      return NULL;
    }
  }
}

int updateObjectCbInDb(uint32_t objectId, objectCallBackFuncType_t objectCallBackFuncType,
                       void *cbfunction) {
  unsigned char index, found = 0;

  if (!isObjectDescDBInit) {
    memset((void *)objectsDescDb, 0, sizeof(objectsDescDb));
    for (index = 0; index < MAX_NUM_OF_OBJECTS_SUPPORTED; index++) {
      objectsDescDb[index].objectId = INVALID_OBJID;
    }

    isObjectDescDBInit = true;
  }

  // 1st find if the object is included
  for (index = 0; index < MAX_NUM_OF_OBJECTS_SUPPORTED; index++) {
    if (objectsDescDb[index].objectId == objectId) {
      found = 1;
      if (objectCallBackFuncType == OBJ_CBTYPE_READ) {
        objectsDescDb[index].objectReadCbPtr = cbfunction;
      } else if (objectCallBackFuncType == OBJ_CBTYPE_WRITE) {
        objectsDescDb[index].objectWriteCbPtr = cbfunction;
      } else if (objectCallBackFuncType == OBJ_CBTYPE_EXE) {
        objectsDescDb[index].objectExeCbPtr = cbfunction;
      }

      // Check if all callback are NULL, implies to remove obj callback
      if (!objectsDescDb[index].objectReadCbPtr && !objectsDescDb[index].objectWriteCbPtr &&
          !objectsDescDb[index].objectExeCbPtr) {
        objectsDescDb[index].objectId = INVALID_OBJID;
      }

      break;
    }
  }
  if (found == 0) {
    // 1st find free place in DB to add the object
    for (index = 0; index < MAX_NUM_OF_OBJECTS_SUPPORTED; index++) {
      if (objectsDescDb[index].objectId == INVALID_OBJID) {
        found = 1;
        objectsDescDb[index].objectId = objectId;
        if (objectCallBackFuncType == OBJ_CBTYPE_READ) {
          objectsDescDb[index].objectReadCbPtr = cbfunction;
        } else if (objectCallBackFuncType == OBJ_CBTYPE_WRITE) {
          objectsDescDb[index].objectWriteCbPtr = cbfunction;
        } else if (objectCallBackFuncType == OBJ_CBTYPE_EXE) {
          objectsDescDb[index].objectExeCbPtr = cbfunction;
        }

        break;
      }
    }
  }
  if (found == 0) {
    APP_DBG_PRINT("objectsDescDb full!\r\n");
    return -1;  // DB is full
  }
  return 0;
}

int getValFromDb(uint32_t objectId, int instanceNum, int resourceNum, int *rsrcType,
                 int *numRsrcIns, int *resourceInst, LWM2MRsrc_type_e *valType,
                 resourceVal_t *resourceVal) {
  static char config_obj_str[] = "Bayer-config";
  *valType = RSRC_VALUE_TYPE_INTEGER;
  *resourceInst = 0;
  *rsrcType = RSRC_INSTNC_TYPE_SINGLE;

  switch (resourceNum) {
    case 0:
      if (objectId == 6) {
        *valType = RSRC_VALUE_TYPE_FLOAT;
        resourceVal->value = 32.13322000;
      } else if (objectId == 19) {
        static char opaqueVal_str[5] = "1234";
        *valType = RSRC_VALUE_TYPE_OPAQUE;
        *rsrcType = RSRC_INSTNC_TYPE_MULTIPLE;
        *numRsrcIns = 1;
        opaqueVal_str[3] = 0xa9;
        resourceVal->opaqueVal.opaqueValLen = 4;
        resourceVal->opaqueVal.opaqueValPtr = (unsigned char *)&opaqueVal_str[0];
      } else if (objectId == 33261) {
        *valType = RSRC_VALUE_TYPE_STRING;
        strncpy(resourceVal->strVal, &config_obj_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
      } else {
        resourceVal->value = 37;
      }

      break;
    case 1:
      if (objectId == 6) {
        resourceVal->value = 34.89709500;
        *valType = RSRC_VALUE_TYPE_FLOAT;
      } else if (objectId == 33260) {
        resourceVal->value = 0;

      } else if (objectId == 33261) {
        *valType = RSRC_VALUE_TYPE_STRING;
        strncpy(resourceVal->strVal, &config_obj_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
      } else {
        resourceVal->value = 0;
        *rsrcType = RSRC_INSTNC_TYPE_MULTIPLE;
        *numRsrcIns = 1;
      }

      break;
    case 2:
      resourceVal->value = 100;
      if (objectId == 6) {
        *valType = RSRC_VALUE_TYPE_FLOAT;
      } else if (objectId == 33261) {
        *valType = RSRC_VALUE_TYPE_STRING;
        strncpy(resourceVal->strVal, &config_obj_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
      }
      break;
    case 3:
      resourceVal->value = 32;
      if (objectId == 6) {
        *valType = RSRC_VALUE_TYPE_FLOAT;
      } else if (objectId == 33261) {
        *valType = RSRC_VALUE_TYPE_STRING;
        strncpy(resourceVal->strVal, &config_obj_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
      }
      break;
    case 4:
      if ((objectId == 6) || (objectId == 33260)) {
        resourceVal->value = -1;
      } else if (objectId == 33261) {
        *valType = RSRC_VALUE_TYPE_BOOLEAN;
        resourceVal->value = 0;
      } else {
        *valType = RSRC_VALUE_TYPE_STRING;
        static char IP_str[] = "192.168.1.1";
        strncpy(resourceVal->strVal, &IP_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
        *rsrcType = RSRC_INSTNC_TYPE_MULTIPLE;
        *numRsrcIns = 1;
      }
      break;
    case 5:
      resourceVal->value = 1572358581000;
      if (objectId == 6) {
        *valType = RSRC_VALUE_TYPE_TIME;
      }

      break;
    case 6:
      resourceVal->value = 100;
      if (objectId == 6) {
        *valType = RSRC_VALUE_TYPE_FLOAT;
      }
      break;
    case 7:
      if (objectId == 33260) {
        resourceVal->value = 107;
      } else {
        *valType = RSRC_VALUE_TYPE_STRING;
        static char APN_str[] = "100";
        strncpy(resourceVal->strVal, &APN_str[0], MAX_LWM2M_STRING_TYPE_LEN - 1);
        *rsrcType = RSRC_INSTNC_TYPE_MULTIPLE;
        *numRsrcIns = 1;
      }
      break;
    case 8:  //(Cell ID)
      resourceVal->value = 3953441;
      break;
    case 9:  // (SMNC)
      resourceVal->value = 2;
      break;
    case 10:  //(SMCC)
      resourceVal->value = 425;
      break;
    case 100:
      *valType = RSRC_VALUE_TYPE_BOOLEAN;
      *rsrcType = RSRC_INSTNC_TYPE_MULTIPLE;
      resourceVal->value = false;
      *numRsrcIns = 5;
      *resourceInst = 1;  // Patch refer ONLY to instance 1 of this rsrc
      break;
    default:
      if (objectId == 33261) {
        resourceVal->value = 777;
      }
      return -1;
  }
  return 0;
}

int getObserverToken(objObserveNum_t objNum, char **token) {
  if (!isObserverTokenInit) {
    memset(observerToken, 0x0, sizeof(observerToken));
    isObserverTokenInit = true;
  }

  if (objNum >= OBJ_MAX) {
    APP_DBG_PRINT("Invalid objNum %lu.\n", (uint32_t)objNum);
    return -1;
  }

  *token = observerToken[objNum];
  return 0;
}

int setObserverToken(objObserveNum_t objNum, char *token) {
  if (!isObserverTokenInit) {
    memset(observerToken, 0x0, sizeof(observerToken));
    isObserverTokenInit = true;
  }

  if (objNum >= OBJ_MAX) {
    APP_DBG_PRINT("Invalid objNum %lu.\n", (uint32_t)objNum);
    return -1;
  }

  strncpy(observerToken[objNum], token, LWM2M_OBSERVE_TOKEN_STR_LEN);
  APP_DBG_PRINT("set object %d token given by observe event :%s.\n", objNumMapVal[objNum], token);
  return 0;
}