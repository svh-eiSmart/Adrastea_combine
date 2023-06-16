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
#include "apicmd_gps_blankingcfg.h"

#define BLANKING_REQ_SUCCESS 0
#define BLANKING_REQ_FAILURE -1
const int BLANKING_DEFAULT_VALUE = 0;

struct gps_blanking_s {
	int32_t cmd;
	int32_t mode;
	uint32_t guardtime;
	int32_t activate_upon_disallow;
	bool guard_time_enabled;
	bool activate_upon_disallow_enabled;
};

static int altcom_gps_blanking_req(struct gps_blanking_s *req, void *result) {
  FAR struct apicmd_blankingcfg_s *blanking_req = NULL;
  FAR struct apicmd_blankingcfgres_s *res = NULL;
  int32_t ret;
  uint16_t reslen = 0;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&blanking_req, APICMDID_GPS_BLANKINGCFG,
                                          BLANKING_REQ_DATALEN, (FAR void **)&res,
                                          BLANKING_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return BLANKING_REQ_FAILURE;
  }

  memset(blanking_req, 0, sizeof(struct apicmd_blankingcfg_s));
  blanking_req->cmd = htonl(req->cmd);
  blanking_req->mode = htonl(req->mode);
  blanking_req->guard_time = htonl(req->guardtime);
  blanking_req->activate_upon_disallow = htonl(req->activate_upon_disallow);
  blanking_req->guard_time_enabled = req->guard_time_enabled;
  blanking_req->activate_upon_disallow_enabled = req->activate_upon_disallow_enabled;

  ret = apicmdgw_send((FAR uint8_t *)blanking_req, (FAR uint8_t *)res, BLANKING_RES_DATALEN,
                      &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != BLANKING_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }


  if (req->cmd == GPSBLANKING_GET) {
    blankingResult_t *status = (blankingResult_t *)result;
    status->mode = ntohl(res->mode);
    status->guardtime = ntohl(res->guard_time);
    status->activateupondisallow = ntohl(res->activate_upon_disallow);
    ret = ntohl(res->result);
  } else {
    ret = ntohl(res->result);
  }

  DBGIF_LOG1_DEBUG("[blanking-res]ret: %ld\n", ret);

  if (ret != APICMD_BLANKING_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(blanking_req, res);

  return BLANKING_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(blanking_req, res);
  return BLANKING_REQ_FAILURE;
}

int altcom_gps_getblankingcfg(blankingResult_t *res) {
  struct gps_blanking_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSBLANKING_GET;
  req.mode = BLANKING_DEFAULT_VALUE;
  req.guardtime = BLANKING_DEFAULT_VALUE;
  req.activate_upon_disallow = BLANKING_DEFAULT_VALUE;
  req.guard_time_enabled = FALSE;
  req.activate_upon_disallow_enabled = FALSE;

  return altcom_gps_blanking_req(&req, res);
}

int altcom_gps_setblankingcfg(int mode, unsigned int guardtime, int activate_upon_disallow, bool guard_time_enabled, bool activate_upon_disallow_enabled) {
  struct gps_blanking_s req;
  int *res = NULL;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSBLANKING_SET;
  req.mode = mode;
  req.guardtime = guardtime;
  req.activate_upon_disallow = activate_upon_disallow;
  req.guard_time_enabled = guard_time_enabled;
  req.activate_upon_disallow_enabled = activate_upon_disallow_enabled;

  return altcom_gps_blanking_req(&req, res);
}
