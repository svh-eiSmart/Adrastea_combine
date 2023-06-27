/****************************************************************************
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
#ifndef __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MSERVERRESP_H
#define __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MSERVERRESP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_SERVER_RESP_RES_RET_CODE_OK (0)
#define APICMD_SERVER_RESP_RES_RET_CODE_ERR (-1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */
#define COAP_RESP_CODE_STR_LEN 4
begin_packed_struct struct apicmd_lwm2mserverresp_s {
  uint8_t client;
  int seqNum;
  char CoAPresponseCode[COAP_RESP_CODE_STR_LEN + 1];  // inc null term place
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2mserverrespres_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MSERVERRESP_H */
