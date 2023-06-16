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
#include "apicmd_gps_satscfg.h"

#define SATS_REQ_SUCCESS 0
#define SATS_REQ_FAILURE -1

const satsConfigSystems_e SATS_DEFAULT_VALUE = 0;

struct gps_sats_config_s {
  int32_t cmd;
  int32_t systems;
};

int altcom_gps_satellites_cfg_req(struct gps_sats_config_s *req, void *result) {
  FAR struct apicmd_satscfg_s *sats_req = NULL;
  FAR struct apicmd_satscfgres_s *res = NULL;
  int32_t ret;
  uint16_t reslen = 0;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&sats_req, APICMDID_GPS_SATSCFG,
                                          SATS_REQ_DATALEN, (FAR void **)&res, SATS_RES_DATALEN)) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return SATS_REQ_FAILURE;
  }

  memset(sats_req, 0, sizeof(struct apicmd_satscfg_s));
  sats_req->cmd = htonl(req->cmd);
  sats_req->gnss_systems_to_config =htonl(req->systems);

  ret = apicmdgw_send((FAR uint8_t *)sats_req, (FAR uint8_t *)res, SATS_RES_DATALEN, &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != SATS_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  if (req->cmd == GPSSATSCFG_GET) {
    *((satsConfigSystems_e *)result) = ntohl(res->gnss_systems_config);
    ret = ntohl(res->result);
  } else {
    ret = ntohl(res->result);
  }

  DBGIF_LOG1_DEBUG("[sats-res]ret: %ld\n", ret);

  if (ret != APICMD_SATS_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(sats_req, res);

  return SATS_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(sats_req, res);
  return SATS_REQ_FAILURE;
}

int altcom_gps_set_satellites_cfg(satsConfigSystems_e systems_to_config) {
  struct gps_sats_config_s req;
  int *res = NULL;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSSATSCFG_SET;
  req.systems = systems_to_config;

  return altcom_gps_satellites_cfg_req(&req, res);
}

int altcom_gps_get_satellites_cfg(satsConfigSystems_e *res) {
  struct gps_sats_config_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return -1;
  }

  req.cmd = GPSSATSCFG_GET;
  req.systems = SATS_DEFAULT_VALUE;

  return altcom_gps_satellites_cfg_req(&req, res);
}
