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

#ifndef MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_INFO_H_
#define MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_INFO_H_

#include "apicmd.h"
#define GPS_NUM_SVID 32
#define GLN_NUM_SVID 24
#define GSV_MAX_OF_SAT 16
#define FIX_BUF_LEN 12

typedef signed char BOOL;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


struct apicmd_gpsgsv_s {
	uint32_t prn;
	uint32_t elevation;
	uint32_t azimuth;
	uint32_t snr;
};

struct apicmd_gpsinfosat_s {
	uint32_t numofsats;
	struct apicmd_gpsgsv_s satlist[GSV_MAX_OF_SAT];
};

struct apicmd_gpsinfoeph_s {
	int32_t status;
	int32_t minsatage;
	int32_t numofsats;
	int32_t satslist[GPS_NUM_SVID + GLN_NUM_SVID];
};

struct apicmd_gpsinfofix_s {
	float fixtime;
	float accuracy;
	float speed;
	double latitude;;
	double longitude;
	double altitude;
	int32_t fixtype;
	int64_t utc;
	char ephtype;
	BOOL accuracy_is_present;
	BOOL speed_is_present;
	int32_t num_of_sats_for_fix;
};

begin_packed_struct struct apicmd_info_s { int32_t cmd; } end_packed_struct;

begin_packed_struct struct apicmd_infores_s {
	int32_t cmd;
	union {
		struct apicmd_gpsinfoeph_s eph;
		struct apicmd_gpsinfosat_s sat;
		struct apicmd_gpsinfofix_s fix;
		int32_t ttff;
	} u;
} end_packed_struct;

#define INFO_REQ_DATALEN (sizeof(struct apicmd_info_s))
#define INFO_RES_DATALEN (sizeof(struct apicmd_infores_s))

#define APICMD_INFO_RES_RET_CODE_OK (0)
#define APICMD_INFO_RES_RET_CODE_ERR (-1)

#endif /* MIDDLEWARE_ALTCOMLIB_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_INFO_H_ */
