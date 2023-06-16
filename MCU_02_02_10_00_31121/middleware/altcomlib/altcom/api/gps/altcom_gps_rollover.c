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
#include "apicmd_gps_rollover.h"

#define ROLLOVER_REQ_SUCCESS 0
#define ROLLOVER_REQ_FAILURE -1
const int ROLLOVER_DEFAULT_VALUE = 0;

struct gps_rollover_s {
  int32_t cmd;
  unsigned char value;
};

static int altcom_gps_rollover_req(struct gps_rollover_s *req, unsigned char *result_value) {
  FAR struct apicmd_rollovercfg_s *ro_req = NULL;
  FAR struct apicmd_rollovercfgres_s *res = NULL;
  int32_t ret;
  uint16_t reslen = 0;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&ro_req, APICMDID_GPS_ROLLOVERCFG,
                                          ROLLOVER_REQ_DATALEN, (FAR void **)&res,
                                          ROLLOVER_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return ROLLOVER_REQ_FAILURE;
  }

  memset(ro_req, 0, sizeof(struct apicmd_rollovercfg_s));

  ro_req->cmd = htonl(req->cmd);
  ro_req->params = req->value;

  ret =
      apicmdgw_send((FAR uint8_t *)ro_req, (FAR uint8_t *)res, ROLLOVER_RES_DATALEN, &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != ROLLOVER_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  if (req->cmd == GPSROLLOVER_GET) {
    ret = APICMD_ROLLOVER_RES_RET_CODE_OK;
    *(result_value) = res->result;
  } else {
    ret = (int32_t)res->result;
  }

  DBGIF_LOG1_DEBUG("[rollover-res]ret: %ld\n", ret);

  if (ret != APICMD_ROLLOVER_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(ro_req, res);

  return ROLLOVER_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(ro_req, res);
  return ROLLOVER_REQ_FAILURE;
}

int altcom_gps_setrollover(int val) {
  struct gps_rollover_s req;
  unsigned char *res = NULL;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSROLLOVER_SET;
  req.value = val;

  return altcom_gps_rollover_req(&req, res);
}

int altcom_gps_getrollover(unsigned char *res) {
  struct gps_rollover_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSROLLOVER_GET;
  req.value = ROLLOVER_DEFAULT_VALUE;

  return altcom_gps_rollover_req(&req, res);
}
