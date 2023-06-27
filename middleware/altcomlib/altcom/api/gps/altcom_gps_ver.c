/****************************************************************************
 * modules/lte/altcom/api/gps/altcom_gps_ver.c
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
#include "apicmd_gps_ver.h"

#define VER_REQ_SUCCESS 0
#define VER_REQ_FAILURE -1

static int gps_ver_req(char *version) {
  FAR struct apicmd_activate_s *cmd = NULL;
  FAR struct apicmd_versionres_s *res = NULL;
  uint16_t reslen = 0;
  int32_t ret;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GPS_IGNSSVER, 0,
                                          (FAR void **)&res, sizeof(struct apicmd_versionres_s))) {
    return VER_REQ_FAILURE;
  }

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, sizeof(struct apicmd_versionres_s),
                      &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen == 0) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  ret = (int32_t)res->result;

  DBGIF_LOG1_DEBUG("[gps-ver]ret: %ld\n", ret);
  strncpy(version, res->version, MAX_GPS_VERSION_DATALEN);

  if (ret != APICMD_VER_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(cmd, res);

  return VER_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return VER_REQ_FAILURE;
}

int altcom_gps_ver(char *version) {
  int result;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  result = gps_ver_req(version);

  if (result != VER_REQ_SUCCESS) {
    return APICMD_VER_RES_RET_CODE_ERR;
  }

  return APICMD_VER_RES_RET_CODE_OK;
}
