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

#ifndef __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MNOTIFY_H
#define __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MNOTIFY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_NOTIFY_RES_RET_CODE_OK (0)
#define APICMD_NOTIFY_RES_RET_CODE_ERR (-1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */
begin_packed_struct struct apicmd_lwm2mnotify_s {
  int8_t client;
  char token[17];
  char spare[2];  // for alignment
  unsigned char numOfUriData;
  int32_t server_id;            // optional may be used
  int32_t askForAckFromServer;  // optional may be used
  int32_t fragment_info;        // optional may be used
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2UriData_s {
  int32_t objectId;
  int32_t instanceNum;
  int32_t resourceNum;
  int resourceType;  // Multiple/Single
  int resourceInst;  // relevant ONLY if resourceType == Multiple
  char valType;
  resourceVal_t resourceVal;  // Must be the last
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2mnotifyres_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MNOTIFY_H */
