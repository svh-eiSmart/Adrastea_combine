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
#include "lwm2m/altcom_lwm2m.h"

typedef enum { OBJ_CBTYPE_WRITE, OBJ_CBTYPE_READ, OBJ_CBTYPE_EXE } objectCallBackFuncType_t;
typedef enum { OBJ_3, OBJ_4, OBJ_6, OBJ_19, OBJ_MAX } objObserveNum_t;

int getValFromDb(uint32_t objectId, int instanceNum, int resourceNum, int *rsrcType,
                 int *numRsrcIns, int *resourceInst, LWM2MRsrc_type_e *valType,
                 resourceVal_t *resourceVal);

int getObserverToken(objObserveNum_t objNum, char **token);
int setObserverToken(objObserveNum_t objNum, char *token);

int updateObjectCbInDb(uint32_t objectId, objectCallBackFuncType_t objectCallBackFuncType,
                       void *cbfunction);
void *getObjectCbInDb(uint32_t objectId, objectCallBackFuncType_t objectCallBackFuncType);
