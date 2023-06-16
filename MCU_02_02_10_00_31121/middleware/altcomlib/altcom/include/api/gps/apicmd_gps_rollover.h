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
#ifndef MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_ROLLOVER_H_
#define MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_ROLLOVER_H_

#include "apicmd.h"

begin_packed_struct struct apicmd_rollovercfg_s {
  int32_t cmd;
  unsigned char params;
} end_packed_struct;

begin_packed_struct struct apicmd_rollovercfgres_s { unsigned char result; } end_packed_struct;

#define ROLLOVER_REQ_DATALEN (sizeof(struct apicmd_rollovercfg_s))
#define ROLLOVER_RES_DATALEN (sizeof(struct apicmd_rollovercfgres_s))

#define APICMD_ROLLOVER_RES_RET_CODE_OK (0)
#define APICMD_ROLLOVER_RES_RET_CODE_ERR (-1)


#endif /* MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_ROLLOVER_H_ */
