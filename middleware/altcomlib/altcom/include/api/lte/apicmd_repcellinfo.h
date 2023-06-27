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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPCELLINFO_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPCELLINFO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_SET_REPCELLINFO_DISABLE (0)
#define APICMD_SET_REPCELLINFO_ENABLE (1)
#define APICMD_SET_REPCELLINFO_INTERVAL_MIN (1)
#define APICMD_SET_REPCELLINFO_INTERVAL_MAX (4233600)

#define APICMD_SET_REPCELLINFO_RES_OK (0)
#define APICMD_SET_REPCELLINFO_RES_ERR (1)

#define APICMD_REP_CELLINFO_DISABLE (0)
#define APICMD_REP_CELLINFO_ENABLE (1)
#define APICMD_SET_REPCELLINFO_CELLID_MIN (0)
#define APICMD_SET_REPCELLINFO_CELLID_MAX (503)
#define APICMD_SET_REPCELLINFO_EARFCN_MIN (0)
#define APICMD_SET_REPCELLINFO_EARFCN_MAX (262143)
#define APICMD_SET_REPCELLINFO_DIGIT_NUM_MIN (0)
#define APICMD_SET_REPCELLINFO_DIGIT_NUM_MAX (9)
#define APICMD_SET_REPCELLINFO_MCC_DIGIT (3)
#define APICMD_SET_REPCELLINFO_MNC_DIGIT_MIN (2)
#define APICMD_SET_REPCELLINFO_MNC_DIGIT_MAX (3)
#define APICMD_SET_REPCELLINFO_CGID_DIGIT_MAX (16)

#define APICMD_SET_REPCELLINFO_TIMEDIFF_INDEX_MAX (4095)
#define APICMD_SET_REPCELLINFO_TA_MAX (1282)
#define APICMD_SET_REPCELLINFO_SFN_MAX (0x03FF)
#define APICMD_SET_REPCELLINFO_NEIGHBOR_CELL_NUM_MAX (32)
#define APICMD_SET_REPCELLINFO_VALID_TIMEDIFFIDX (1 << 1)
#define APICMD_SET_REPCELLINFO_VALID_TA (1 << 2)
#define APICMD_SET_REPCELLINFO_VALID_SFN (1 << 3)
#define APICMD_SET_REPCELLINFO_VALID_RSRP (1 << 4)
#define APICMD_SET_REPCELLINFO_VALID_RSRQ (1 << 5)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

begin_packed_struct struct apicmd_cmddat_setrepcellinfo_s {
  uint8_t enability;
  uint32_t interval;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_setrepcellinfo_res_s { uint8_t result; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_neighbor_cell_s {
  uint8_t valid;
  uint32_t cell_id;
  uint32_t earfcn;

  /* When setting "sfn",
   * APICMD_CELLINFO_VALID_SFN flag has been added to "valid".
   */

  uint16_t sfn;

  /* When setting "rsrp",
   * APICMD_CELLINFO_VALID_RSRP flag has been added to "valid".
   */

  int16_t rsrp;

  /* When setting "rsrq",
   * APICMD_CELLINFO_VALID_RSRQ flag has been added to "valid".
   */

  int16_t rsrq;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_repcellinfo_s {
  uint8_t enability;
  uint32_t cell_id;
  uint32_t earfcn;
  uint8_t mcc[APICMD_SET_REPCELLINFO_MCC_DIGIT];
  uint8_t mnc_digit;
  uint8_t mnc[APICMD_SET_REPCELLINFO_MNC_DIGIT_MAX];
  uint8_t cgid[APICMD_SET_REPCELLINFO_CGID_DIGIT_MAX + 1];
  uint16_t tac;

  /* When setting "time_diffidx",
   * APICMD_CELLINFO_VALID_TIMEDIFFIDX flag has been added to "valid".
   */

  uint16_t time_diffidx;

  /* When setting "ta",
   * APICMD_CELLINFO_VALID_TA flag has been added to "valid".
   */

  uint16_t ta;

  /* When setting "sfn",
   * APICMD_CELLINFO_VALID_SFN flag has been added to "valid".
   */

  uint16_t sfn;

  /* When setting "rsrp",
   * APICMD_CELLINFO_VALID_RSRP flag has been added to "valid".
   */

  int16_t rsrp;

  /* When setting "rsrq",
   * APICMD_CELLINFO_VALID_RSRQ flag has been added to "valid".
   */

  int16_t rsrq;

  uint8_t neighbor_num;
  struct apicmd_cmddat_neighbor_cell_s neighbor_cell[APICMD_SET_REPCELLINFO_NEIGHBOR_CELL_NUM_MAX];
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_REPCELLINFO_H */
