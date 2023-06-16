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

int database_init(void);
int database_read(int objId, int instId, int rsrcId, lwm2m_uri_and_data_t **uridata,
                  lwm2m_uridata_free_cb_t *freecb, void *userPriv);
int database_write(int objId, int instId, int rsrcId, int rsrcInst, LWM2MInstnc_type_e instncType,
                   char *resourceVal, uint16_t resourceLen, void *userPriv);
int database_exe(int objId, int instId, int rsrcId, int rsrcInst, char *param, void *userPriv);
int database_read_obj(int objId, void *objPtr);