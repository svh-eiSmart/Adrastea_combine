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
#include "apicmd_gps_cep.h"

#define CEP_REQ_SUCCESS 0
#define CEP_REQ_FAILURE -1
#define CEP_DEFAULT_TIMEOUT 10

struct gps_cep_s {
  int cmd;
  int days;
  unsigned int timeout;
};

static int altcom_gps_cep_req(struct gps_cep_s *req, void *result) {
  FAR struct apicmd_cep_s *cep_req = NULL;
  FAR struct apicmd_cepres_s *res = NULL;
  uint16_t reslen = 0;
  int32_t ret;
  uint32_t timeout;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cep_req,
		  APICMDID_GPS_IGNSSCEP,
		  CEP_REQ_DATALEN,
          (FAR void **)&res,
		  CEP_RES_DATALEN)) {
    return CEP_REQ_FAILURE;
  }

  memset(cep_req, 0, sizeof(struct apicmd_cep_s));

  cep_req->cmd = htonl(req->cmd);
  cep_req->days = htonl(req->days);
  cep_req->timeout = htonl(req->timeout);

  timeout = (cep_req->days != 0 ? cep_req->days : CEP_DEFAULT_TIMEOUT) * 10000;

  ret = apicmdgw_send((FAR uint8_t *)cep_req, (FAR uint8_t *)res, CEP_RES_DATALEN, &reslen,
                      timeout);  // timeout according to CEP length

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != CEP_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  if (req->cmd == CEP_STATUS_CMD)
  {
	  cepStatusResult_t *status = (cepStatusResult_t *)result;

	  status->result = res->result;
	  status->days = ntohl(res->days);
	  status->hours = ntohl(res->hours);
	  status->minutes = ntohl(res->minutes);
	  ret = (int32_t)ntohl(res->ret_code);
  } else {
	  *((uint8_t *)result) = res->result;
	  ret = (int32_t)ntohl(res->ret_code);
  }
  DBGIF_LOG1_DEBUG("[cep-res]ret: %ld\n", ret);

  if (ret != APICMD_CEP_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(cep_req, res);

  return CEP_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cep_req, res);
  return CEP_REQ_FAILURE;
}

int altcom_gps_cep_download(cepDay_e days, unsigned int timeout, uint8_t *result) {
  struct gps_cep_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return CEP_REQ_FAILURE;
  }

  req.cmd = CEP_DLD_CMD;
  req.days = days;
  req.timeout = timeout;

  return altcom_gps_cep_req(&req, result);
}

int altcom_gps_cep_erase(uint8_t *result) {
  struct gps_cep_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return CEP_REQ_FAILURE;
  }

  req.cmd = CEP_ERASE_CMD;
  req.days = CEP_DAY_MAX;
  req.timeout = 0;

  return altcom_gps_cep_req(&req, result);
}

int altcom_gps_cep_status(cepStatusResult_t *result) {
  struct gps_cep_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not Initialized\n");
    return CEP_REQ_FAILURE;
  }

  req.cmd = CEP_STATUS_CMD;
  req.days = CEP_DAY_MAX;
  req.timeout = 0;

  return altcom_gps_cep_req(&req, result);
}
