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

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "altcom.h"
#include "lte/lte_api.h"

/* App includes */
#include "apitest_main.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APITEST_CMD_ARG (1)
#define APITEST_CMD_PARAM_1 (2)
#define APITEST_CMD_PARAM_2 (3)
#define APITEST_CMD_PARAM_3 (4)
#define APITEST_CMD_PARAM_4 (5)
#define APITEST_CMD_PARAM_5 (6)
#define APITEST_CMD_PARAM_6 (7)
#define APITEST_CMD_PARAM_7 (8)
#define APITEST_CMD_PARAM_8 (9)
#define APITEST_CMD_PARAM_9 (10)
#define APITEST_CMD_PARAM_10 (11)
#define APITEST_CMD_PARAM_11 (12)
#define APITEST_CMD_PARAM_12 (13)
#define APITEST_CMD_PARAM_13 (14)

#define APITEST_MAX_NUMBER_OF_PARAM (15)
#define APITEST_MAX_API_MQUEUE (16)

#define APITEST_TASK_STACKSIZE (3072)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_PINCODE_LEN (9)
#define APITEST_INVALID_VALUE (99)
#define APITEST_STRTOL_BASE (10)
#define APITEST_INVALID_ARG ("INVALID")
#define APITEST_NULL_ARG ("NULL")
#define APITEST_ENABLE_ARG ("ENABLE")
#define APITEST_DISABLE_ARG ("DISABLE")
#define APITEST_REQUESTED_ARG ("REQ")
#define APITEST_NWPROVIDED_ARG ("NW")

#define APITEST_GETFUNCNAME(func) (#func)

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
    printf(__VA_ARGS__);       \
    UNLOCK();                  \
  } while (0)

#define APITEST_PRINT_RESULT(result) \
  do {                               \
    show_result(result, __func__);   \
  } while (0)

#define APITEST_PRINT_ERROR(result, errcause) \
  do {                                        \
    show_error(result, errcause, __func__);   \
  } while (0)

#define APITEST_FREE_CMD(cmd, argc)    \
  do {                                 \
    int32_t idx;                       \
    for (idx = 0; idx < argc; idx++) { \
      free(cmd->argv[idx]);            \
      cmd->argv[idx] = NULL;           \
    }                                  \
                                       \
    free(cmd);                         \
  } while (0);

/****************************************************************************
 * Private Data Type
 ****************************************************************************/

struct apitest_command_s {
  int argc;
  char *argv[APITEST_MAX_NUMBER_OF_PARAM];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void show_help(char *cmd) {
  if (strncmp(cmd, "strat", strlen("strat")) == 0) {
    printf("usage: strat <rat> <persistency>\n");
    printf("<rat>\n");
    printf("\t1:default\n");
    printf("\t2:CATM\n");
    printf("\t3:NBIOT\n");
    printf("\t4:GSM\n");
    printf("\t5:C2D\n");
    printf("\t6:N2D\n");
    printf("\t7:G2D\n");
    printf("<persistency>\n");
    printf("\t0:Not persistent\n");
    printf("\t1:Persistent\n");
  } else if (strncmp(cmd, "stintrcptscn", strlen("stintrcptscn")) == 0) {
    printf("%s\n", "usage: stintrcptscn <mode> <timer value>\n");
    printf("%s\n", "<mode>");
    printf("%s\n", "\t0:diable");
    printf("%s\n", "\t1:enable");
    printf("%s\n", "<timer value>");
    printf("%s\n", "\ttime in ms");
  } else if (strncmp(cmd, "pdcptmr", strlen("pdcptmr")) == 0) {
    printf("%s\n", "usage: stintrcptscn <timer value>\n");
    printf("%s\n", "<timer value>");
    printf("%s\n", "\t0:network default");
    printf("%s\n", "\t1:infinity");
    printf("%s\n", "\t2:50 ms");
    printf("%s\n", "\t3:100 ms");
    printf("%s\n", "\t4:150 ms");
    printf("%s\n", "\t5:300 ms");
    printf("%s\n", "\t6:500 ms");
    printf("%s\n", "\t7:750 ms");
    printf("%s\n", "\t8:1500 ms");
  }
}
static void show_result(uint32_t result, const char *funcname) {
  if (LTE_RESULT_OK == result) {
    APITEST_DBG_PRINT("[CB_OK] %s result : \"%lu\"\n", funcname, result);
  } else {
    APITEST_DBG_PRINT("[CB_NG] %s result : \"%lu\"\n", funcname, result);
  }
}

static void show_error(uint32_t result, uint32_t errcause, const char *funcname) {
  if (LTE_RESULT_ERROR == result) {
    APITEST_DBG_PRINT("[CB_NG] %s error : \"%lu\"\n", funcname, errcause);
  }
}

static void atchnet_cb(lteresult_e result, lteerrcause_e errcause, void *userPriv) {
  APITEST_PRINT_RESULT(result);
  APITEST_PRINT_ERROR(result, errcause);
  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void dtchnet_cb(lteresult_e result, void *userPriv) {
  APITEST_PRINT_RESULT(result);
  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void daton_cb(lteresult_e result, lteerrcause_e errcause, void *userPriv) {
  APITEST_PRINT_RESULT(result);
  APITEST_PRINT_ERROR(result, errcause);
  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void datoff_cb(lteresult_e result, lteerrcause_e errcause, void *userPriv) {
  APITEST_PRINT_RESULT(result);
  APITEST_PRINT_ERROR(result, errcause);
  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void ntstrepo_cb(uint8_t sessionid, ltenetstate_e netstat, void *userPriv) {
  if (LTE_NETSTAT_DETACH >= netstat) {
    APITEST_DBG_PRINT("[CB_VAL] netstat : \"%lu\", priv: %p\n", (uint32_t)netstat, userPriv);
  } else {
    APITEST_DBG_PRINT("[CB_VAL] session_id: %lu, datastate: %lu, priv: %p\n", (uint32_t)sessionid,
                      (uint32_t)netstat, userPriv);
  }
}

static void timerevt_cb(lte_timerevt_evttype_t ev_type, lte_timerevt_evsel_t status,
                        void *userPriv) {
  char evtype[10] = {0};
  char st[10] = {0};

  if (ev_type == LTE_EVTYPE_3412)
    strncpy(evtype, "T3412", sizeof(evtype) - 1);
  else if (ev_type == LTE_EVTYPE_3402)
    strncpy(evtype, "T3402", sizeof(evtype) - 1);
  else
    strncpy(evtype, "Unknown", sizeof(evtype) - 1);

  if (status == LTE_EVSEL_STOP)
    strncpy(st, "Stop", sizeof(st) - 1);
  else if (status == LTE_EVSEL_START)
    strncpy(st, "Start", sizeof(st) - 1);
  else if (status == LTE_EVSEL_EXPIRED)
    strncpy(st, "Expired", sizeof(st) - 1);
  else
    strncpy(st, "Unknown", sizeof(st) - 1);

  APITEST_DBG_PRINT("[CB_VAL] timerevt : Received %s with %s event  priv: %p\n", evtype, st,
                    userPriv);
}

static void smstrepo_cb(ltesimstate_e simstat, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] simstat : \"%lu\", priv: %p\n", (uint32_t)simstat, userPriv);
}

static void lcltmrepo_cb(lte_localtime_t *localtime, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] time      : \"%02ld/%02ld/%02ld : %02ld:%02ld:%02ld\"\n",
                    localtime->year, localtime->mon, localtime->mday, localtime->hour,
                    localtime->min, localtime->sec);
  APITEST_DBG_PRINT("[CB_VAL] time_zone : \"%ld\", priv: %p\n", localtime->tz_sec, userPriv);
}

static void qtyrepo_cb(lte_quality_t *quality, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] valid : \"%d\"\n", quality->valid);

  if (LTE_VALID == quality->valid) {
    APITEST_DBG_PRINT("[CB_VAL] rsrp  : \"%hd\"\n", quality->rsrp);
    APITEST_DBG_PRINT("[CB_VAL] rsrq  : \"%hd\"\n", quality->rsrq);
    APITEST_DBG_PRINT("[CB_VAL] sinr  : \"%hd\"\n", quality->sinr);
    APITEST_DBG_PRINT("[CB_VAL] rssi  : \"%hd\"\n", quality->rssi);
  }

  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void qtyrepo_2g_cb(lte_quality_2g_t *quality, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] valid : \"%d\"\n", quality->valid);

  if (LTE_VALID == quality->valid) {
    APITEST_DBG_PRINT("[CB_VAL] rssi  : \"%hd\"\n", quality->rssi);
    APITEST_DBG_PRINT("[CB_VAL] ber  : \"%hd\"\n", quality->ber);
  }

  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void clinfrepo_cb(lte_cellinfo_t *cellinfo, void *userPriv) {
  int i = 0;

  APITEST_DBG_PRINT("[CB_VAL] valid      : \"%d\"\n", cellinfo->valid);

  if (LTE_VALID & cellinfo->valid) {
    APITEST_DBG_PRINT("[CB_VAL] phycell_id : \"%lu\"\n", cellinfo->phycell_id);
    APITEST_DBG_PRINT("[CB_VAL] earfcn     : \"%lu\"\n", cellinfo->earfcn);

    for (i = 0; i < LTE_CELLINFO_MCC_DIGIT; i++) {
      APITEST_DBG_PRINT("[CB_VAL] mcc[%d]    : \"%d\"\n", i, cellinfo->mcc[i]);
    }

    APITEST_DBG_PRINT("[CB_VAL] mnc_digit  : \"%d\"\n", cellinfo->mnc_digit);

    for (i = 0; i < cellinfo->mnc_digit; i++) {
      APITEST_DBG_PRINT("[CB_VAL] mnc[%d]    : \"%d\"\n", i, cellinfo->mnc[i]);
    }

    APITEST_DBG_PRINT("[CB_VAL] cgid    : \"%s\"\n", cellinfo->cgid);
    APITEST_DBG_PRINT("[CB_VAL] tac    : \"%04X\"\n", (unsigned int)cellinfo->tac);
    if (cellinfo->valid & LTE_TIMEDIFFIDX_VALID) {
      APITEST_DBG_PRINT("[CB_VAL] time_diffidx    : \"%hu\"\n", cellinfo->time_diffidx);
    }

    if (cellinfo->valid & LTE_TIMEDIFFIDX_VALID) {
      APITEST_DBG_PRINT("[CB_VAL] ta    : \"%hu\"\n", cellinfo->ta);
    }

    if (cellinfo->valid & LTE_TA_VALID) {
      APITEST_DBG_PRINT("[CB_VAL] SFN    : \"%hu\"\n", cellinfo->sfn);
    }

    if (cellinfo->valid & LTE_RSRP_VALID) {
      APITEST_DBG_PRINT("[CB_VAL] rsrp    : \"%hd\"\n", cellinfo->rsrp);
    }

    if (cellinfo->valid & LTE_RSRQ_VALID) {
      APITEST_DBG_PRINT("[CB_VAL] rsrq    : \"%hd\"\n", cellinfo->rsrq);
    }

    APITEST_DBG_PRINT("[CB_VAL] neighbor_num    : \"%lu\"\n", (uint32_t)cellinfo->neighbor_num);
    for (i = 0; i < cellinfo->neighbor_num; i++) {
      APITEST_DBG_PRINT("[CB_VAL] phycell_id : \"%lu\"\n", cellinfo->neighbor_cell[i].cell_id);
      APITEST_DBG_PRINT("[CB_VAL] earfcn     : \"%lu\"\n", cellinfo->neighbor_cell[i].earfcn);
      if (cellinfo->neighbor_cell[i].valid & LTE_SFN_VALID) {
        APITEST_DBG_PRINT("[CB_VAL] sfn[%d]    : \"%hu\"\n", i, cellinfo->neighbor_cell[i].sfn);
      }

      if (cellinfo->neighbor_cell[i].valid & LTE_RSRP_VALID) {
        APITEST_DBG_PRINT("[CB_VAL] rsrp[%d]    : \"%hd\"\n", i, cellinfo->neighbor_cell[i].rsrp);
      }

      if (cellinfo->neighbor_cell[i].valid & LTE_RSRQ_VALID) {
        APITEST_DBG_PRINT("[CB_VAL] rsrq[%d]    : \"%hd\"\n", i, cellinfo->neighbor_cell[i].rsrq);
      }
    }
  }

  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void clinfrepo_2g_cb(lte_cellinfo_2g_t *cellinfo, void *userPriv) {
  int i, j;

  APITEST_DBG_PRINT("[CB_VAL] valid      : \"%d\"\n", cellinfo->serv_cell.valid);

  if (LTE_VALID & cellinfo->serv_cell.valid) {
    for (i = 0; i < LTE_CELLINFO_MCC_DIGIT; i++) {
      APITEST_DBG_PRINT("[CB_VAL] mcc[%d]    : \"%d\"\n", i, cellinfo->serv_cell.mcc[i]);
    }

    APITEST_DBG_PRINT("[CB_VAL] mnc_digit  : \"%d\"\n", cellinfo->serv_cell.mnc_digit);

    for (i = 0; i < cellinfo->serv_cell.mnc_digit; i++) {
      APITEST_DBG_PRINT("[CB_VAL] mnc[%d]    : \"%d\"\n", i, cellinfo->serv_cell.mnc[i]);
    }

    APITEST_DBG_PRINT("[CB_VAL] lac : \"%lu\"\n", cellinfo->serv_cell.lac);
    APITEST_DBG_PRINT("[CB_VAL] cell_id : \"%lu\"\n", cellinfo->serv_cell.cell_id);
    APITEST_DBG_PRINT("[CB_VAL] bs_id : \"%lu\"\n", cellinfo->serv_cell.cell_id);
    APITEST_DBG_PRINT("[CB_VAL] arfcn     : \"%lu\"\n", cellinfo->serv_cell.arfcn);
    APITEST_DBG_PRINT("[CB_VAL] rx_level     : \"%lu\"\n", cellinfo->serv_cell.rx_level);
    APITEST_DBG_PRINT("[CB_VAL] t_adv     : \"%hu\"\n", cellinfo->serv_cell.t_adv);

    APITEST_DBG_PRINT("[CB_VAL] neighbor_num    : \"%lu\"\n", (uint32_t)cellinfo->neighbor_num);
    for (i = 0; i < cellinfo->neighbor_num; i++) {
      if (cellinfo->neighbor_cell[i].valid) {
        for (j = 0; j < LTE_CELLINFO_MCC_DIGIT; j++) {
          APITEST_DBG_PRINT("[CB_VAL] mcc[%d]    : \"%d\"\n", j, cellinfo->neighbor_cell[i].mcc[j]);
        }

        APITEST_DBG_PRINT("[CB_VAL] mnc_digit  : \"%d\"\n", cellinfo->neighbor_cell[i].mnc_digit);

        for (j = 0; j < cellinfo->neighbor_cell[i].mnc_digit; j++) {
          APITEST_DBG_PRINT("[CB_VAL] mnc[%d]    : \"%d\"\n", j, cellinfo->neighbor_cell[i].mnc[j]);
        }

        APITEST_DBG_PRINT("[CB_VAL] lac : \"%lu\"\n", cellinfo->neighbor_cell[i].lac);
        APITEST_DBG_PRINT("[CB_VAL] cell_id : \"%lu\"\n", cellinfo->neighbor_cell[i].cell_id);
        APITEST_DBG_PRINT("[CB_VAL] bs_id : \"%lu\"\n", cellinfo->neighbor_cell[i].bs_id);
        APITEST_DBG_PRINT("[CB_VAL] arfcn     : \"%lu\"\n", cellinfo->neighbor_cell[i].arfcn);
        APITEST_DBG_PRINT("[CB_VAL] rx_level     : \"%lu\"\n", cellinfo->neighbor_cell[i].rx_level);
      }
    }
  }

  APITEST_DBG_PRINT("[PRIV] : %p", userPriv);
}

static void regstate_cb(lteregstate_e regstate, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] regstate : \"%lu\", priv: %p\n", (uint32_t)regstate, userPriv);
}

static void psmstate_cb(ltepsmstate_e psmstate, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] psmstate : \"%lu\", priv: %p\n", (uint32_t)psmstate, userPriv);
}

static void dynpsm_cb(lte_psm_setting_t *psm_settings, void *userPriv) {
  if (LTE_PSM_DISABLE == psm_settings->func) {
    APITEST_DBG_PRINT("[CB_VAL] psm disabled, priv: %p\n", userPriv);
  } else {
    APITEST_DBG_PRINT("[CB_VAL] Active Time, unit: %lu, time_val: %lu, priv: %p\n",
                      (uint32_t)psm_settings->active_time.unit,
                      (uint32_t)psm_settings->active_time.time_val, userPriv);
    APITEST_DBG_PRINT("[CB_VAL] Extended periodic TAU, unit: %lu, time_val: %lu, priv: %p\n",
                      (uint32_t)psm_settings->ext_periodic_tau_time.unit,
                      (uint32_t)psm_settings->ext_periodic_tau_time.time_val, userPriv);
  }
}

static void dynedrx_cb(lte_edrx_setting_t *edrx_settings, void *userPriv) {
  if (LTE_DISABLE == edrx_settings->enable) {
    APITEST_DBG_PRINT("[CB_VAL] edrx disabled, priv: %p\n", userPriv);
  } else {
    APITEST_DBG_PRINT("[CB_VAL] act_type: %lu, edrx_cycle: %lu, ptw_val: %lu, priv: %p\n",
                      (uint32_t)edrx_settings->act_type, (uint32_t)edrx_settings->edrx_cycle,
                      (uint32_t)edrx_settings->ptw_val, userPriv);
  }
}

static void connphase_cb(lteconphase_e conphase, lteconphase_rat_e rat,
                         lteconfphase_scan_type_e scan_type, lteconfphase_scan_reason_e scan_reason,
                         void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] connphase : %lu\n", (uint32_t)conphase);
  APITEST_DBG_PRINT("[CB_VAL] rat : %lu\n", (uint32_t)rat);
  APITEST_DBG_PRINT("[CB_VAL] scan_type : %lu\n", (uint32_t)scan_type);
  APITEST_DBG_PRINT("[CB_VAL] scan_reason : %lu\n", (uint32_t)scan_reason);
  APITEST_DBG_PRINT("[CB_VAL] priv: %p\n", userPriv);
}

static void antitamperEvt_cb(uint8_t data, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] ANTITAMPER : priv: %p\n", userPriv);
}

static void scanresult_report_cb(lte_scan_result_t *scanresult, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] scan result: %lu, priv: %p\n", (uint32_t)scanresult->resultcode,
                    userPriv);
  if (scanresult->resultcode == LTE_SCAN_RESULTCODE_FAILURE) {
    APITEST_DBG_PRINT("[CB_VAL] last scan info: %lu\n", (uint32_t)scanresult->info);
  }
}

static void lwm2m_fw_upgrade_evt_report_cb(lte_lwm2m_fw_upgrade_evt_t *lwm2m_fw_upgrade_evt,
                                           void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] lwm2m fw upgrade event: %lu, priv: %p\n",
                    (uint32_t)lwm2m_fw_upgrade_evt->eventtype, userPriv);
  APITEST_DBG_PRINT("[CB_VAL] lwm2m fw upgrade event package name: %s\n",
                    lwm2m_fw_upgrade_evt->package_name);
  APITEST_DBG_PRINT("[CB_VAL] lwm2m fw upgrade event package size: %lu\n",
                    (uint32_t)lwm2m_fw_upgrade_evt->package_size);
  APITEST_DBG_PRINT("[CB_VAL] lwm2m fw upgrade event error type: %lu\n",
                    (uint32_t)lwm2m_fw_upgrade_evt->error_type);
}

static void pdn_actevt_report_cb(uint8_t cid, lte_pdn_event_t pdn_actevt, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] pdn activation event, cid %lu, event: %lu, priv: %p\n", (uint32_t)cid,
                    (uint32_t)pdn_actevt, userPriv);
}

static void print_retval(int32_t val, char *str) {
  if (0 != val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%ld\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%ld\"\n", str, val);
  }
}

static void apitest_task(void *arg) {
  int cmp_res;

  osStatus_t ret;
  struct apitest_command_s *command;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until send commands */
    APITEST_DBG_PRINT("Entering blocking by xQueueReceive.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      /* lte_attach_network */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "atchnet", strlen("atchnet"));
      if (cmp_res == 0) {
        if (2 <= command->argc) {
          print_retval(lte_attach_network(atchnet_cb, (void *)0xdeadbeef),
                       APITEST_GETFUNCNAME(lte_attach_network));
        }
      }

      /* lte_detach_network */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "dtchnet", strlen("dtchnet"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteradiomode_e radiomode;
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "NOP", strlen("NOP"))) {
            radiomode = LTE_NONOPERATIONAL_MODE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "FLIGHT", strlen("FLIGHT"))) {
            radiomode = LTE_FLIGHT_MODE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "OP", strlen("OP"))) {
            radiomode = LTE_OPERATIONAL_MODE;
          } else {
            radiomode = LTE_MODE_MAX;
          }

          print_retval(lte_detach_network(radiomode, dtchnet_cb, (void *)0xdeadbeef),
                       APITEST_GETFUNCNAME(lte_detach_network));
        }
      }

      /* lte_get_netstat */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtnetst", strlen("gtnetst"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          ltenetstate_e state;

          print_retval(result = lte_get_netstat(&state), APITEST_GETFUNCNAME(lte_get_netstat));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] state : %lu\n", (uint32_t)state);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_data_on */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "dataon", strlen("dataon"));
      if (cmp_res == 0) {
        if (3 <= command->argc) {
          uint8_t session_id;

          if ((0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                            strlen(APITEST_INVALID_ARG)))) {
            session_id = APITEST_INVALID_VALUE;
          } else {
            session_id =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (4 <= command->argc && 0 == strncmp(command->argv[APITEST_CMD_PARAM_2],
                                                 APITEST_NULL_ARG, strlen(APITEST_NULL_ARG))) {
            print_retval(lte_data_on(session_id, NULL, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_data_on));
          } else {
            print_retval(lte_data_on(session_id, daton_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_data_on));
          }
        }
      }

      /* lte_data_off */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "dataoff", strlen("dataoff"));
      if (cmp_res == 0) {
        if (3 <= command->argc) {
          uint8_t session_id;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            session_id = APITEST_INVALID_VALUE;
          } else {
            session_id =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (4 <= command->argc && 0 == strncmp(command->argv[APITEST_CMD_PARAM_2],
                                                 APITEST_NULL_ARG, strlen(APITEST_NULL_ARG))) {
            print_retval(lte_data_off(session_id, NULL, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_data_off));
          } else {
            print_retval(lte_data_off(session_id, datoff_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_data_off));
          }
        }
      }

      /* lte_get_datastate */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtdtst", strlen("gtdtst"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_datastatelist_t state;

          print_retval(result = lte_get_datastate(&state), APITEST_GETFUNCNAME(lte_get_datastate));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] listnum : %lu\n", (uint32_t)state.listnum);
            int i;
            for (i = 0; i < (state.listnum); i++) {
              APITEST_DBG_PRINT("[RET_VAL] session_id: %lu, state: %lu\n",
                                (uint32_t)state.statelist[i].session_id,
                                (uint32_t)state.statelist[i].state);
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_dataconfig */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtdtcfg", strlen("gtdtcfg"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          lteenableflag_e general, roaming;
          ltedatatype_e data_type;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_DATA_TYPE_USER",
                           strlen("LTE_DATA_TYPE_USER"))) {
            data_type = LTE_DATA_TYPE_USER;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_DATA_TYPE_IMS",
                                  strlen("LTE_DATA_TYPE_IMS"))) {
            data_type = LTE_DATA_TYPE_IMS;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            data_type = LTE_DATA_TYPE_IMS + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }
          print_retval(result = lte_get_dataconfig(data_type, &general, &roaming),
                       APITEST_GETFUNCNAME(lte_get_dataconfig));

          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] data_type : \"%lu\"\n", (uint32_t)data_type);
            APITEST_DBG_PRINT("[RET_VAL] general   : \"%lu\"\n", (uint32_t)general);
            APITEST_DBG_PRINT("[RET_VAL] roaming   : \"%lu\"\n", (uint32_t)roaming);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_dataconfig */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stdtcfg", strlen("stdtcfg"));
      if (cmp_res == 0) {
        if (5 == command->argc) {
          lteenableflag_e general, roaming;
          ltedatatype_e data_type;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_DATA_TYPE_USER",
                           strlen("LTE_DATA_TYPE_USER"))) {
            data_type = LTE_DATA_TYPE_USER;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_DATA_TYPE_IMS",
                                  strlen("LTE_DATA_TYPE_IMS"))) {
            data_type = LTE_DATA_TYPE_IMS;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            data_type = LTE_DATA_TYPE_IMS + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 ==
              strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_ENABLE", strlen("LTE_ENABLE"))) {
            general = LTE_ENABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_DISABLE",
                                  strlen("LTE_DISABLE"))) {
            general = LTE_DISABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 ==
              strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_ENABLE", strlen("LTE_ENABLE"))) {
            roaming = LTE_ENABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_DISABLE",
                                  strlen("LTE_DISABLE"))) {
            roaming = LTE_DISABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          print_retval(lte_set_dataconfig(data_type, general, roaming),
                       APITEST_GETFUNCNAME(lte_set_dataconfig));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_apnlist */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtapnst", strlen("gtapnst"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          static lte_apnlist_t *apn;

          if (!apn) {
            apn = malloc(sizeof(lte_apnlist_t));
            assert(apn);
          }

          memset((void *)apn, 0, sizeof(lte_apnlist_t));
          print_retval(result = lte_get_apnlist(apn), APITEST_GETFUNCNAME(lte_get_apnlist));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] listnum : %d\n", (apn->listnum));
            int i;
            for (i = 0; i < (apn->listnum); i++) {
              APITEST_DBG_PRINT("[RET_VAL] [%d] session_id : \"%lu\"\n", i,
                                (uint32_t)apn->apnlist[i].session_id);
              APITEST_DBG_PRINT("[RET_VAL] [%d] apn        : \"%s\"\n", i, apn->apnlist[i].apn);
              APITEST_DBG_PRINT("[RET_VAL] [%d] ip_type    : \"%lu\"\n", i,
                                (uint32_t)apn->apnlist[i].ip_type);
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_apnlistv2 */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtv2apnst", strlen("gtv2apnst"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          static lte_apnlistv2_t *apn = NULL;

          if (!apn) {
            apn = malloc(sizeof(lte_apnlistv2_t));
            assert(apn);
          }

          memset((void *)apn, 0, sizeof(lte_apnlistv2_t));
          print_retval(result = lte_get_apnlistv2(apn), APITEST_GETFUNCNAME(lte_get_apnlistv2));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] listnum : %lu\n", (uint32_t)apn->listnum);
            int i;
            for (i = 0; i < (apn->listnum); i++) {
              APITEST_DBG_PRINT("[RET_VAL] session_id: %lu\n",
                                (uint32_t)apn->apnlist[i].session_id);
              APITEST_DBG_PRINT("[RET_VAL] apn: %s\n", apn->apnlist[i].apn);
              APITEST_DBG_PRINT("[RET_VAL] ip_type: %lu\n", (uint32_t)apn->apnlist[i].ip_type);
              APITEST_DBG_PRINT("[RET_VAL] auth_type: %lu\n", (uint32_t)apn->apnlist[i].auth_type);
              APITEST_DBG_PRINT("[RET_VAL] username: %s\n", apn->apnlist[i].username);
              APITEST_DBG_PRINT("[RET_VAL] password: %s\n", apn->apnlist[i].password);
              APITEST_DBG_PRINT("[RET_VAL] hostname: %s\n", apn->apnlist[i].hostname);
              APITEST_DBG_PRINT("[RET_VAL] ipv4addralloc: %lu\n",
                                (uint32_t)apn->apnlist[i].ipv4addralloc);
              APITEST_DBG_PRINT("[RET_VAL] pcscf_discovery: %lu\n",
                                (uint32_t)apn->apnlist[i].pcscf_discovery);
              APITEST_DBG_PRINT("[RET_VAL] nslpi: %lu\n", (uint32_t)apn->apnlist[i].nslpi);
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_apn */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stapn", strlen("stapn"));
      if (cmp_res == 0) {
        if (8 == command->argc) {
          uint8_t session_id;
          lteapniptype_e ip_type;
          lteapnauthtype_e auth_type;
          int8_t *apnname = NULL;
          int8_t *usrname = NULL;
          int8_t *passwrd = NULL;
          int8_t apnname_tmp[LTE_APN_LEN];
          int8_t usrname_tmp[LTE_APN_USER_NAME_LEN];
          int8_t passwrd_tmp[LTE_APN_PASSWD_LEN];

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            session_id = APITEST_INVALID_VALUE;
          } else {
            session_id =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            apnname = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(apnname_tmp, 'X', LTE_APN_LEN - 1);
            apnname_tmp[LTE_APN_LEN - 1] = 0;
            apnname = apnname_tmp;
          } else {
            strncpy((char *)apnname_tmp, (char *)command->argv[APITEST_CMD_PARAM_2], LTE_APN_LEN);
            apnname = apnname_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IPV6",
                           strlen("LTE_APN_IPTYPE_IPV6"))) {
            ip_type = LTE_APN_IPTYPE_IPV6;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IPV4V6",
                                  strlen("LTE_APN_IPTYPE_IPV4V6"))) {
            ip_type = LTE_APN_IPTYPE_IPV4V6;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IP",
                                  strlen("LTE_APN_IPTYPE_IP"))) {
            ip_type = LTE_APN_IPTYPE_IP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_NONIP",
                                  strlen("LTE_APN_IPTYPE_NONIP"))) {
            ip_type = LTE_APN_IPTYPE_NONIP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            ip_type = LTE_APN_IPTYPE_NONIP + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_NONE",
                           strlen("LTE_APN_AUTHTYPE_NONE"))) {
            auth_type = LTE_APN_AUTHTYPE_NONE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_PAP",
                                  strlen("LTE_APN_AUTHTYPE_PAP"))) {
            auth_type = LTE_APN_AUTHTYPE_PAP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_CHAP",
                                  strlen("LTE_APN_AUTHTYPE_CHAP"))) {
            auth_type = LTE_APN_AUTHTYPE_CHAP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            auth_type = LTE_APN_AUTHTYPE_CHAP + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            usrname = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(usrname_tmp, 'X', LTE_APN_USER_NAME_LEN - 1);
            usrname_tmp[LTE_APN_USER_NAME_LEN - 1] = 0;
            usrname = usrname_tmp;
          } else {
            strncpy((char *)usrname_tmp, (char *)command->argv[APITEST_CMD_PARAM_5],
                    LTE_APN_USER_NAME_LEN);
            usrname = usrname_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_6], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            passwrd = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_6], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(passwrd_tmp, 'X', LTE_APN_PASSWD_LEN - 1);
            passwrd_tmp[LTE_APN_PASSWD_LEN - 1] = 0;
            passwrd = passwrd_tmp;
          } else {
            strncpy((char *)passwrd_tmp, (char *)command->argv[APITEST_CMD_PARAM_6],
                    LTE_APN_PASSWD_LEN);
            passwrd = passwrd_tmp;
          }

          print_retval(lte_set_apn(session_id, apnname, ip_type, auth_type, usrname, passwrd),
                       APITEST_GETFUNCNAME(lte_set_apn));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_apnv2 */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stv2apn", strlen("stv2apn"));
      if (cmp_res == 0) {
        if (12 == command->argc) {
          uint8_t session_id;
          lteapniptype_e ip_type;
          lteapnauthtype_e auth_type;
          int8_t *apnname = NULL;
          int8_t *usrname = NULL;
          int8_t *passwrd = NULL;
          int8_t *hostname = NULL;
          int8_t apnname_tmp[LTE_APN_LEN];
          int8_t usrname_tmp[LTE_APN_USER_NAME_LEN];
          int8_t passwrd_tmp[LTE_APN_PASSWD_LEN];
          int8_t hostname_tmp[LTE_APN_LEN];
          lteenableflag_e pcscf_discovery;
          lte_ipv4addralloc_t ipv4addralloc;
          lte_nslpi_t nslpi;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            session_id = APITEST_INVALID_VALUE;
          } else {
            session_id =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            apnname = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(apnname_tmp, 'X', LTE_APN_LEN - 1);
            apnname_tmp[LTE_APN_LEN - 1] = 0;
            apnname = apnname_tmp;
          } else {
            strncpy((char *)apnname_tmp, (char *)command->argv[APITEST_CMD_PARAM_2], LTE_APN_LEN);
            apnname = apnname_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IPV6",
                           strlen("LTE_APN_IPTYPE_IPV6"))) {
            ip_type = LTE_APN_IPTYPE_IPV6;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IPV4V6",
                                  strlen("LTE_APN_IPTYPE_IPV4V6"))) {
            ip_type = LTE_APN_IPTYPE_IPV4V6;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_IP",
                                  strlen("LTE_APN_IPTYPE_IP"))) {
            ip_type = LTE_APN_IPTYPE_IP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_APN_IPTYPE_NONIP",
                                  strlen("LTE_APN_IPTYPE_NONIP"))) {
            ip_type = LTE_APN_IPTYPE_NONIP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            ip_type = LTE_APN_IPTYPE_NONIP + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_NONE",
                           strlen("LTE_APN_AUTHTYPE_NONE"))) {
            auth_type = LTE_APN_AUTHTYPE_NONE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_PAP",
                                  strlen("LTE_APN_AUTHTYPE_PAP"))) {
            auth_type = LTE_APN_AUTHTYPE_PAP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_APN_AUTHTYPE_CHAP",
                                  strlen("LTE_APN_AUTHTYPE_CHAP"))) {
            auth_type = LTE_APN_AUTHTYPE_CHAP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            auth_type = LTE_APN_AUTHTYPE_CHAP + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            usrname = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(usrname_tmp, 'X', LTE_APN_USER_NAME_LEN - 1);
            usrname_tmp[LTE_APN_USER_NAME_LEN - 1] = 0;
            usrname = usrname_tmp;
          } else {
            strncpy((char *)usrname_tmp, (char *)command->argv[APITEST_CMD_PARAM_5],
                    LTE_APN_USER_NAME_LEN);
            usrname = usrname_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_6], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            passwrd = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_6], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(passwrd_tmp, 'X', LTE_APN_PASSWD_LEN - 1);
            passwrd_tmp[LTE_APN_PASSWD_LEN - 1] = 0;
            passwrd = passwrd_tmp;
          } else {
            strncpy((char *)passwrd_tmp, (char *)command->argv[APITEST_CMD_PARAM_6],
                    LTE_APN_PASSWD_LEN);
            passwrd = passwrd_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_7], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            passwrd = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_7], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(hostname_tmp, 'X', LTE_APN_LEN - 1);
            hostname_tmp[LTE_APN_LEN - 1] = 0;
            hostname = hostname_tmp;
          } else {
            strncpy((char *)hostname_tmp, (char *)command->argv[APITEST_CMD_PARAM_7], LTE_APN_LEN);
            hostname = hostname_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_8], "DISABLE", strlen("DISABLE"))) {
            pcscf_discovery = LTE_DISABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_8], "ENABLE", strlen("ENABLE"))) {
            pcscf_discovery = LTE_ENABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_8], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            pcscf_discovery = LTE_ENABLE + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 ==
              strncmp(command->argv[APITEST_CMD_PARAM_9], "ALLOC_NASSIG", strlen("ALLOC_NASSIG"))) {
            ipv4addralloc = ALLOC_NASSIG;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_9], "ALLOC_DHCP",
                                  strlen("ALLOC_DHCP"))) {
            ipv4addralloc = ALLOC_DHCP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_9], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            ipv4addralloc = ALLOC_MAX;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_10], "NSLPI_CONFIGURED",
                           strlen("NSLPI_CONFIGURED"))) {
            nslpi = NSLPI_CONFIGURED;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_10], "NSLPI_NOTCONFIGURED",
                                  strlen("NSLPI_NOTCONFIGURED"))) {
            nslpi = NSLPI_NOTCONFIGURED;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_10], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            nslpi = NSLPI_MAX;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          print_retval(lte_set_apnv2(session_id, apnname, ip_type, auth_type, usrname, passwrd,
                                     hostname, pcscf_discovery, ipv4addralloc, nslpi),
                       APITEST_GETFUNCNAME(lte_set_apnv2));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_version */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtver", strlen("gtver"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_version_t version;

          memset(&version, 0x0, sizeof(lte_version_t));
          print_retval(result = lte_get_version(&version), APITEST_GETFUNCNAME(lte_get_version));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] bb_product : \"%s\"\n", version.bb_product);
            APITEST_DBG_PRINT("[RET_VAL] np_package : \"%s\"\n", version.np_package);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_phoneno */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtphno", strlen("gtphno"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lteerrcause_e errcause;
          int8_t phoneno[LTE_MAX_PHONENUM_LEN];

          print_retval(result = lte_get_phoneno(&errcause, phoneno),
                       APITEST_GETFUNCNAME(lte_get_phoneno));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] phoneno : \"%s\"\n", phoneno);
          } else {
            APITEST_DBG_PRINT("[RET_VAL] errcause : %lu\n", (uint32_t)errcause);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_imsi */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtimsi", strlen("gtimsi"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lteerrcause_e errcause;
          int8_t imsi[LTE_MAX_IMSI_LEN];

          print_retval(result = lte_get_imsi(&errcause, imsi), APITEST_GETFUNCNAME(lte_get_imsi));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] imsi : \"%s\"\n", imsi);
          } else {
            APITEST_DBG_PRINT("[RET_VAL] errcause : %lu\n", (uint32_t)errcause);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_imei */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtimei", strlen("gtimei"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          int8_t imei[LTE_MAX_IMEI_LEN];

          print_retval(result = lte_get_imei(imei), APITEST_GETFUNCNAME(lte_get_imei));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] imei : \"%s\"\n", imei);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_pinattributes */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtpinst", strlen("gtpinst"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_pin_attributes_t pinattr;

          print_retval(result = lte_get_pinattributes(&pinattr),
                       APITEST_GETFUNCNAME(lte_get_pinattributes));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] enable            : \"%lu\"\n", (uint32_t)pinattr.enable);
            if (LTE_PIN_ENABLE == pinattr.enable) {
              APITEST_DBG_PRINT("[RET_VAL] status            : \"%lu\"\n",
                                (uint32_t)pinattr.status);
            }

            APITEST_DBG_PRINT("[RET_VAL] pin_attemptsleft  : \"%lu\"\n",
                              (uint32_t)pinattr.pin_attemptsleft);
            APITEST_DBG_PRINT("[RET_VAL] puk_attemptsleft  : \"%lu\"\n",
                              (uint32_t)pinattr.puk_attemptsleft);
            APITEST_DBG_PRINT("[RET_VAL] pin2_attemptsleft : \"%lu\"\n",
                              (uint32_t)pinattr.pin2_attemptsleft);
            APITEST_DBG_PRINT("[RET_VAL] puk2_attemptsleft : \"%lu\"\n",
                              (uint32_t)pinattr.puk2_attemptsleft);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_pinenable */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stpinenb", strlen("stpinenb"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          lteresult_e result;
          lteenableflag_e enable;
          int8_t *pincode;
          int8_t pincode_tmp[LTE_MAX_PINCODE_LEN + 1];
          uint8_t attemptsleft;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_PIN_ENABLE",
                           strlen("LTE_PIN_ENABLE"))) {
            enable = LTE_ENABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_PIN_DISABLE",
                                  strlen("LTE_PIN_DISABLE"))) {
            enable = LTE_DISABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            pincode = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(pincode_tmp, '0', LTE_MAX_PINCODE_LEN);
            pincode_tmp[LTE_MAX_PINCODE_LEN] = 0;
            pincode = pincode_tmp;
          } else {
            strncpy((char *)pincode_tmp, (char *)command->argv[APITEST_CMD_PARAM_2],
                    LTE_MAX_PINCODE_LEN + 1);
            pincode = pincode_tmp;
          }

          print_retval(result = lte_set_pinenable(enable, pincode, &attemptsleft),
                       APITEST_GETFUNCNAME(lte_set_pinenable));
          if (LTE_RESULT_ERROR == result) {
            APITEST_DBG_PRINT("[RET_VAL] attemptsleft : \"%lu\"\n", (uint32_t)attemptsleft);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_cfun */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stcfun", strlen("stcfun"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          ltecfunoptn_e fun;

          fun =
              (ltecfunoptn_e)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);

          print_retval(result = lte_set_cfun(fun), APITEST_GETFUNCNAME(lte_set_cfun));
          if (LTE_RESULT_ERROR == result) {
            APITEST_DBG_PRINT("[RET_VAL] OK\n");
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_change_pin */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "cngpin", strlen("cngpin"));
      if (cmp_res == 0) {
        if (5 == command->argc) {
          lteresult_e result;
          ltetargetpin_e tgt_pin;
          int8_t *pincode;
          int8_t *newpincode;
          int8_t pincode_tmp[LTE_MAX_PINCODE_LEN + 1];
          int8_t newpincode_tmp[LTE_MAX_PINCODE_LEN + 1];
          uint8_t attemptsleft;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_TARGET_PIN2",
                           strlen("LTE_TARGET_PIN2"))) {
            tgt_pin = LTE_TARGET_PIN2;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_TARGET_PIN",
                                  strlen("LTE_TARGET_PIN"))) {
            tgt_pin = LTE_TARGET_PIN;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            tgt_pin = LTE_TARGET_PIN2 + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            pincode = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(pincode_tmp, '0', LTE_MAX_PINCODE_LEN);
            pincode_tmp[LTE_MAX_PINCODE_LEN] = 0;
            pincode = pincode_tmp;
          } else {
            strncpy((char *)pincode_tmp, (char *)command->argv[APITEST_CMD_PARAM_2],
                    LTE_MAX_PINCODE_LEN + 1);
            pincode = pincode_tmp;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            newpincode = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(newpincode_tmp, '0', LTE_MAX_PINCODE_LEN);
            newpincode_tmp[LTE_MAX_PINCODE_LEN] = 0;
            newpincode = newpincode_tmp;
          } else {
            strncpy((char *)newpincode_tmp, (char *)command->argv[APITEST_CMD_PARAM_3],
                    LTE_MAX_PINCODE_LEN + 1);
            newpincode = newpincode_tmp;
          }

          print_retval(result = lte_change_pin(tgt_pin, pincode, newpincode, &attemptsleft),
                       APITEST_GETFUNCNAME(lte_change_pin));
          if (LTE_RESULT_ERROR == result) {
            APITEST_DBG_PRINT("[RET_VAL] attemptsleft : \"%lu\"\n", (uint32_t)attemptsleft);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_enter_pin */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "entpin", strlen("entpin"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          lteresult_e result;
          int8_t *pincode;
          int8_t *newpincode;
          int8_t pincode_tmp[LTE_MAX_PINCODE_LEN + 1];
          int8_t newpincode_tmp[LTE_MAX_PINCODE_LEN + 1];
          ltesimstate_e simstate;
          uint8_t attemptsleft;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            pincode = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(pincode_tmp, '0', LTE_MAX_PINCODE_LEN);
            pincode_tmp[LTE_MAX_PINCODE_LEN] = 0;
            pincode = pincode_tmp;
          } else {
            strncpy((char *)pincode_tmp, (char *)command->argv[APITEST_CMD_PARAM_1],
                    LTE_MAX_PINCODE_LEN + 1);
            pincode = pincode_tmp;
          }
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NULL_ARG,
                           strlen(APITEST_NULL_ARG))) {
            newpincode = NULL;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            memset(newpincode_tmp, '0', LTE_MAX_PINCODE_LEN);
            newpincode_tmp[LTE_MAX_PINCODE_LEN] = 0;
            newpincode = newpincode_tmp;
          } else {
            strncpy((char *)newpincode_tmp, (char *)command->argv[APITEST_CMD_PARAM_2],
                    LTE_MAX_PINCODE_LEN + 1);
            newpincode = newpincode_tmp;
          }

          print_retval(result = lte_enter_pin(pincode, newpincode, &simstate, &attemptsleft),
                       APITEST_GETFUNCNAME(lte_enter_pin));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] simstate : \"%lu\"\n", (uint32_t)simstate);
          } else {
            APITEST_DBG_PRINT("[RET_VAL] attemptsleft : \"%lu\"\n", (uint32_t)attemptsleft);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_localtime */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtlcltm", strlen("gtlcltm"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_localtime_t localtime;

          print_retval(result = lte_get_localtime(&localtime),
                       APITEST_GETFUNCNAME(lte_get_localtime));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] time      : \"%02ld/%02ld/%02ld : %02ld:%02ld:%02ld\"\n",
                              localtime.year, localtime.mon, localtime.mday, localtime.hour,
                              localtime.min, localtime.sec);
            APITEST_DBG_PRINT("[RET_VAL] time_zone : \"%ld\"\n", localtime.tz_sec);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }
      /* lte_get_ccid */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtccid", strlen("gtccid"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_ccid_t ccid;

          print_retval(result = lte_get_ccid(&ccid), APITEST_GETFUNCNAME(lte_get_ccid));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] ccid      : \"%s\"\n", ccid.ccid);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_timing_advance */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtta", strlen("gtta"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_timging_advance_t ta;

          print_retval(result = lte_get_timing_advance(&ta),
                       APITEST_GETFUNCNAME(lte_get_timing_advance));
          if (LTE_RESULT_OK == result) {
            if (ta.timing_advance != LTE_TIMING_ADV_NA)
              APITEST_DBG_PRINT("[RET_VAL] timing advance  : \"%d\"\n", ta.timing_advance);
            else
              APITEST_DBG_PRINT("[RET_VAL] timing advance  : \"NA\"\n");
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_iuicc_status */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtuicc", strlen("gtuicc"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_get_iuicc_status_t iuicc;

          print_retval(result = lte_get_iuicc_status(&iuicc),
                       APITEST_GETFUNCNAME(lte_get_iuicc_status));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] IUICC Status  : \"%d\"\n", iuicc.iuicc_status);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_operator */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtoprtr", strlen("gtoprtr"));
      if (cmp_res == 0) {
        if (2 <= command->argc) {
          lteresult_e result;
          int8_t oper[LTE_MAX_OPERATOR_NAME_LEN];

          print_retval(result = lte_get_operator(oper), APITEST_GETFUNCNAME(lte_get_operator));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] operator : \"%s\"\n", oper);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_edrx */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtedrx", strlen("gtedrx"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          lte_edrx_setting_t settings;
          ltecfgtype_e cfgType;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_REQUESTED_ARG,
                           strlen(APITEST_REQUESTED_ARG))) {
            cfgType = LTE_CFGTYPE_REQUESTED;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_NWPROVIDED_ARG,
                                  strlen(APITEST_NWPROVIDED_ARG))) {
            cfgType = LTE_CFGTYPE_NW_PROVIDED;
          } else {
            cfgType = LTE_CFGTYPE_MAX;
          }

          print_retval(result = lte_get_edrx(&settings, cfgType),
                       APITEST_GETFUNCNAME(lte_get_edrx));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] act type   : \"%lu\"\n", (uint32_t)settings.act_type);
            APITEST_DBG_PRINT("[RET_VAL] enable     : \"%lu\"\n", (uint32_t)settings.enable);

            if (LTE_ENABLE == settings.enable) {
              APITEST_DBG_PRINT("[RET_VAL] edrx_cycle : \"%lu\"\n", (uint32_t)settings.edrx_cycle);
              APITEST_DBG_PRINT("[RET_VAL] ptw_val    : \"%lu\"\n", (uint32_t)settings.ptw_val);
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_edrx */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stedrx", strlen("stedrx"));
      if (cmp_res == 0) {
        if (6 == command->argc) {
          lte_edrx_setting_t *edrx_settings = NULL;
          lte_edrx_setting_t edrx_stg_tmp;
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_EDRX_ACTTYPE_WBS1",
                           strlen("LTE_EDRX_ACTTYPE_WBS1"))) {
            edrx_stg_tmp.act_type = LTE_EDRX_ACTTYPE_WBS1;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_EDRX_ACTTYPE_NBS1",
                                  strlen("LTE_EDRX_ACTTYPE_NBS1"))) {
            edrx_stg_tmp.act_type = LTE_EDRX_ACTTYPE_NBS1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 ==
              strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_ENABLE", strlen("LTE_ENABLE"))) {
            edrx_stg_tmp.enable = LTE_ENABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_DISABLE",
                                  strlen("LTE_DISABLE"))) {
            edrx_stg_tmp.enable = LTE_DISABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_512",
                           strlen("LTE_EDRX_CYC_512"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_512;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_1024",
                                  strlen("LTE_EDRX_CYC_1024"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_1024;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_2048",
                                  strlen("LTE_EDRX_CYC_2048"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_2048;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_4096",
                                  strlen("LTE_EDRX_CYC_4096"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_4096;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_6144",
                                  strlen("LTE_EDRX_CYC_6144"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_6144;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_8192",
                                  strlen("LTE_EDRX_CYC_8192"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_8192;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_10240",
                                  strlen("LTE_EDRX_CYC_10240"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_10240;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_12288",
                                  strlen("LTE_EDRX_CYC_12288"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_12288;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_14336",
                                  strlen("LTE_EDRX_CYC_14336"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_14336;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_16384",
                                  strlen("LTE_EDRX_CYC_16384"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_16384;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_32768",
                                  strlen("LTE_EDRX_CYC_32768"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_32768;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_65536",
                                  strlen("LTE_EDRX_CYC_65536"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_65536;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_131072",
                                  strlen("LTE_EDRX_CYC_131072"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_131072;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_262144",
                                  strlen("LTE_EDRX_CYC_262144"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_262144;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_524288",
                                  strlen("LTE_EDRX_CYC_524288"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_524288;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "LTE_EDRX_CYC_1048576",
                                  strlen("LTE_EDRX_CYC_1048576"))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_1048576;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            edrx_stg_tmp.edrx_cycle = LTE_EDRX_CYC_262144 + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_128",
                           strlen("LTE_EDRX_PTW_128"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_128;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_256",
                                  strlen("LTE_EDRX_PTW_256"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_256;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_384",
                                  strlen("LTE_EDRX_PTW_384"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_384;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_512",
                                  strlen("LTE_EDRX_PTW_512"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_512;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_640",
                                  strlen("LTE_EDRX_PTW_640"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_640;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_768",
                                  strlen("LTE_EDRX_PTW_768"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_768;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_896",
                                  strlen("LTE_EDRX_PTW_896"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_896;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1024",
                                  strlen("LTE_EDRX_PTW_1024"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1024;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1152",
                                  strlen("LTE_EDRX_PTW_1152"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1152;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1280",
                                  strlen("LTE_EDRX_PTW_1280"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1280;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1408",
                                  strlen("LTE_EDRX_PTW_1408"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1408;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1536",
                                  strlen("LTE_EDRX_PTW_1536"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1536;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1664",
                                  strlen("LTE_EDRX_PTW_1664"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1664;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1792",
                                  strlen("LTE_EDRX_PTW_1792"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1792;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_1920",
                                  strlen("LTE_EDRX_PTW_1920"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_1920;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_EDRX_PTW_2048",
                                  strlen("LTE_EDRX_PTW_2048"))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_2048;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            edrx_stg_tmp.ptw_val = LTE_EDRX_PTW_2048 + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          edrx_settings = &edrx_stg_tmp;
          print_retval(lte_set_edrx(edrx_settings), APITEST_GETFUNCNAME(lte_set_edrx));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_psm */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtpsm", strlen("gtpsm"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          lte_psm_setting_t settings;
          ltecfgtype_e cfgType;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_REQUESTED_ARG,
                           strlen(APITEST_REQUESTED_ARG))) {
            cfgType = LTE_CFGTYPE_REQUESTED;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_NWPROVIDED_ARG,
                                  strlen(APITEST_NWPROVIDED_ARG))) {
            cfgType = LTE_CFGTYPE_NW_PROVIDED;
          } else {
            cfgType = LTE_CFGTYPE_MAX;
          }

          print_retval(result = lte_get_psm(&settings, cfgType), APITEST_GETFUNCNAME(lte_get_psm));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] func                         : \"%lu\"\n",
                              (uint32_t)settings.func);
            if (LTE_PSM_DISABLE != settings.func) {
              APITEST_DBG_PRINT("[RET_VAL] active_time.unit           : \"%lu\"\n",
                                (uint32_t)settings.active_time.unit);
              APITEST_DBG_PRINT("[RET_VAL] active_time.time_val       : \"%lu\"\n",
                                (uint32_t)settings.active_time.time_val);
              APITEST_DBG_PRINT("[RET_VAL] ext_periodic_tau_time.unit     : \"%lu\"\n",
                                (uint32_t)settings.ext_periodic_tau_time.unit);
              APITEST_DBG_PRINT("[RET_VAL] ext_periodic_tau_time.time_val : \"%lu\"\n",
                                (uint32_t)settings.ext_periodic_tau_time.time_val);
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_psm */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stpsm", strlen("stpsm"));
      if (cmp_res == 0) {
        if (7 == command->argc) {
          lte_psm_setting_t *psm_settings = NULL;
          lte_psm_setting_t psm_stg_tmp;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_ENABLE_ON",
                           strlen("LTE_ENABLE_ON"))) {
            psm_stg_tmp.func = LTE_PSM_ENABLE_FORCE_ON;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_ENABLE_OFF",
                                  strlen("LTE_ENABLE_OFF"))) {
            psm_stg_tmp.func = LTE_PSM_ENABLE_FORCE_OFF;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LTE_DISABLE",
                                  strlen("LTE_DISABLE"))) {
            psm_stg_tmp.func = LTE_PSM_DISABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_PSM_T3324_UNIT_2SEC",
                           strlen("LTE_PSM_T3324_UNIT_2SEC"))) {
            psm_stg_tmp.active_time.unit = LTE_PSM_T3324_UNIT_2SEC;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_PSM_T3324_UNIT_1MIN",
                                  strlen("LTE_PSM_T3324_UNIT_1MIN"))) {
            psm_stg_tmp.active_time.unit = LTE_PSM_T3324_UNIT_1MIN;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "LTE_PSM_T3324_UNIT_6MIN",
                                  strlen("LTE_PSM_T3324_UNIT_6MIN"))) {
            psm_stg_tmp.active_time.unit = LTE_PSM_T3324_UNIT_6MIN;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            psm_stg_tmp.active_time.unit = LTE_PSM_T3324_UNIT_6MIN + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            psm_stg_tmp.active_time.time_val = APITEST_INVALID_VALUE;
          } else {
            psm_stg_tmp.active_time.time_val =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_3], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_2SEC",
                           strlen("LTE_PSM_T3412_UNIT_2SEC"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_2SEC;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_30SEC",
                                  strlen("LTE_PSM_T3412_UNIT_30SEC"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_30SEC;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_1MIN",
                                  strlen("LTE_PSM_T3412_UNIT_1MIN"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_1MIN;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_10MIN",
                                  strlen("LTE_PSM_T3412_UNIT_10MIN"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_10MIN;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_1HOUR",
                                  strlen("LTE_PSM_T3412_UNIT_1HOUR"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_1HOUR;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_10HOUR",
                                  strlen("LTE_PSM_T3412_UNIT_10HOUR"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_10HOUR;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "LTE_PSM_T3412_UNIT_320HOUR",
                                  strlen("LTE_PSM_T3412_UNIT_320HOUR"))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_320HOUR;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], APITEST_INVALID_ARG,
                                  strlen(APITEST_INVALID_ARG))) {
            psm_stg_tmp.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_320HOUR + 1;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_5], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            psm_stg_tmp.ext_periodic_tau_time.time_val = APITEST_INVALID_VALUE;
          } else {
            psm_stg_tmp.ext_periodic_tau_time.time_val =
                (uint8_t)strtol(command->argv[APITEST_CMD_PARAM_5], NULL, APITEST_STRTOL_BASE);
          }

          psm_settings = &psm_stg_tmp;
          print_retval(lte_set_psm(psm_settings), APITEST_GETFUNCNAME(lte_set_psm));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_netstate */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strpntst", strlen("strpntst"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_netstate(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_netstate));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_netstate(ntstrepo_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_netstate));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_simstate */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strpsmst", strlen("strpsmst"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_simstate(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_simstate));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_simstate(smstrepo_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_simstate));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_localtime */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strplcltm", strlen("strplcltm"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_localtime(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_localtime));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_localtime(lcltmrepo_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_localtime));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_quality */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strpqty", strlen("strpqty"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint32_t period;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            period = 0;
          } else {
            period =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_quality(NULL, period, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_quality));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_quality(qtyrepo_cb, period, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_quality));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_2]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_quality_2g */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strp2gqty", strlen("strp2gqty"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint32_t period;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            period = 0;
          } else {
            period =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_quality_2g(NULL, period, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_quality_2g));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_quality_2g(qtyrepo_2g_cb, period, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_quality_2g));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_2]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_timerevt */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "sttmrevt", strlen("sttmrevt"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint32_t prec_time;
          lte_settimerevt_t sttmrevt;
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "T3412", strlen("T3412"))) {
            prec_time =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_2], NULL, APITEST_STRTOL_BASE);

            sttmrevt.ev_type = LTE_EVTYPE_3412;
            sttmrevt.mode = prec_time ? LTE_MODE_ENABLE : LTE_MODE_DISABLE;
            sttmrevt.ev_sel = LTE_EVSEL_EXPIRED;
            sttmrevt.preceding_time = prec_time;
            print_retval(lte_set_report_timerevt(timerevt_cb, &sttmrevt, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_timerevt));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "T3402", strlen("T3402"))) {
            prec_time =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_2], NULL, APITEST_STRTOL_BASE);
            sttmrevt.ev_type = LTE_EVTYPE_3402;
            sttmrevt.ev_sel = LTE_EVSEL_STOP | LTE_EVSEL_START;
            sttmrevt.mode = prec_time ? LTE_MODE_ENABLE : LTE_MODE_DISABLE;
            sttmrevt.preceding_time = 0;
            print_retval(lte_set_report_timerevt(timerevt_cb, &sttmrevt, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_timerevt));

          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_cellinfo */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strpclinf", strlen("strpclinf"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint32_t period;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            period = 0;
          } else {
            period =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_cellinfo(NULL, period, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_cellinfo));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_cellinfo(clinfrepo_cb, period, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_cellinfo));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_2]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_sleepmode */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtslp", strlen("gtslp"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lteslpmode_e slpmode;
          print_retval(result = lte_get_sleepmode(&slpmode),
                       APITEST_GETFUNCNAME(lte_get_sleepmode));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] sleep mode   : \"%lu\"\n", (uint32_t)slpmode);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_sleepmode */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stslp", strlen("stslp"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteslpmode_e slpmode;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "DISABLE", strlen("DISABLE"))) {
            slpmode = LTE_SLPMODE_DISABLED;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "NAP", strlen("NAP"))) {
            slpmode = LTE_SLPMODE_NAP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "LS", strlen("LS"))) {
            slpmode = LTE_SLPMODE_LIGHTSLEEP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "DS", strlen("DS"))) {
            slpmode = LTE_SLPMODE_DEEPSLEEP;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "DH0", strlen("DH0"))) {
            slpmode = LTE_SLPMODE_DEEPHIBER0;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "DH1", strlen("DH1"))) {
            slpmode = LTE_SLPMODE_DEEPHIBER1;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], "DH2", strlen("DH2"))) {
            slpmode = LTE_SLPMODE_DEEPHIBER2;
          } else {
            slpmode = LTE_SLPMODE_DEEPHIBER2 + 1;
          }

          print_retval(lte_set_sleepmode(slpmode), APITEST_GETFUNCNAME(lte_set_sleepmode));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_regstate */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpreg", strlen("rpreg"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_NULL_ARG))) {
            print_retval(lte_set_report_regstate(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_regstate));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_NULL_ARG))) {
            print_retval(lte_set_report_regstate(regstate_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_regstate));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_rat */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtrat", strlen("gtrat"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_get_rat_t rat;
          print_retval(result = lte_get_rat(&rat), APITEST_GETFUNCNAME(lte_get_rat));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] RAT  : \"%d\"\n", rat.rat);
            APITEST_DBG_PRINT("[RET_VAL] RAT Mode  : \"%d\"\n", rat.rat_mode);
            APITEST_DBG_PRINT("[RET_VAL] RAT Source  : \"%d\"\n", rat.source);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_rat */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strat", strlen("strat"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          lte_set_rat_t rat;
          rat.rat = strtol(command->argv[APITEST_CMD_PARAM_1], NULL, 10);
          rat.persis = strtol(command->argv[APITEST_CMD_PARAM_2], NULL, 10);

          print_retval(lte_set_rat(&rat), APITEST_GETFUNCNAME(lte_set_rat));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          show_help("strat");
          goto errout_with_param;
        }
      }

      /* lte_set_report_active_psm */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpactpsm", strlen("rpactpsm"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_active_psm(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_active_psm));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_active_psm(psmstate_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_active_psm));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_dynamic_psm */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpdynpsm", strlen("rpdynpsm"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_dynamic_psm(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_dynamic_psm));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_dynamic_psm(dynpsm_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_dynamic_psm));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_dynamic_edrx */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpdynedrx", strlen("rpdynedrx"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_dynamic_edrx(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_dynamic_edrx));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_dynamic_edrx(dynedrx_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_dynamic_edrx));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_antitamper */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpantitamper", strlen("rpantitamper"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_antitamper(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_antitamper));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_antitamper(antitamperEvt_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_antitamper));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_connectivity_phase */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpconnph", strlen("rpconnph"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_connectivity_phase(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_connectivity_phase));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_connectivity_phase(connphase_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_connectivity_phase));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_psm_camp */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "psmcamp", strlen("psmcamp"));
      if (cmp_res == 0) {
        lteenableflag_e enable;
        uint32_t timeout;

        if (4 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_NULL_ARG))) {
            enable = LTE_DISABLE;
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_NULL_ARG))) {
            enable = LTE_ENABLE;
          } else {
            APITEST_DBG_PRINT("unexpected parameter\n");
            goto errout_with_param;
          }

          timeout = atoi(command->argv[APITEST_CMD_PARAM_2]);
          print_retval(lte_psm_camp(enable, timeout), APITEST_GETFUNCNAME(lte_psm_camp));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_cellinfo */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtcellinfo", strlen("gtcellinfo"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_cellinfo_t cellinfo;
          int i;

          print_retval(result = lte_get_cellinfo(&cellinfo), APITEST_GETFUNCNAME(lte_get_cellinfo));
          if (LTE_RESULT_OK == result && (cellinfo.valid & LTE_VALID)) {
            APITEST_DBG_PRINT("[RET_VAL] phycell_id : \"%lu\"\n", cellinfo.phycell_id);
            APITEST_DBG_PRINT("[RET_VAL] earfcn     : \"%lu\"\n", cellinfo.earfcn);

            for (i = 0; i < LTE_CELLINFO_MCC_DIGIT; i++) {
              APITEST_DBG_PRINT("[RET_VAL] mcc[%d]    : \"%d\"\n", i, cellinfo.mcc[i]);
            }

            APITEST_DBG_PRINT("[RET_VAL] mnc_digit  : \"%d\"\n", cellinfo.mnc_digit);

            for (i = 0; i < cellinfo.mnc_digit; i++) {
              APITEST_DBG_PRINT("[RET_VAL] mnc[%d]    : \"%d\"\n", i, cellinfo.mnc[i]);
            }

            APITEST_DBG_PRINT("[RET_VAL] cgid    : \"%s\"\n", cellinfo.cgid);
            APITEST_DBG_PRINT("[RET_VAL] tac    : \"%04X\"\n", (unsigned int)cellinfo.tac);
            if (cellinfo.valid & LTE_TIMEDIFFIDX_VALID) {
              APITEST_DBG_PRINT("[RET_VAL] time_diffidx    : \"%hu\"\n", cellinfo.time_diffidx);
            }

            if (cellinfo.valid & LTE_TIMEDIFFIDX_VALID) {
              APITEST_DBG_PRINT("[RET_VAL] ta    : \"%hu\"\n", cellinfo.ta);
            }

            if (cellinfo.valid & LTE_TA_VALID) {
              APITEST_DBG_PRINT("[RET_VAL] SFN    : \"%hu\"\n", cellinfo.sfn);
            }

            if (cellinfo.valid & LTE_RSRP_VALID) {
              APITEST_DBG_PRINT("[RET_VAL] rsrp    : \"%hd\"\n", cellinfo.rsrp);
            }

            if (cellinfo.valid & LTE_RSRQ_VALID) {
              APITEST_DBG_PRINT("[RET_VAL] rsrq    : \"%hd\"\n", cellinfo.rsrq);
            }

            APITEST_DBG_PRINT("[RET_VAL] neighbor_num    : \"%lu\"\n",
                              (uint32_t)cellinfo.neighbor_num);
            for (i = 0; i < cellinfo.neighbor_num; i++) {
              APITEST_DBG_PRINT("[RET_VAL] phycell_id : \"%lu\"\n",
                                cellinfo.neighbor_cell[i].cell_id);
              APITEST_DBG_PRINT("[RET_VAL] earfcn     : \"%lu\"\n",
                                cellinfo.neighbor_cell[i].earfcn);
              if (cellinfo.neighbor_cell[i].valid & LTE_SFN_VALID) {
                APITEST_DBG_PRINT("[RET_VAL] sfn[%d]    : \"%hu\"\n", i,
                                  cellinfo.neighbor_cell[i].sfn);
              }

              if (cellinfo.neighbor_cell[i].valid & LTE_RSRP_VALID) {
                APITEST_DBG_PRINT("[RET_VAL] rsrp[%d]    : \"%hd\"\n", i,
                                  cellinfo.neighbor_cell[i].rsrp);
              }

              if (cellinfo.neighbor_cell[i].valid & LTE_RSRQ_VALID) {
                APITEST_DBG_PRINT("[RET_VAL] rsrq[%d]    : \"%hd\"\n", i,
                                  cellinfo.neighbor_cell[i].rsrq);
              }
            }
          } else {
            APITEST_DBG_PRINT("[RET_VAL] result(%d), cellinfo.valid(0x%x)\n", (int)result,
                              (unsigned int)cellinfo.valid);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_quality */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtqty", strlen("gtqty"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_quality_t quality;

          print_retval(result = lte_get_quality(&quality), APITEST_GETFUNCNAME(lte_get_quality));
          if (LTE_RESULT_OK == result && LTE_VALID == quality.valid) {
            APITEST_DBG_PRINT("[RET_VAL] rsrp  : \"%hd\"\n", quality.rsrp);
            APITEST_DBG_PRINT("[RET_VAL] rsrq  : \"%hd\"\n", quality.rsrq);
            APITEST_DBG_PRINT("[RET_VAL] sinr  : \"%hd\"\n", quality.sinr);
            APITEST_DBG_PRINT("[RET_VAL] rssi  : \"%hd\"\n", quality.rssi);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_quality_2g */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gt2gqty", strlen("gt2gqty"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_quality_2g_t quality;

          print_retval(result = lte_get_quality_2g(&quality),
                       APITEST_GETFUNCNAME(lte_get_quality_2g));
          if (LTE_RESULT_OK == result && LTE_VALID == quality.valid) {
            APITEST_DBG_PRINT("[RET_VAL] rssi  : \"%hd\"\n", quality.rssi);
            APITEST_DBG_PRINT("[RET_VAL] ber  : \"%hd\"\n", quality.ber);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_active_psm */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtactpsm", strlen("gtactpsm"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          ltepsmactstate_e actpsm;

          print_retval(result = lte_get_active_psm(&actpsm),
                       APITEST_GETFUNCNAME(lte_get_active_psm));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] PSM activity state : %s\n",
                              LTE_PSMACTSTAT_INACTIVE == actpsm ? "Inactive" : "Active");
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_intercepted_scan */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stintrcptscn", strlen("stintrcptscn"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          lte_set_interruptedscan_t intrcptscn;
          intrcptscn.mode = atoi(command->argv[APITEST_CMD_PARAM_1]);
          intrcptscn.timer_value = atoi(command->argv[APITEST_CMD_PARAM_2]);
          print_retval(lte_set_intercepted_scan(&intrcptscn),
                       APITEST_GETFUNCNAME(lte_set_intercepted_scan));

        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          show_help("stintrcptscn");
          goto errout_with_param;
        }
      }

      /* lte_set_pdcp_discard_timer */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "pdcptmr", strlen("pdcptmr"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lte_set_pdcp_discard_timer_t timer;
          timer = atoi(command->argv[APITEST_CMD_PARAM_1]);
          print_retval(lte_set_pdcp_discard_timer(timer),
                       APITEST_GETFUNCNAME(lte_set_pdcp_discard_timer));

        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          show_help("pdcptmr");
          goto errout_with_param;
        }
      }

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "fwupgcmd", strlen("fwupgcmd"));
      if (cmp_res == 0) {
        if (5 == command->argc) {
          lte_fw_upgrade_cmd_t fwupgradecmd;
          fwupgradecmd.cmd = atoi(command->argv[APITEST_CMD_PARAM_1]);
          fwupgradecmd.confirmation = atoi(command->argv[APITEST_CMD_PARAM_2]);
          fwupgradecmd.result = atoi(command->argv[APITEST_CMD_PARAM_2]);
          print_retval(lte_fw_upgrade_cmd(&fwupgradecmd), APITEST_GETFUNCNAME(lte_fw_upgrade_cmd));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtfwupgstate", strlen("gtfwupgstate"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_fw_upgrade_state_t fwupgradestate;

          print_retval(result = lte_get_fw_upgrade_state(&fwupgradestate),
                       APITEST_GETFUNCNAME(lte_get_fw_upgrade_state));
          if (LTE_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] FW upgrade state : %d\n", fwupgradestate.state);
            APITEST_DBG_PRINT("[RET_VAL] FW upgrade image_downloaded_size : %lu\n",
                              fwupgradestate.image_downloaded_size);
            APITEST_DBG_PRINT("[RET_VAL] FW upgrade image_total_size : %lu\n",
                              fwupgradestate.image_total_size);
            APITEST_DBG_PRINT("[RET_VAL] FW upgrade fota_result : %d\n",
                              fwupgradestate.fota_result);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_pdn_activation */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rppdnact", strlen("rppdnact"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_pdn_activation(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_pdn_activation));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_pdn_activation(pdn_actevt_report_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_pdn_activation));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_cellinfo_2g */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "strp2gclinf", strlen("strp2gclinf"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint32_t period;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_INVALID_ARG,
                           strlen(APITEST_INVALID_ARG))) {
            period = 0;
          } else {
            period =
                (uint32_t)strtol(command->argv[APITEST_CMD_PARAM_1], NULL, APITEST_STRTOL_BASE);
          }

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_cellinfo_2g(NULL, period, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_cellinfo_2g));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_cellinfo_2g(clinfrepo_2g_cb, period, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_cellinfo_2g));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_2]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_cellinfo_2g */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gt2gcellinfo", strlen("gt2gcellinfo"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          lteresult_e result;
          lte_cellinfo_2g_t cellinfo;
          int i, j;

          memset(&cellinfo, 0x0, sizeof(lte_cellinfo_2g_t));
          print_retval(result = lte_get_cellinfo_2g(&cellinfo),
                       APITEST_GETFUNCNAME(lte_get_cellinfo_2g));
          if (LTE_RESULT_OK == result && (cellinfo.serv_cell.valid & LTE_VALID)) {
            for (i = 0; i < LTE_CELLINFO_MCC_DIGIT; i++) {
              APITEST_DBG_PRINT("[RET_VAL] mcc[%d]    : \"%d\"\n", i, cellinfo.serv_cell.mcc[i]);
            }

            APITEST_DBG_PRINT("[RET_VAL] mnc_digit  : \"%d\"\n", cellinfo.serv_cell.mnc_digit);

            for (i = 0; i < cellinfo.serv_cell.mnc_digit; i++) {
              APITEST_DBG_PRINT("[RET_VAL] mnc[%d]    : \"%d\"\n", i, cellinfo.serv_cell.mnc[i]);
            }

            APITEST_DBG_PRINT("[RET_VAL] lac: \"%lu\"\n", cellinfo.serv_cell.lac);
            APITEST_DBG_PRINT("[RET_VAL] cell_id: \"%lu\"\n", cellinfo.serv_cell.cell_id);
            APITEST_DBG_PRINT("[RET_VAL] bs_id: \"%lu\"\n", cellinfo.serv_cell.bs_id);
            APITEST_DBG_PRINT("[RET_VAL] arfcn: \"%lu\"\n", cellinfo.serv_cell.arfcn);
            APITEST_DBG_PRINT("[RET_VAL] rx_level: \"%lu\"\n", cellinfo.serv_cell.rx_level);
            APITEST_DBG_PRINT("[RET_VAL] t_adv: \"%hu\"\n", cellinfo.serv_cell.t_adv);

            APITEST_DBG_PRINT("[RET_VAL] neighbor_num: \"%lu\"\n", (uint32_t)cellinfo.neighbor_num);
            for (i = 0; i < cellinfo.neighbor_num; i++) {
              if (cellinfo.neighbor_cell[i].valid & LTE_VALID) {
                for (j = 0; j < LTE_CELLINFO_MCC_DIGIT; j++) {
                  APITEST_DBG_PRINT("[RET_VAL] mcc[%d]    : \"%d\"\n", j,
                                    cellinfo.neighbor_cell[i].mcc[j]);
                }

                APITEST_DBG_PRINT("[RET_VAL] mnc_digit  : \"%d\"\n",
                                  cellinfo.neighbor_cell[i].mnc_digit);

                for (j = 0; j < cellinfo.neighbor_cell[i].mnc_digit; j++) {
                  APITEST_DBG_PRINT("[RET_VAL] mnc[%d]    : \"%d\"\n", j,
                                    cellinfo.neighbor_cell[i].mnc[j]);
                }

                APITEST_DBG_PRINT("[RET_VAL] lac : \"%lu\"\n", cellinfo.neighbor_cell[i].lac);
                APITEST_DBG_PRINT("[RET_VAL] cell_id: \"%lu\"\n",
                                  cellinfo.neighbor_cell[i].cell_id);
                APITEST_DBG_PRINT("[RET_VAL] bs_id: \"%lu\"\n", cellinfo.neighbor_cell[i].bs_id);
                APITEST_DBG_PRINT("[RET_VAL] arfcn: \"%lu\"\n", cellinfo.neighbor_cell[i].arfcn);
                APITEST_DBG_PRINT("[RET_VAL] rx_level: \"%lu\"\n",
                                  cellinfo.neighbor_cell[i].rx_level);
              }
            }
          } else {
            APITEST_DBG_PRINT("[RET_VAL] result(%d), cellinfo.serv_cell.valid(0x%x)\n", (int)result,
                              (unsigned int)cellinfo.serv_cell.valid);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

#if 0
    /* lte_get_scan_scheme */

    cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gtscansch", strlen("gtscansch"));
    if (cmp_res == 0) {
      if (2 == command->argc) {
        lteresult_e result;
        lte_scan_scheme_t scheme;

        print_retval(result = lte_get_scan_scheme(&scheme),
                     APITEST_GETFUNCNAME(lte_get_scan_scheme));
        if (LTE_RESULT_OK == result) {
          APITEST_DBG_PRINT(
              "[RET_VAL] Scan scheme\n"
              "num_mru_scans: %hu\n"
              "num_country_scans: %hu\n"
              "num_region_scans: %hu\n"
              "num_full_scans: %lu\n"
              "fallback_full_to_country: %lu\n"
              "slptime_btwn_scans: %lu\n"
              "max_slptime_btwn_scans: %lu\n"
              "slptime_step: %hu\n",
              scheme.num_mru_scans, scheme.num_country_scans, scheme.num_region_scans,
              (uint32_t)scheme.num_full_scans, (uint32_t)scheme.fallback_full_to_country,
              scheme.slptime_btwn_scans, scheme.max_slptime_btwn_scans, scheme.slptime_step);
        }
      } else {
        APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
        goto errout_with_param;
      }
    }
#endif
      /* lte_set_scan_scheme */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "stscansch", strlen("stscansch"));
      if (cmp_res == 0) {
        if (10 == command->argc) {
          lte_scan_scheme_t scheme;

          scheme.num_mru_scans = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          scheme.num_country_scans = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_2]);
          scheme.num_region_scans = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_3]);
          scheme.num_full_scans = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_4]);
          scheme.fallback_full_to_country = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_5]);
          scheme.slptime_btwn_scans = (uint32_t)atoi(command->argv[APITEST_CMD_PARAM_6]);
          scheme.max_slptime_btwn_scans = (uint32_t)atoi(command->argv[APITEST_CMD_PARAM_7]);
          scheme.slptime_step = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_8]);
          print_retval(lte_set_scan_scheme(&scheme), APITEST_GETFUNCNAME(lte_set_scan_scheme));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_set_report_scanresult */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rpscan", strlen("rpscan"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_scanresult(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_scanresult));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_scanresult(scanresult_report_cb, (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_scanresult));
          } else {
            APITEST_DBG_PRINT("invalid arg %s\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lwm2m_lte_fw_upgrade_evt */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "rplwm2mfwupgevt", strlen("rpfwupgevt"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_DISABLE_ARG,
                           strlen(APITEST_DISABLE_ARG))) {
            print_retval(lte_set_report_lwm2m_fw_upgrade_evt(NULL, NULL),
                         APITEST_GETFUNCNAME(lte_set_report_lwm2m_fw_upgrade_evt));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_1], APITEST_ENABLE_ARG,
                                  strlen(APITEST_ENABLE_ARG))) {
            print_retval(lte_set_report_lwm2m_fw_upgrade_evt(lwm2m_fw_upgrade_evt_report_cb,
                                                             (void *)0xdeadbeef),
                         APITEST_GETFUNCNAME(lte_set_report_lwm2m_fw_upgrade_evt));
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "appready", strlen("appready"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          altcom_app_ready();
          print_retval(0, APITEST_GETFUNCNAME(altcom_app_ready));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_pdninfo */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "pdninfo", strlen("pdninfo"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          uint32_t cid;
          lte_pdn_info_t info;

          memset((void *)&info, 0, sizeof(lte_pdn_info_t));
          cid = (uint32_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          result = lte_get_pdninfo(cid, &info);
          print_retval(result != LTE_RESULT_OK, APITEST_GETFUNCNAME(lte_get_pdninfo));
          if (result == LTE_RESULT_OK) {
            uint32_t i;

            APITEST_DBG_PRINT("\n====Active PDN Information====\n");
            for (i = 0; i < 2; i++) {
              if (info.pdn[i].is_valid) {
                APITEST_DBG_PRINT("ip_type: %s\n",
                                  info.pdn[i].ip_type == LTE_APN_IPTYPE_IP ? "IPv4" : "IPv6");
                APITEST_DBG_PRINT("cid: %lu\n", info.pdn[i].cid);
                APITEST_DBG_PRINT("bearer_id: %lu\n", info.pdn[i].bearer_id);
                APITEST_DBG_PRINT("apn_name: %s\n", info.pdn[i].apn_name);
                APITEST_DBG_PRINT("ip_str: %s\n", info.pdn[i].ip_str);
                APITEST_DBG_PRINT("nmask_str: %s\n", info.pdn[i].nmask_str);
                APITEST_DBG_PRINT("gw_str: %s\n", info.pdn[i].gw_str);
                APITEST_DBG_PRINT("pri_dns_str: %s\n", info.pdn[i].pri_dns_str);
                APITEST_DBG_PRINT("sec_dns_str: %s\n\n", info.pdn[i].sec_dns_str);
              }
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* lte_get_pdpaddr */
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "pdpaddr", strlen("pdpaddr"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          lteresult_e result;
          uint32_t cid;
          lte_pdp_addr_t pdp;

          memset((void *)&pdp, 0, sizeof(lte_pdp_addr_t));
          cid = (uint32_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          result = lte_get_pdp_addr(cid, &pdp);
          print_retval(result != LTE_RESULT_OK, APITEST_GETFUNCNAME(lte_get_pdp_addr));
          if (result == LTE_RESULT_OK) {
            uint32_t i;

            APITEST_DBG_PRINT("\n====Addrees of PDP Context====\n");
            APITEST_DBG_PRINT("cid: %lu\n", pdp.cid);
            for (i = 0; i < 2; i++) {
              if (pdp.addr[i].is_valid) {
                APITEST_DBG_PRINT("ip_type: %s\n",
                                  pdp.addr[i].ip_type == LTE_APN_IPTYPE_IP ? "IPv4" : "IPv6");
                APITEST_DBG_PRINT("ip_str: %s\n", pdp.addr[i].ip_str);
              }
            }
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }
    }

  errout_with_param:
    if (command) {
      APITEST_FREE_CMD(command, command->argc);
      command = NULL;
    }
  }

  osThreadExit();
  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void apitest_log_lock(void) {
  if (app_log_mtx) {
    osMutexAcquire(app_log_mtx, osWaitForever);
  }
}

void apitest_log_unlock(void) {
  if (app_log_mtx) {
    osMutexRelease(app_log_mtx);
  }
}

int32_t apitest_init(void) {
  if (!app_init) {
    app_log_mtx = osMutexNew(NULL);
    if (!app_log_mtx) {
      APITEST_DBG_PRINT("app_log_mtx init failed\n");
      goto errout;
    }

    cmdq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(struct apitest_command_s *), NULL);
    if (!cmdq) {
      APITEST_DBG_PRINT("cmdq init failed\n");
      goto errout;
    }
  } else {
    APITEST_DBG_PRINT("App already initialized\n");
  }

  app_init = true;
  return 0;

errout:
  if (app_log_mtx) {
    osMutexDelete(app_log_mtx);
    app_log_mtx = NULL;
  }

  if (cmdq) {
    osMessageQueueDelete(cmdq);
    cmdq = NULL;
  }

  return -1;
}

int32_t apitest_main(int32_t argc, char *argv[]) {
  struct apitest_command_s *command;
  int32_t itr = 0;

  if (!app_init) {
    APITEST_DBG_PRINT("App not yet initialized\n");
    return -1;
  }

  if (2 <= argc) {
    if (APITEST_MAX_NUMBER_OF_PARAM < argc) {
      APITEST_DBG_PRINT("too many arguments\n");
      return -1;
    }

    if ((command = malloc(sizeof(struct apitest_command_s))) == NULL) {
      APITEST_DBG_PRINT("malloc fail\n");
      return -1;
    }

    memset(command, 0, sizeof(struct apitest_command_s));
    command->argc = argc;

    for (itr = 0; itr < argc; itr++) {
      if ((command->argv[itr] = malloc(strlen(argv[itr]) + 1)) == NULL) {
        APITEST_FREE_CMD(command, itr);
        return -1;
      }

      memset(command->argv[itr], '\0', (strlen(argv[itr]) + 1));
      strncpy((command->argv[itr]), argv[itr], strlen(argv[itr]));
    }

    if (!taskrunning) {
      osThreadAttr_t attr = {0};

      attr.name = "apitest";
      attr.stack_size = APITEST_TASK_STACKSIZE;
      attr.priority = APITEST_TASK_PRI;

      osThreadId_t tid = osThreadNew(apitest_task, NULL, &attr);

      if (!tid) {
        APITEST_DBG_PRINT("osThreadNew failed\n");
        APITEST_FREE_CMD(command, command->argc);
        return -1;
      }
    }

    if (osOK != osMessageQueuePut(cmdq, (const void *)&command, 0, osWaitForever)) {
      APITEST_DBG_PRINT("osMessageQueuePut to apitest_task Failed!!\n");
      APITEST_FREE_CMD(command, command->argc);
    }
  }

  return 0;
}
