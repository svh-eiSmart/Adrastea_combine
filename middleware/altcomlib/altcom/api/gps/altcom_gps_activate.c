/****************************************************************************
 * modules/lte/altcom/api/gps/altcom_gps_activate.c
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>
#include <stdbool.h>

#include "dbg_if.h"
#include "apiutil.h"
#include "altcom_cc.h"
#include "apicmd_gps_activate.h"

#define ACTIVATE_REQ_SUCCESS 0
#define ACTIVATE_REQ_FAILURE -1

struct gps_activate_s {
  int mode;
  int tolerence;
};

static int gps_active_req(struct gps_activate_s *req) {
  FAR struct apicmd_activate_s *cmd = NULL;
  FAR struct apicmd_activateres_s *res = NULL;
  uint16_t reslen = 0;
  int32_t ret;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GPS_ACTIVATE,
                                          ACTIVATE_REQ_DATALEN, (FAR void **)&res,
                                          ACTIVATE_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return ACTIVATE_REQ_FAILURE;
  }

  memset(cmd, 0, sizeof(struct apicmd_activate_s));

  cmd->mode = htonl(req->mode);
  cmd->tolerence = htonl(req->tolerence);

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, ACTIVATE_RES_DATALEN, &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != ACTIVATE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  ret = (int32_t)res->result;

  DBGIF_LOG1_DEBUG("[active-res]ret: %ld\n", ret);

  if (ret != APICMD_ACTIVATE_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(cmd, res);

  return ACTIVATE_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return ACTIVATE_REQ_FAILURE;
}

int altcom_gps_activate(int mode, int tolerence) {
  struct gps_activate_s req;
  int result;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  req.mode = mode;
  req.tolerence = tolerence;

  result = gps_active_req(&req);

  if (result != ACTIVATE_REQ_SUCCESS) {
    return APICMD_ACTIVATE_RES_RET_CODE_ERR;
  }

  return APICMD_ACTIVATE_RES_RET_CODE_OK;
}
