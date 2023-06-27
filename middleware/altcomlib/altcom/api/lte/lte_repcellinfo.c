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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "lte/lte_api.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_repcellinfo.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CELLINFO_DATA_LEN (sizeof(struct apicmd_cmddat_setrepcellinfo_s))
#define CELLINFO_SETRES_DATA_LEN (sizeof(struct apicmd_cmddat_setrepcellinfo_res_s))
#define CELLINFO_PERIOD_MIN (1)
#define CELLINFO_PERIOD_MAX (4233600)
#define GET_CELLINFO_DATA_LEN (0)
#define GET_CELLINFO_RESP_LEN (sizeof(struct apicmd_cmddat_repcellinfo_s))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_lte_setrepcellinfo_isproc = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern cellinfo_report_cb_t g_cellinfo_callback;
extern void *g_cellinfo_cbpriv;

/****************************************************************************
 * External Functions
 ****************************************************************************/
extern bool check_arrydigitnum(FAR uint8_t number[], uint8_t digit);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_set_report_cellinfo() Change the report setting of the cell
 * information. The default report setting is disable.
 *
 * @param [in] cellinfo_callback: Callback function to notify that
 * cell information. If NULL is set, the report setting is disabled.
 * @param [in] period: Reporting cycle in sec (1-4233600)
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_cellinfo(cellinfo_report_cb_t cellinfo_callback, uint32_t period,
                                void *userPriv) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_setrepcellinfo_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_setrepcellinfo_res_s *resbuff = NULL;
  uint16_t resbufflen = CELLINFO_SETRES_DATA_LEN;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EPERM;
  }

  /* Check if callback registering only */

  if (altcom_isCbRegOnly()) {
    ALTCOM_CLR_CALLBACK(g_cellinfo_callback, g_cellinfo_cbpriv);
    if (cellinfo_callback) {
      ALTCOM_REG_CALLBACK(ret, g_cellinfo_callback, cellinfo_callback, g_cellinfo_cbpriv, userPriv);
    }

    return ret;
  }

  if (cellinfo_callback) {
    if (CELLINFO_PERIOD_MIN > period || CELLINFO_PERIOD_MAX < period) {
      DBGIF_LOG_ERROR("Invalid parameter.\n");
      return -EINVAL;
    }
  }

  /* Check this process runnning. */

  if (g_lte_setrepcellinfo_isproc) {
    return -EBUSY;
  }

  g_lte_setrepcellinfo_isproc = true;

  /* Accept the API */
  /* Allocate API command buffer to send */

  cmdbuff = (FAR struct apicmd_cmddat_setrepcellinfo_s *)apicmdgw_cmd_allocbuff(
      APICMDID_SET_REP_CELLINFO, CELLINFO_DATA_LEN);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    g_lte_setrepcellinfo_isproc = false;
    return -ENOMEM;
  } else {
    resbuff = (FAR struct apicmd_cmddat_setrepcellinfo_res_s *)BUFFPOOL_ALLOC(resbufflen);
    if (!resbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      altcom_free_cmd((FAR uint8_t *)cmdbuff);
      g_lte_setrepcellinfo_isproc = false;
      return -ENOMEM;
    }

    /* Set event field */

    cmdbuff->enability =
        !cellinfo_callback ? APICMD_SET_REPCELLINFO_DISABLE : APICMD_SET_REPCELLINFO_ENABLE;
    cmdbuff->interval = htonl(period);

    ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, resbufflen, &reslen,
                        ALTCOM_SYS_TIMEO_FEVR);
  }

  if (0 <= ret && resbufflen == reslen) {
    if (APICMD_SET_REPCELLINFO_RES_OK == resbuff->result) {
      /* Register API callback */

      ALTCOM_CLR_CALLBACK(g_cellinfo_callback, g_cellinfo_cbpriv);
      if (cellinfo_callback) {
        ALTCOM_REG_CALLBACK(ret, g_cellinfo_callback, cellinfo_callback, g_cellinfo_cbpriv,
                            userPriv);
      }
    } else {
      DBGIF_LOG_ERROR("API command response is err.\n");
      ret = -EIO;
    }
  }

  if (0 <= ret) {
    ret = 0;
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  (void)BUFFPOOL_FREE(resbuff);
  g_lte_setrepcellinfo_isproc = false;

  return ret;
}

/**
 * @brief lte_get_cellinfo() get the current cell information.
 *
 * @param [inout] cellinfo: Cell information.
 * See @ref lte_cellinfo_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_cellinfo(lte_cellinfo_t *cellinfo) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR void *cmd;
  FAR struct apicmd_cmddat_repcellinfo_s *res;

  /* Return error if callback is NULL */
  if (!cellinfo) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LTE_RESULT_ERROR;
  }

  /* Allocate API command buffer to send */
  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GET_CELLINFO,
                                         GET_CELLINFO_DATA_LEN, (FAR void **)&res,
                                         GET_CELLINFO_RESP_LEN)) {
    /* Send API command to modem */
    ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, GET_CELLINFO_RESP_LEN, &resLen,
                        ALTCOM_SYS_TIMEO_FEVR);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return LTE_RESULT_ERROR;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  if (GET_CELLINFO_RESP_LEN != resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = LTE_RESULT_ERROR;
    goto errout_with_cmdfree;
  }

  /* Check API return code */
  cellinfo->valid = (ltevalidflag_t)res->enability;
  if (cellinfo->valid) {
    cellinfo->phycell_id = ntohl(res->cell_id);
    cellinfo->earfcn = ntohl(res->earfcn);
    memcpy(cellinfo->mcc, res->mcc, APICMD_SET_REPCELLINFO_MCC_DIGIT);
    cellinfo->mnc_digit = res->mnc_digit;
    memcpy(cellinfo->mnc, res->mnc, res->mnc_digit);
    memcpy(cellinfo->cgid, res->cgid, APICMD_SET_REPCELLINFO_CGID_DIGIT_MAX + 1);
    cellinfo->tac = ntohs(res->tac);
    if (APICMD_SET_REPCELLINFO_CELLID_MAX < cellinfo->phycell_id) {
      DBGIF_LOG1_ERROR("cellinfo->phycell_id error:%lu\n", cellinfo->phycell_id);
      cellinfo->valid = LTE_INVALID;
    } else if (APICMD_SET_REPCELLINFO_EARFCN_MAX < cellinfo->earfcn) {
      DBGIF_LOG1_ERROR("cellinfo->earfcn error:%lu\n", cellinfo->earfcn);
      cellinfo->valid = LTE_INVALID;
    } else if (!check_arrydigitnum(cellinfo->mcc, APICMD_SET_REPCELLINFO_MCC_DIGIT)) {
      DBGIF_LOG_ERROR("cellinfo->mcc error\n");
      cellinfo->valid = LTE_INVALID;
    } else if (cellinfo->mnc_digit < APICMD_SET_REPCELLINFO_MNC_DIGIT_MIN ||
               APICMD_SET_REPCELLINFO_MNC_DIGIT_MAX < cellinfo->mnc_digit) {
      DBGIF_LOG1_ERROR("cellinfo->mnc_digit error:%d\n", cellinfo->mnc_digit);
      cellinfo->valid = LTE_INVALID;
    } else if (!check_arrydigitnum(cellinfo->mnc, cellinfo->mnc_digit)) {
      DBGIF_LOG_ERROR("cellinfo->mnc error\n");
      cellinfo->valid = LTE_INVALID;
    }

    cellinfo->time_diffidx = ntohs(res->time_diffidx);
    cellinfo->ta = ntohs(res->ta);
    cellinfo->sfn = ntohs(res->sfn);
    cellinfo->rsrp = ntohs(res->rsrp);
    cellinfo->rsrq = ntohs(res->rsrq);
    cellinfo->neighbor_num = res->neighbor_num;

    int i;

    for (i = 0; i < cellinfo->neighbor_num; i++) {
      cellinfo->neighbor_cell[i].valid = (ltevalidflag_t)res->neighbor_cell[i].valid;
      cellinfo->neighbor_cell[i].cell_id = ntohl(res->neighbor_cell[i].cell_id);
      cellinfo->neighbor_cell[i].earfcn = ntohl(res->neighbor_cell[i].earfcn);
      cellinfo->neighbor_cell[i].sfn = ntohs(res->neighbor_cell[i].sfn);
      cellinfo->neighbor_cell[i].rsrp = ntohs(res->neighbor_cell[i].rsrp);
      cellinfo->neighbor_cell[i].rsrq = ntohs(res->neighbor_cell[i].rsrq);
    }
  }

  ret = LTE_RESULT_OK;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (lteresult_e)ret;
}