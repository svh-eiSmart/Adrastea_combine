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
#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_SCANSCHEME_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_SCANSCHEME_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_SCANSCHEME_RES_OK (0)
#define APICMD_SCANSCHEME_RES_ERR (1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

begin_packed_struct struct apicmd_cmddat_scanscheme_s {
  uint8_t set1_get2;
  uint16_t num_mru_scans;
  uint16_t num_country_scans;
  uint16_t num_region_scans;
  uint8_t num_full_scans;
  uint8_t fallback_full_to_country;
  uint32_t slptime_btwn_scans;
  uint32_t max_slptime_btwn_scans;
  int16_t slptime_step;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_scanschemeres_s {
  uint8_t result;
  uint16_t num_mru_scans;
  uint16_t num_country_scans;
  uint16_t num_region_scans;
  uint8_t num_full_scans;
  uint8_t fallback_full_to_country;
  uint32_t slptime_btwn_scans;
  uint32_t max_slptime_btwn_scans;
  int16_t slptime_step;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_SCANSCHEME_H */
