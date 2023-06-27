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
#ifndef __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2M_GET_SERVERINFO_H
#define __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2M_GET_SERVERINFO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "apicmd.h"
#include "lwm2m/altcom_lwm2m.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_SERVERINFO_RES_RET_CODE_OK (0)
#define APICMD_SERVERINFO_RES_RET_CODE_ERR (-1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

begin_packed_struct struct apicmd_lwm2mserverinfo_s { int8_t client; } end_packed_struct;

begin_packed_struct struct apicmd_lwm2mserver_s {
  uint8_t client;
  char server_uri[MAX_LWM2M_SERVER_URI];
  uint32_t server_id;
  int32_t liftime;
  uint8_t binding;
  uint8_t server_stat;
  int64_t last_regdate;
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2mserverinfores_s {
  int8_t ret_code;
  uint8_t server_cnt;
  struct apicmd_lwm2mserver_s info[MAX_LWM2M_SERVER_NUM];
} end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2M_GET_SERVERINFO_H */
