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

#include "altcom.h"
#include "dbg_if.h"
#include "buffpoolwrapper.h"
#include "wrkrid.h"
#include "thrdfctry.h"
#include "evtdisp.h"
#include "apicmd.h"
#include "altcombuilder.h"
#include "apiutil.h"
#ifdef HAL_UART_ALT125X_MCU
#include "hal_uart_alt125x.h"
#elif defined(HAL_EMUX_ALT125X)
#include "hal_emux_alt125x.h"
#elif defined(HAL_UART_NXP)
#include "hal_uart_nxp.h"
#elif defined(HAL_EMUX_NXP)
#include "hal_emux_nxp.h"
#endif
#include "apicmdgw.h"
#include "apicmdhdlr_errind.h"
#ifdef __ENABLE_COAP_API__
#include "apicmdhdlr_cmdurcevt.h"
#endif
#ifdef __ENABLE_LTE_API__
#include "apicmdhdlr_attachnet.h"
#include "apicmdhdlr_dataoff.h"
#include "apicmdhdlr_dataon.h"
#include "apicmdhdlr_detachnet.h"
#include "apicmdhdlr_repcellinfo.h"
#include "apicmdhdlr_repevt.h"
#include "apicmdhdlr_repnetstat.h"
#include "apicmdhdlr_repquality.h"
#include "apicmdhdlr_reptimerevt.h"
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_ATCMD_API__
#include "apicmdhdlr_atcmdUrcEvt.h"
#endif /* __ENABLE_ATCMD_API__ */
#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
#include "apicmdhdlr_mqttMessageEvt.h"
#endif /* defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__) */
#ifdef __ENABLE_GPS_API__
#include "apicmdhdlr_nmearepevt.h"
#endif /* __ENABLE_GPS_API__ */
#ifdef __ENABLE_LWM2M_API__
#include "apicmdhdlr_lwm2mevt.h"
#endif /* __ENABLE_LWM2M_API__ */
#ifdef __ENABLE_ATSOCKET_API__
#include "apicmdhdlr_atsocketevt.h"
#endif /* __ENABLE_ATSOCKET_API__ */
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICALLBACK_THRD_STACKSIZE (4096)
#define APICALLBACK_THRD_PRIO ALTCOM_SYS_TASK_PRIO_NORMAL
#define APICALLBACK_THRD_NUM (1)
#define APICALLBACK_THRD_QNUM (16) /* tentative */
#define BLOCKSETLIST_NUM (sizeof(g_blk_settings) / sizeof(g_blk_settings[0]))
#define THRDSETLIST_NUM (1)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static CODE int32_t lte_buildmain(FAR void *arg);
static CODE int32_t lte_destroy(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct buffpool_blockset_s g_blk_settings[] = {{16, 16}, {32, 12},  {128, 5}, {256, 4},
                                                      {512, 2}, {2064, 1}, {3200, 3}};

static struct evtdisp_s *g_evtdips_obj;

static evthdl_if_t g_apicmdhdlrs[] = {
#ifdef __ENABLE_LTE_API__
    apicmdhdlr_repcellinfo,     apicmdhdlr_repquality,
    apicmdhdlr_attachnet,       apicmdhdlr_detachnet,
    apicmdhdlr_dataon,          apicmdhdlr_dataoff,
    apicmdhdlr_repnetstat,      apicmdhdlr_repevt,
    apicmdhdlr_reptimerevt,
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_COAP_API__
    apicmdhdlr_cmdevt,          apirsthdlr_cmdevt,
    apitermhdlr_cmdevt,
#endif /* __ENABLE_COAP_API__ */
#ifdef __ENABLE_ATCMD_API__
    apicmdhdlr_atcmdUrcEvt,
#endif /* __ENABLE_ATCMD_API__ */

#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
    apicmdhdlr_mqttMessageEvt,
#endif /* defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__) */

#ifdef __ENABLE_GPS_API__
    apicmdhdlr_nmearepevt,
#endif /* __ENABLE_GPS_API__ */

#ifdef __ENABLE_LWM2M_API__
    apicmdhdlr_lwm2mMessageEvt,
#endif /* __ENABLE_LWM2M_API__ */
#ifdef __ENABLE_ATSOCKET_API__
    apicmdhdlr_atsocketevt,
#endif /* __ENABLE_ATSOCKET_API__ */
    apicmdhdlr_errindication,   EVTDISP_EVTHDLLIST_TERMINATION};

static enum apiCmdId g_postponable_evt_list[] = {
#ifdef __ENABLE_LTE_API__
    APICMDID_REPORT_CELLINFO,
    APICMDID_REPORT_QUALITY,
    (enum apiCmdId)APICMDID_CONVERT_RES(APICMDID_ATTACH_NET),
    (enum apiCmdId)APICMDID_CONVERT_RES(APICMDID_DETACH_NET),
    (enum apiCmdId)APICMDID_CONVERT_RES(APICMDID_DATAON),
    (enum apiCmdId)APICMDID_CONVERT_RES(APICMDID_DATAOFF),
    APICMDID_REPORT_NETSTAT,
    APICMDID_REPORT_EVT,
    APICMDID_REPORT_TIMER_EVT,
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_COAP_API__
    APICMDID_COAP_CMD_URC,
    APICMDID_COAP_TERM_URC,
    APICMDID_COAP_RST_URC,
#endif
#ifdef __ENABLE_ATCMD_API__
    APICMDID_ATCMDCONN_URCEVT,
#endif /* __ENABLE_ATCMD_API__ */

#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
    APICMDID_MQTT_MESSAGEEVT,
#endif /* defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__) */

#ifdef __ENABLE_GPS_API__
    APICMDID_GPS_IGNSSEVU,
#endif /* __ENABLE_GPS_API__ */

#ifdef __ENABLE_LWM2M_API__
    APICMDID_LWM2M_URC,
#endif /* __ENABLE_LWM2M_API__ */

    APICMDID_ERRIND /* Termination item */
};

static FAR struct hal_if_s *g_halif;
static int local_logmtx = 0;

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct builder_if_s g_altcombuilder = {.buildmain = lte_buildmain,
                                       .buildsub1 = NULL,
                                       .buildsub2 = NULL,
                                       .buildsub3 = NULL,
                                       .destroy = lte_destroy};

extern int32_t modem_powerctrl(bool on);

#ifdef __ENABLE_LTE_API__
extern void lte_callback_init(void);
#endif
#ifdef __ENABLE_GPS_API__
extern void gps_callback_init(void);
#endif
#ifdef __ENABLE_COAP_API__
extern void coap_callback_init(void);
#endif

struct apicmd_moduleinfo_s g_moduleInfo[] = {
#ifdef __ENABLE_COAP_API__
    {APIMODULEID_COAP, COAPVER_MAJOR, COAPVER_MINOR},
#endif /* __ENABLE_COAP_API__ */
#ifdef __ENABLE_LTE_API__
    {APIMODULEID_LTE, LTEVER_MAJOR, LTEVER_MINOR},
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_SOCKET_API__
    {APIMODULEID_SOCKET, SOCKETVER_MAJOR, SOCKETVER_MINOR},
#endif /* __ENABLE_SOCKET_API__ */
#ifdef __ENABLE_ATCMD_API__
    {APIMODULEID_ATCMD, ATCMDVER_MAJOR, ATCMDVER_MINOR},
#endif /* __ENABLE_ATCMD_API__ */
#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
    {APIMODULEID_MQTT, MQTTVER_MAJOR, MQTTVER_MINOR},
#endif /* __ENABLE_MQTT_API__ || __ENABLE_AWS_API__ */
#ifdef __ENABLE_GPS_API__
    {APIMODULEID_GPS, GPSVER_MAJOR, GPSVER_MINOR},
#endif /* __ENABLE_GPS_API__ */
#ifdef __ENABLE_CERTMGMT_API__
    {APIMODULEID_CERTMGMT, CERTMGMTVER_MAJOR, CERTMGMTVER_MINOR},
#endif /* __ENABLE_CERTMGMT_API__ */
#ifdef __ENABLE_LWM2M_API__
    {APIMODULEID_LWM2M, LWM2MVER_MAJOR, LWM2MVER_MINOR},
#endif /* __ENABLE_LWM2M_API__ */
#ifdef __ENABLE_MISC_API__
    {APIMODULEID_MISC, MISCVER_MAJOR, MISCVER_MINOR},
#endif /* __ENABLE_MISC_API__ */
};

const uint8_t g_moduleInfoCnt = (sizeof(g_moduleInfo) / sizeof(struct apicmd_moduleinfo_s));
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bufferpool_initialize
 *
 * Description:
 *   Initialize buffer pool for use.
 *
 * Input Parameters:
 *   [in] blkCfg Block set configuration
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t bufferpool_initialize(blockcfg_t *blkCfg) {
  int32_t ret = -1;
  FAR struct buffpool_blockset_s *pset;
  uint8_t psetNum;

  pset = NULL != blkCfg->blksetCfg ? (FAR struct buffpool_blockset_s *)blkCfg->blksetCfg
                                   : &g_blk_settings[0];
  psetNum = NULL != blkCfg->blksetCfg ? blkCfg->blksetNum : BLOCKSETLIST_NUM;
  DBGIF_LOG1_NORMAL("Use %s block configuration.\n",
                    NULL != blkCfg->blksetCfg ? "application" : "default");

  if (pset->size) {
    ret = buffpoolwrapper_init(pset, psetNum);
    if (0 > ret) {
      DBGIF_LOG1_ERROR("buffpoolwrapper_init() error :%ld.\n", ret);
    }
  } else {
    DBGIF_LOG_ERROR("Invalid blockset size\n");
  }

  return ret;
}

/****************************************************************************
 * Name: bufferpool_uninitialize
 *
 * Description:
 *   Uninitialize buffer pool.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t bufferpool_uninitialize(void) {
  int32_t ret;

  ret = buffpoolwrapper_fin();
  if (0 > ret) {
    DBGIF_LOG1_ERROR("buffpoolwrapper_fin() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: workerthread_initialize
 *
 * Description:
 *   Initialize worker thread for use.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t workerthread_initialize(void) {
  int32_t ret;
  struct thrdfctry_thrdset_s settings;

  /* worker thread settings for API callback */

  settings.id = WRKRID_API_CALLBACK_THREAD;
  settings.type = THRDFCTRY_SEQUENTIAL;
  settings.u.seqset.thrdstacksize = APICALLBACK_THRD_STACKSIZE;
  settings.u.seqset.thrdpriority = APICALLBACK_THRD_PRIO;
  settings.u.seqset.maxquenum = APICALLBACK_THRD_QNUM;

  ret = thrdfctry_init(&settings, THRDSETLIST_NUM);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("thrdfctry_init() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: workerthread_uninitialize
 *
 * Description:
 *   Uninitialize worker thread.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t workerthread_uninitialize(void) {
  int32_t ret;

  ret = thrdfctry_fin();
  if (0 > ret) {
    DBGIF_LOG1_ERROR("thrdfctry_fin() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: eventdispatcher_initialize
 *
 * Description:
 *   Initialize event dispatcher for use.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t eventdispatcher_initialize(void) {
  int ret = 0;

  g_evtdips_obj = evtdisp_create(g_apicmdhdlrs);
  if (!g_evtdips_obj) {
    DBGIF_LOG_ERROR("evtdisp_create() error.\n");
    ret = -1;
  }

  return ret;
}

/****************************************************************************
 * Name: eventdispatcher_uninitialize
 *
 * Description:
 *   Uninitialize event dispatcher.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t eventdispatcher_uninitialize(void) {
  int32_t ret;

  ret = evtdisp_delete(g_evtdips_obj);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("evtdisp_delete() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: hal_initialize
 *
 * Description:
 *   Initialize HAL I/F for use.
 *
 * Input Parameters:
 *   [in] halCfg HAL configuration.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t hal_initialize(halcfg_t *halCfg) {
  int32_t ret = 0;

  if (!halCfg) {
    return -1;
  }

  g_halif = NULL;
  switch (halCfg->halType) {
    case ALTCOM_HAL_INT_UART:
#ifdef HAL_UART_ALT125X_MCU
      g_halif = hal_uart_alt125x_create();
#else
      DBGIF_LOG1_ERROR("Incorrect Compile/Run time configuration, type %lu.\n",
                       (uint32_t)halCfg->halType);
#endif
      break;

    case ALTCOM_HAL_INT_EMUX:
#ifdef HAL_EMUX_ALT125X
      g_halif = hal_emux_alt125x_create(halCfg->virtPortId);
#else
      DBGIF_LOG1_ERROR("Incorrect Compile/Run time configuration, type %lu.\n",
                       (uint32_t)halCfg->halType);
#endif
      break;

    case ALTCOM_HAL_EXT_UART:
#ifdef HAL_UART_NXP
      g_halif = hal_uart_nxp_create();
#else
      DBGIF_LOG1_ERROR("Incorrect Compile/Run time configuration, type %lu.\n",
                       (uint32_t)halCfg->halType);
#endif
      break;

    case ALTCOM_HAL_EXT_EMUX:
#ifdef HAL_EMUX_NXP
      g_halif = hal_emux_nxp_create(halCfg->virtPortId);
#else
      DBGIF_LOG1_ERROR("Incorrect Compile/Run time configuration, type %lu.\n",
                       (uint32_t)halCfg->halType);
#endif
      break;

    default:
      DBGIF_LOG1_ERROR("Unsupported HAL type %lu.\n", (uint32_t)halCfg->halType);
      break;
  }

  if (!g_halif) {
    DBGIF_LOG_ERROR("HAL create error.\n");
    ret = -1;
  }

  return ret;
}

/****************************************************************************
 * Name: hal_uninitialize
 *
 * Description:
 *   Uninitialize HAL I/F.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t hal_uninitialize(void) {
  int32_t ret = -1;

#ifdef HAL_UART_ALT125X_MCU
  ret = hal_uart_alt125x_delete(g_halif);
#elif defined(HAL_EMUX_ALT125X)
  ret = hal_emux_alt125x_delete(g_halif);
#elif defined(HAL_UART_NXP)
  ret = hal_uart_nxp_delete(g_halif);
#elif defined(HAL_EMUX_NXP)
  ret = hal_emux_nxp_delete(g_halif);
#endif
  if (0 > ret) {
    DBGIF_LOG1_ERROR("HAL delete error :%ld.\n", ret);
    ret = -1;
  }

  return ret;
}

/****************************************************************************
 * Name: apicmdgw_initialize
 *
 * Description:
 *   Initialize api command gateway for use.
 *
 * Input Parameters:
 *   is_postpone_evt: Indicate if events need to be postponed before application ready.
 *   is_bypass_echo: Indicate if echo command can be bypassed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t apicmdgw_initialize(int is_postpone_evt, int is_bypass_echo) {
  int32_t ret;
  struct apicmdgw_set_s set;

  if (!g_evtdips_obj || !g_halif) {
    DBGIF_LOG2_ERROR("g_evtdips_obj: %p, g_halif: %p.\n", (void *)g_evtdips_obj, (void *)g_halif);
    return -1;
  }

  set.dispatcher = g_evtdips_obj;
  set.halif = g_halif;
  set.postponable_evt_list = is_postpone_evt ? g_postponable_evt_list : NULL;
  set.bypass_echo = is_bypass_echo;
  ret = apicmdgw_init(&set);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_init() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: apicmdgw_uninitialize
 *
 * Description:
 *   Uninitialize api command gateway.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static int32_t apicmdgw_uninitialize(void) {
  int32_t ret;

  ret = apicmdgw_fin();
  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_init() error :%ld.\n", ret);
  }

  return ret;
}

/****************************************************************************
 * Name: lte_buildmain
 *
 * Description:
 *   Initialize the resource which is the mandatory used in the library.
 *
 * Input Parameters:
 *   arg  Arguments required for library initialization.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static CODE int32_t lte_buildmain(FAR void *arg) {
  int32_t ret;
  altcom_sys_cremtx_s mtx_param;
  altcom_init_t *initCfg;

  if (!arg) {
    return -1;
  }

  initCfg = (altcom_init_t *)arg;
  altcom_SetCallbackReg(initCfg->is_cbreg_only_until_appready);

#ifdef __ENABLE_LTE_API__
  lte_callback_init();
#endif

#ifdef __ENABLE_GPS_API__
  gps_callback_init();
#endif

#ifdef __ENABLE_COAP_API__
  coap_callback_init();
#endif

  if (initCfg->altcom_log_lock) {
    DBGIF_LOG_NORMAL("Use application log lock.\n");
    DbgIf_SetLogMtx(initCfg->altcom_log_lock);
  }

  if (!DbgIf_GetLogMtx()) {
    altcom_sys_mutex_t mtx;

    ret = altcom_sys_create_mutex(&mtx, &mtx_param);
    if (ret < 0) {
      goto errout;
    }

    DBGIF_LOG_NORMAL("Use default log lock.\n");
    DbgIf_SetLogMtx(mtx);
    local_logmtx = 1;
  }

  DBGIF_LOG1_NORMAL("ALTCOM log level %lu.\n", initCfg->dbgLevel);
  DbgIf_SetLogLevel(initCfg->dbgLevel);

  ret = bufferpool_initialize(&initCfg->blkCfg);
  if (ret < 0) {
    goto errout_with_logmutex;
  }

  ret = workerthread_initialize();
  if (ret < 0) {
    goto errout_with_buffpl;
  }

  ret = eventdispatcher_initialize();
  if (ret < 0) {
    goto errout_with_workerthread;
  }

  ret = hal_initialize(&initCfg->halCfg);
  if (ret < 0) {
    goto errout_with_evtdispatcher;
  }

  ret = apicmdgw_initialize(initCfg->is_postpone_evt_until_appready,
                            initCfg->is_cbreg_only_until_appready);
  if (ret < 0) {
    goto errout_with_hal;
  }

  return 0;

errout_with_hal:
  (void)hal_uninitialize();

errout_with_evtdispatcher:
  (void)eventdispatcher_uninitialize();

errout_with_workerthread:
  (void)workerthread_uninitialize();

errout_with_buffpl:
  (void)bufferpool_uninitialize();

errout_with_logmutex:
  if (local_logmtx) {
    altcom_sys_mutex_t mtx;

    mtx = DbgIf_GetLogMtx();
    (void)altcom_sys_delete_mutex(&mtx);
    DbgIf_SetLogMtx(NULL);
    local_logmtx = 0;
  }
errout:
  return ret;
}

/****************************************************************************
 * Name: lte_destroy
 *
 * Description:
 *   Uninitalize the resources used in the library.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

static CODE int32_t lte_destroy(void) {
  int32_t ret;

  ret = apicmdgw_uninitialize();
  if (ret < 0) {
    return ret;
  }

  ret = hal_uninitialize();
  if (ret < 0) {
    return ret;
  }

  ret = eventdispatcher_uninitialize();
  if (ret < 0) {
    return ret;
  }

  ret = workerthread_uninitialize();
  if (ret < 0) {
    return ret;
  }

  ret = bufferpool_uninitialize();
  if (ret < 0) {
    return ret;
  }

  if (local_logmtx) {
    altcom_sys_mutex_t mtx;

    mtx = DbgIf_GetLogMtx();
    ret = altcom_sys_delete_mutex(&mtx);
    if (ret < 0) {
      return ret;
    }

    DbgIf_SetLogMtx(NULL);
    local_logmtx = 0;
  }

#ifdef __ENABLE_COAP_API__
  coap_callback_init();
#endif

#ifdef __ENABLE_LTE_API__
  lte_callback_init();
#endif

#ifdef __ENABLE_GPS_API__
  gps_callback_init();
#endif

  return 0;
}
