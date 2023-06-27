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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETAPNLIST_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETAPNLIST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_GETAPNLIST_RES_OK (0)
#define APICMD_GETAPNLIST_RES_ERR (1)

#define APICMD_GETAPNLIST_APN_LEN 101

#define APICMD_GETAPNLIST_IPTYPE_IP (0)
#define APICMD_GETAPNLIST_IPTYPE_IPV6 (1)
#define APICMD_GETAPNLIST_IPTYPE_IPV4V6 (2)

#define APICMD_GETAPNLIST_RES_LIST_MAX_NUM (5)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

/* APICMDID_GET_APNLIST
 * no data
 */

begin_packed_struct struct apicmd_cmddat_getapndata_s {
  uint8_t session_id;
  uint8_t apn[APICMD_GETAPNLIST_APN_LEN];
  uint8_t ip_type;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_getapnlistres_s {
  uint8_t result;
  uint8_t listnum;
  struct apicmd_cmddat_getapndata_s apnlist[APICMD_GETAPNLIST_RES_LIST_MAX_NUM];
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETAPNLIST_H */
