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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDPADDR_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDPADDR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"
#include "lte/lte_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_GETPDPADDR_RES_OK (0)
#define APICMD_GETPDPADDR_RES_ERR (1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

/* APICMDID_GET_PDPADDR */

begin_packed_struct struct apicmd_cmddat_getpdpaddr_s { uint32_t cid; } end_packed_struct;

begin_packed_struct struct pdpaddr_s {
  uint8_t is_valid;
  uint8_t ip_type;
  char ip_str[LTE_IP_STRLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_getpdpaddrres_s {
  uint8_t result;
  uint32_t cid;
  struct pdpaddr_s pdpaddr[2];
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETPDPADDR_H */
