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

#ifndef __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MBOOTSTARP_H
#define __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MBOOTSTARP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_BOOTSTARP_RES_RET_CODE_OK (0)
#define APICMD_BOOTSTARP_RES_RET_CODE_ERR (-1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */
begin_packed_struct struct apicmd_lwm2BootstrapCmdStart_s {
  uint8_t client;
  unsigned char factoryBsNameLen;
  char factoryBsName;  // 1st char of factory name
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdStartres_s {
  int32_t ret_code;
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdDelete_s { uint8_t client; } end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdDeleteres_s {
  int32_t ret_code;
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdDone_s { uint8_t client; } end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdDoneres_s { int32_t ret_code; } end_packed_struct;

// lwm2m_resource_and_data_t
begin_packed_struct struct apicmd_lwm2BootstrapCmdCreat_s {
  uint8_t client;
  int32_t objectId;
  int32_t instanceNum;
  unsigned char numOfResrcNumData;
  char ResrcNumDataStart;
} end_packed_struct;

begin_packed_struct struct apicmd_lwm2BootstrapCmdCreatres_s {
  int32_t ret_code;
} end_packed_struct;
#endif /* __MODULES_ALTCOM_INCLUDE_API_LWM2M_APICMD_LWM2MBOOTSTARP_H */
