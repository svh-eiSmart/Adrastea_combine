/****************************************************************************
 * modules/lte/altcom/api/gps/altcom_gps_memerase.c
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
#include "apicmd_gps_memerase.h"

#define MEMERASE_REQ_SUCCESS 0
#define MEMERASE_REQ_FAILURE -1

struct gps_memerase_s {
  int bitmap;
};

static int altcom_gps_memerase_req(struct gps_memerase_s *req) {
  FAR struct apicmd_memerase_s *cmd = NULL;
  FAR struct apicmd_memeraseres_s *res = NULL;
  uint16_t reslen = 0;
  int32_t ret;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GPS_MEMERASE,
                                          sizeof(struct apicmd_memerase_s), (FAR void **)&res,
                                          sizeof(struct apicmd_memeraseres_s))) {
    return MEMERASE_REQ_FAILURE;
  }

  memset(cmd, 0, sizeof(struct apicmd_memerase_s));

  cmd->bitmap = htonl(req->bitmap);

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, sizeof(struct apicmd_memeraseres_s),
                      &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != sizeof(struct apicmd_memeraseres_s)) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  ret = ntohl(res->result);

  DBGIF_LOG1_DEBUG("[memerase-res]ret: %ld\n", ret);

  if (ret != APICMD_MEMERASE_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(cmd, res);

  return MEMERASE_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return MEMERASE_REQ_FAILURE;
}

int altcom_gps_memerase(int bitmap) {
  struct gps_memerase_s memerase;
  int result;

  printf("altcom_gps_memerase");

  memerase.bitmap = bitmap;

  result = altcom_gps_memerase_req(&memerase);

  if (result != MEMERASE_REQ_SUCCESS) {
    return APICMD_MEMERASE_RES_RET_CODE_ERR;
  }

  return APICMD_MEMERASE_RES_RET_CODE_OK;
}
