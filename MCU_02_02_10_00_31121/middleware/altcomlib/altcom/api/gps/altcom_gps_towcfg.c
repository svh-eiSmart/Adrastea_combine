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
#include "apicmd_gps_towcfg.h"

#define TOW_REQ_SUCCESS 0
#define TOW_REQ_FAILURE -1

const int TOW_DEFAULT_VALUE = 0;

struct gps_towcfg_s {
  int32_t cmd;
  int32_t mode;
};

static int altcom_gps_tow_req(struct gps_towcfg_s *req, int32_t *result_value) {
  FAR struct apicmd_towcfg_s *tow_req = NULL;
  FAR struct apicmd_towcfgres_s *res = NULL;
  int32_t ret;
  uint16_t reslen = 0;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&tow_req, APICMDID_GPS_TOWCFG,
                                          TOW_REQ_DATALEN, (FAR void **)&res,
                                          TOW_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return TOW_REQ_FAILURE;
  }

  memset(tow_req, 0, sizeof(struct apicmd_towcfg_s));

  tow_req->cmd = htonl(req->cmd);
  tow_req->mode = htonl(req->mode);

  ret =
      apicmdgw_send((FAR uint8_t *)tow_req, (FAR uint8_t *)res, TOW_RES_DATALEN, &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != TOW_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  if (req->cmd == GPSTOW_GET) {
    ret = APICMD_TOW_RES_RET_CODE_OK;
    *(result_value) = ntohl(res->result);
  } else {
    ret = (int32_t)res->result;
  }

  DBGIF_LOG1_DEBUG("[tow-res]ret: %ld\n", ret);

  if (ret != APICMD_TOW_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(tow_req, res);

  return TOW_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(tow_req, res);
  return TOW_REQ_FAILURE;
}

int altcom_gps_set_time_of_week_algorithm(int val) {
  struct gps_towcfg_s req;
  int32_t *res = NULL;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSTOW_SET;
  req.mode = val;

  return altcom_gps_tow_req(&req, res);
}

int altcom_gps_get_time_of_week_algorithm(int32_t *res) {
  struct gps_towcfg_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSTOW_GET;
  req.mode = TOW_DEFAULT_VALUE;

  return altcom_gps_tow_req(&req, res);
}
