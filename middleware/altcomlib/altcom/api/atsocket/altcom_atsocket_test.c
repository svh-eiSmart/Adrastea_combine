/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

#include <string.h>

#include "dbg_if.h"
#include "util/apiutil.h"
#include "atsocket_types.h"
#include "apicmd_atsocket.h"
#include "apicmd_atsocket_test.h"

SocketError_e altcom_atsocket_test(int32_t arg) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return (SocketError_e)-1;
  }

  struct apicmd_atsocket_test_s *cmd = NULL;
  struct apicmd_atsocket_test_resp_s *resp = NULL;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_TEST, sizeof(*cmd),
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->arg = htonl(arg);

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return (SocketError_e)err;
}
