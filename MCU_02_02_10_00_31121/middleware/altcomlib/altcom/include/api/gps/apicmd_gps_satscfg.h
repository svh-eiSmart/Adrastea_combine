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
#ifndef MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_SATSCFG_H_
#define MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_SATSCFG_H_

#include "apicmd.h"

typedef enum {
	GNSS_SYSTEM_NONE = 0,
	GNSS_SYSTEM_GPS,
	GNSS_SYSTEM_GLONASS,
	GNSS_SYSTEM_BOTH
} gnssSysConfig_e;

begin_packed_struct struct apicmd_satscfg_s {
  int32_t cmd;
  int32_t gnss_systems_to_config;
} end_packed_struct;

begin_packed_struct struct apicmd_satscfgres_s {
  int32_t result;
  int32_t gnss_systems_config;
} end_packed_struct;

#define SATS_REQ_DATALEN (sizeof(struct apicmd_satscfg_s))
#define SATS_RES_DATALEN (sizeof(struct apicmd_satscfgres_s))

#define APICMD_SATS_RES_RET_CODE_OK (0)
#define APICMD_SATS_RES_RET_CODE_ERR (-1)


#endif /* MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_SATSCFG_H_ */
