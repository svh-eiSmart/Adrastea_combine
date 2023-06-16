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
#include <string.h>
#include <stdbool.h>

#include "dbg_if.h"
#include "apiutil.h"
#include "altcom_cc.h"
#include "altcom_gps.h"
#include "apicmd_gps_info.h"

#define INFO_REQ_SUCCESS 0
#define INFO_REQ_FAILURE -1

static int altcom_gps_info_req(int req, void *result) {
  FAR struct apicmd_info_s *info_req = NULL;
  FAR struct apicmd_infores_s *res = NULL;
  int32_t ret;
  unsigned int i;
  uint16_t reslen = 0;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&info_req, APICMDID_GPS_INFO,
                                          INFO_REQ_DATALEN, (FAR void **)&res, INFO_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return INFO_REQ_FAILURE;
  }

  memset(info_req, 0, sizeof(struct apicmd_info_s));

  info_req->cmd = htonl(req);

  ret = apicmdgw_send((FAR uint8_t *)info_req, (FAR uint8_t *)res, INFO_RES_DATALEN, &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != INFO_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  ret = APICMD_INFO_RES_RET_CODE_OK;

  switch (ntohl(res->cmd)) {
    case INFO_SAT_CMD: {
      infoSatResult_t *satResult = (infoSatResult_t *)result;
      satResult->numofsats = ntohl(res->u.sat.numofsats);
      for (i = 0; i < satResult->numofsats; i++) {
        satResult->satlist[i].PRN = ntohl(res->u.sat.satlist[i].prn);
        satResult->satlist[i].elevation = ntohl(res->u.sat.satlist[i].elevation);
        satResult->satlist[i].azimuth = ntohl(res->u.sat.satlist[i].azimuth);
        satResult->satlist[i].SNR = ntohl(res->u.sat.satlist[i].snr);
      }
    } break;
    case INFO_EPH_CMD: {
      infoEphResult_t *ephStat = (infoEphResult_t *)result;
      ephStat->status = ntohl(res->u.eph.status);
      ephStat->minsatage = ntohl(res->u.eph.minsatage);
      ephStat->numofsats = ntohl(res->u.eph.numofsats);
      for (int i = 0; i < ephStat->numofsats; i++) {
        ephStat->satlist[i] = ntohl(res->u.eph.satslist[i]);
      }
    } break;
    case INFO_FIX_CMD: {
      infoFixResult_t *fixResult = (infoFixResult_t *)result;
      fixResult->fixtype = ntohl(res->u.fix.fixtype);
      fixResult->latitude = res->u.fix.latitude;
      fixResult->longitude = res->u.fix.longitude;
      fixResult->altitude = res->u.fix.altitude;
      fixResult->utc = ntohll(res->u.fix.utc);
      fixResult->accuracy = res->u.fix.accuracy;
      fixResult->speed = res->u.fix.speed;
      fixResult->ephtype = res->u.fix.ephtype;
      fixResult->accuracy_is_present = res->u.fix.accuracy_is_present;
      fixResult->speed_is_present = res->u.fix.speed_is_present;
      fixResult->num_of_sats_for_fix = ntohl(res->u.fix.num_of_sats_for_fix);
    } break;
    case INFO_TTFF_CMD: {
      *((int32_t *)result) = ntohl(res->u.ttff);
    } break;
  }

  DBGIF_LOG1_DEBUG("[info-res]ret: %ld\n", ret);

  altcom_generic_free_cmdandresbuff(info_req, res);

  return INFO_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(info_req, res);
  return INFO_REQ_FAILURE;
}

int altcom_gps_info_get_ephemeris_status(infoEphResult_t *res) {
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  return altcom_gps_info_req(INFO_EPH_CMD, res);
}

int altcom_gps_info_get_ttff(int32_t *res) {
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  return altcom_gps_info_req(INFO_TTFF_CMD, res);
}

int altcom_gps_info_get_fix(infoFixResult_t *res) {
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  return altcom_gps_info_req(INFO_FIX_CMD, res);
}

int altcom_gps_info_get_sat_in_view(infoSatResult_t *res) {
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  return altcom_gps_info_req(INFO_SAT_CMD, res);
}
