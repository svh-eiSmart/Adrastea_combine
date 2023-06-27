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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETEDRX_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETEDRX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_GETEDRX_RES_OK (0)
#define APICMD_GETEDRX_RES_ERR (1)

#define APICMD_GETEDRX_DISABLE (0)
#define APICMD_GETEDRX_ENABLE (1)

#define APICMD_GETEDRX_CYC_512 (0)    /**< eDRX cycle     5.12 sec */
#define APICMD_GETEDRX_CYC_1024 (1)   /**< eDRX cycle    10.24 sec */
#define APICMD_GETEDRX_CYC_2048 (2)   /**< eDRX cycle    20.48 sec */
#define APICMD_GETEDRX_CYC_4096 (3)   /**< eDRX cycle    40.96 sec */
#define APICMD_GETEDRX_CYC_8192 (4)   /**< eDRX cycle    81.92 sec */
#define APICMD_GETEDRX_CYC_16384 (5)  /**< eDRX cycle   163.84 sec */
#define APICMD_GETEDRX_CYC_32768 (6)  /**< eDRX cycle   327.68 sec */
#define APICMD_GETEDRX_CYC_65536 (7)  /**< eDRX cycle   655.36 sec */
#define APICMD_GETEDRX_CYC_131072 (8) /**< eDRX cycle  1310.72 sec */
#define APICMD_GETEDRX_CYC_262144 (9) /**< eDRX cycle  2621.44 sec */

#define APICMD_GETEDRX_PTW_128 (0)   /**< Paging time window  1.28 sec */
#define APICMD_GETEDRX_PTW_256 (1)   /**< Paging time window  2.56 sec */
#define APICMD_GETEDRX_PTW_384 (2)   /**< Paging time window  3.84 sec */
#define APICMD_GETEDRX_PTW_512 (3)   /**< Paging time window  5.12 sec */
#define APICMD_GETEDRX_PTW_640 (4)   /**< Paging time window  6.40 sec */
#define APICMD_GETEDRX_PTW_768 (5)   /**< Paging time window  7.68 sec */
#define APICMD_GETEDRX_PTW_896 (6)   /**< Paging time window  8.96 sec */
#define APICMD_GETEDRX_PTW_1024 (7)  /**< Paging time window 10.24 sec */
#define APICMD_GETEDRX_PTW_1152 (8)  /**< Paging time window 11.52 sec */
#define APICMD_GETEDRX_PTW_1280 (9)  /**< Paging time window 12.80 sec */
#define APICMD_GETEDRX_PTW_1408 (10) /**< Paging time window 14.08 sec */
#define APICMD_GETEDRX_PTW_1536 (11) /**< Paging time window 15.36 sec */
#define APICMD_GETEDRX_PTW_1664 (12) /**< Paging time window 16.64 sec */
#define APICMD_GETEDRX_PTW_1792 (13) /**< Paging time window 17.92 sec */
#define APICMD_GETEDRX_PTW_1920 (14) /**< Paging time window 19.20 sec */
#define APICMD_GETEDRX_PTW_2048 (15) /**< Paging time window 20.48 sec */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

/* APICMDID_GET_EDRX */

begin_packed_struct struct apicmd_cmddat_getedrx_s { uint8_t cfgType; } end_packed_struct;

/* APICMDID_GET_EDRX_RES */

begin_packed_struct struct apicmd_cmddat_getedrxres_s {
  uint8_t result;
  uint8_t acttype;
  uint8_t enable;
  uint8_t edrx_cycle;
  uint8_t ptw_val;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GETEDRX_H */
