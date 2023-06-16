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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDNINFO_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDNINFO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"
#include "lte/lte_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_GETPDNINFO_RES_OK (0)
#define APICMD_GETPDNINFO_RES_ERR (1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

/* APICMDID_GET_PDNINFO */

begin_packed_struct struct apicmd_cmddat_getpdninfo_s { uint32_t cid; } end_packed_struct;

begin_packed_struct struct pdninfo_s {
  uint8_t is_valid;
  uint8_t ip_type;
  uint32_t cid;
  uint32_t bearer_id;
  char apn_name[LTE_APN_LEN];
  char ip_nmask_str[LTE_IP_STRLEN * 2];
  char gw_str[LTE_IP_STRLEN];
  char pri_dns_str[LTE_IP_STRLEN];
  char sec_dns_str[LTE_IP_STRLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_getpdninfores_s {
  uint8_t result;
  struct pdninfo_s pdninfo[2];
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDNINFO_H */
