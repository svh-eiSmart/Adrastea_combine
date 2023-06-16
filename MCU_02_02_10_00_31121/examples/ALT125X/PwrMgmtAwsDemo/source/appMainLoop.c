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
/* General Header */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* Peripheral Header */
#include "DRV_GPIO.h"

/* App Header */
#include "altcom.h"
#include "lte/lte_api.h"
#include "aws/altcom_aws.h"
#include "gps/altcom_gps.h"

/* Retention DB Header */
#include "kvpfs_api.h"

/* Power Manager Header */
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "sleep_mngr.h"
#include "sleep_notify.h"

/* App includes */
#include "appMainLoop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* MISC */
#define MAX_TIME_VALID_PERIOD (600)
#define DEFAULT_SLEEP_TIMEOUT_MS (15000)
#define GNSS_TIMEOUT_MS (600000)
#define MAX_DBC_TIMEOUT_MS (100)
#define BTN_GPIO_NUM (MCU_GPIO_ID_02)
#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

/* Main loop */
#define APPMAINLOOP_TASK_STACKSIZE (2048)
#define APPMAINLOOP_TASK_PRI (osPriorityNormal)
#define MAX_APPMAINLOOP_QUENUM 1

/* Debugging Message */
#define LOCK() appMainLoop_log_lock()
#define UNLOCK() appMainLoop_log_unlock()
#define APP_DBG_PRINT(format, ...)                                    \
  do {                                                                \
    LOCK();                                                           \
    printf("[%s:%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    UNLOCK();                                                         \
  } while (0)

#define APP_ASSERT(condition, format, ...)                          \
  do {                                                              \
    if (true != (condition)) {                                      \
      APP_DBG_PRINT("##We ASSERT HERE!!## " format, ##__VA_ARGS__); \
      while (1) {                                                   \
        continue;                                                   \
      };                                                            \
    }                                                               \
  } while (0)

#define APP_ASSERT_ISR(condition, format, ...)                                                    \
  do {                                                                                            \
    if (true != (condition)) {                                                                    \
      printf("[%s:%d] ##We ASSERT HERE(ISR)!!## " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      while (1) {                                                                                 \
        continue;                                                                                 \
      };                                                                                          \
    }                                                                                             \
  } while (0)

#define APP_TRACE()               \
  do {                            \
    APP_DBG_PRINT("APP_TRACE\n"); \
  } while (0)

#define TOSTR(x) #x

/* Event allocate & send helper */
#define EVT_SEND(ctx, eventId, param)                                                  \
  do {                                                                                 \
    evtSend((appMainLoopCtx_t *)ctx, (appMainLoopEvt_e)eventId, (void *)param, false); \
  } while (0)

#define EVT_SEND_ISR(ctx, eventId, param)                                             \
  do {                                                                                \
    evtSend((appMainLoopCtx_t *)ctx, (appMainLoopEvt_e)eventId, (void *)param, true); \
  } while (0)

/* Compilation */
#define APP_UNUSED(x) (void)(x)

/* Time related */
#if defined(__GNUC__)
#define GMTIME_R(time, tm) gmtime_r(time, tm)
#elif defined(__ICCARM__)
#define GMTIME_R(time, tm) gmtime_s(time, tm)
#else
#error time function not supported in this toolchain
#endif

/* AWS related */
#define AWS_ENDPOINTURL "a2u2j94jvpbpay-ats.iot.us-east-2.amazonaws.com"
#define AWS_CLIENTID "device1"
#define AWS_TOPIC "tracker/location"
#define MAX_AWS_STRLEN 64
#define AWS_PORT 8883

/* Reteintion Key-Value-Pair */
#define KVP_LTETIME_STR "LTE_TIME"
#define KVP_GPSLOCATION_STR "GPS_LOCATION"

/* Power related */
#define PWRAWSDEMO_SMNGR_DEVNAME_SYNC "PwrMgmt-AWS-Demo"

/* callback function used by sleep_notify_register_callback before
 * sleep or after wakeup according to pm_state*/
static int32_t pwrAwsDemo_sleep_notify(sleep_notify_state pm_state, PWR_MNGR_PwrMode pwr_mode, void *ptr_ctx);

/* enable more debug dump */
#define PWRMGMTAWS_PRINT_STM_STR 1
/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef enum {
  EVT_INIT = 0,
  EVT_EXPIRY,
  EVT_BTN,
  EVT_BTN_EXP,
  EVT_BTN_FALSE,
  EVT_LTE_START, /* 5 */
  EVT_LTE_ATCH,
  EVT_LTE_DTCH,
  EVT_TIME_READY,
  EVT_TIME_INVALID,
  EVT_GNSS_START, /* 10 */
  EVT_GNSS_READY,
  EVT_AWS_START,
  EVT_AWS_READY,
  EVT_AWS_FINISH,
  EVT_LEAVE, /* 15 */
  EVT_SWBTN,
  EVT_SLEEP,
  EVT_MAX
} appMainLoopEvt_e;

typedef enum {
  STATE_INIT = 0,
  STATE_IDLE,
  STATE_BTNDBC_WAIT,
  STATE_BTN_CHECK,
  STATE_LTE_ATCH_WAIT,
  STATE_LTE_ATCH, /* 5 */
  STATE_TIME_CHECK,
  STATE_LTE_DTCH_WAIT,
  STATE_LTE_DTCH,
  STATE_GNSS_WAIT,
  STATE_GNSS_COMPLETE, /* 10 */
  STATE_AWS_WAIT,
  STATE_AWS_CONNECTED,
  STATE_SLEEP,
  STATE_COMPLETE,
  STATE_MAX
} appMainLoopState_e;

#if (PWRMGMTAWS_PRINT_STM_STR == 1)
char evt_string[EVT_MAX][30] = {
  "INIT(0)",
  "EXPIRY(1)",
  "BTN(2)",
  "BTN_EXP(3)",
  "BTN_FALSE(4)",
  "LTE_START(5)", /* 5 */
  "LTE_ATCH(6)",
  "LTE_DTCH(7)",
  "TIME_READY(8)",
  "TIME_INVALID(9)",
  "GNSS_START(10)", /* 10 */
  "GNSS_READY(11)",
  "AWS_START(12)",
  "AWS_READY(13)",
  "AWS_FINISH(14)",
  "LEAVE(15)", /* 15 */
  "SWBTN(16)",
  "SLEEP(17)"
};

char state_string[STATE_MAX][30] = {
  "INIT(0)",
  "IDLE(1)",
  "BTNDBC_WAIT(2)",
  "BTN_CHECK(3)",
  "LTE_ATCH_WAIT(4)",
  "LTE_ATCH(5)", /* 5 */
  "TIME_CHECK(6)",
  "LTE_DTCH_WAIT(7)",
  "LTE_DTCH(8)",
  "GNSS_WAIT(9)",
  "GNSS_COMPLETE(10)", /* 10 */
  "AWS_WAIT(11)",
  "AWS_CONNECTED(12)",
  "SLEEP(13)",
  "COMPLETE(14)"
};
#endif

typedef appMainLoopState_e (*stateTransFunc_t)(void *pCtx, void *pParam);

typedef struct {
  double utc;
  double lat;
  char dirlat;
  double lon;
  char dirlon;
  int16_t numsv;
} GnssInfo_t;

typedef struct {
  appMainLoopState_e currState;
  osMessageQueueId_t evtQueue;
  osThreadId_t taskHandle;
  osTimerId_t sleepTimer;
  osTimerId_t btnTimer;
  stateTransFunc_t stateTransMatrix[STATE_MAX][EVT_MAX];
  int expiryTimeoutTable[STATE_MAX];
  time_t timeInfo;
  time_t *lastDoGNSStime;
  bool taskRunning;
} appMainLoopCtx_t;

typedef struct {
  appMainLoopEvt_e event;
  void *param;
} appMainLoopEvt_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static osMutexId_t app_log_mtx = NULL;
static appMainLoopCtx_t gAppMainLoopCtx;

uint32_t pwrawsdemo_smngr_sync_id;

bool useFakeGps = false;
GnssInfo_t fakeGnssInfo = {0.0, 2450.513400, 'N', 12101.142800, 'E', 0};
bool forceGpsRenew = false;

static time_t gLocalTime = 0;
static char gLocation[128] = {0};

static osMessageQueueId_t wakeq = NULL;

static bool gIoWake = false;
/****************************************************************************
 * Private Functions
 ****************************************************************************/
void appMainLoop_log_lock(void) {
  if (app_log_mtx) {
    osMutexAcquire(app_log_mtx, osWaitForever);
  }
}

void appMainLoop_log_unlock(void) {
  if (app_log_mtx) {
    osMutexRelease(app_log_mtx);
  }
}

static void evtSend(appMainLoopCtx_t *pCtx, appMainLoopEvt_e eventId, void *pParam, bool isISR) {
  appMainLoopEvt_t evt;

  evt.event = eventId;
  evt.param = pParam;

  if (isISR) {
    APP_ASSERT(osOK == osMessageQueuePut(pCtx->evtQueue, (const void *)&evt, 0, 0),
               "event send fail-1\n");
  } else {
    APP_ASSERT(osOK == osMessageQueuePut(pCtx->evtQueue, (const void *)&evt, 0, osWaitForever),
               "event send fail-2\n");
  }
}

static void sleep_timeout(osTimerId_t xTimer) { EVT_SEND(xTimer, EVT_EXPIRY, NULL); }

static void btn_timeout(osTimerId_t xTimer) { EVT_SEND(xTimer, EVT_BTN_EXP, BTN_GPIO_NUM); }

static void btn_interrupt_handler(void *user_param) {
  DRV_GPIO_DisableIrq(BTN_GPIO_NUM);
  EVT_SEND_ISR(user_param, EVT_BTN, BTN_GPIO_NUM);
}

static void atchnet_cb(lteresult_e result, lteerrcause_e errcause, void *pCtx) {
  APP_TRACE();
  if (LTE_RESULT_OK == result) {
    EVT_SEND(pCtx, EVT_LTE_ATCH, NULL);
  } else {
    APP_DBG_PRINT("result: %d, error: %d\n", (int)result, (int)errcause);
  }
}

static void dtchnet_cb(lteresult_e result, void *pCtx) {
  APP_TRACE();
  APP_ASSERT(LTE_RESULT_OK == result, "result: %lu,\n", (uint32_t)result);

  EVT_SEND(pCtx, EVT_LTE_DTCH, NULL);
}

void nmea_cb(gps_event_t *nmeaEvt, void *pCtx) {
  GnssInfo_t *gnssInfo;

  APP_ASSERT(EVENT_NMEA_TYPE == nmeaEvt->eventType, "Invalid eventType: %d,\n",
             (int)nmeaEvt->eventType);

  if (EVENT_NMEA_GGA_TYPE != nmeaEvt->nmeaType) {
    return;
  }

  if (nmeaEvt->u.gga.quality == 1 || useFakeGps) {
    gnssInfo = malloc(sizeof(GnssInfo_t));
    APP_ASSERT(NULL != gnssInfo, "[%s:%d]malloc failed!\n", __FUNCTION__, __LINE__);

    if (nmeaEvt->u.gga.quality == 1) {
      gnssInfo->utc = nmeaEvt->u.gga.UTC;
      gnssInfo->lat = nmeaEvt->u.gga.lat;
      gnssInfo->dirlat = nmeaEvt->u.gga.dirlat;
      gnssInfo->lon = nmeaEvt->u.gga.lon;
      gnssInfo->dirlon = nmeaEvt->u.gga.dirlon;
      gnssInfo->numsv = nmeaEvt->u.gga.numsv;
    } else {
      /* Use fake GPS data for quick demo purpose */
      memcpy(gnssInfo, &fakeGnssInfo, sizeof(GnssInfo_t));
    }

    EVT_SEND(pCtx, EVT_GNSS_READY, gnssInfo);
  } else {
    APP_DBG_PRINT("utc=%f \n", nmeaEvt->u.gga.UTC);
    /*
    APP_DBG_PRINT("lat=%f dirlat=%c\n",nmeagga->u.gga.lat,nmeagga->u.gga.dirlat);
    APP_DBG_PRINT("lon=%f dirlon=%c\n",nmeagga->u.gga.lon,nmeagga->u.gga.dirlon);
    APP_DBG_PRINT("sv=%d \n\n",nmeagga->u.gga.numsv);
    */
  }
}

static void dtchnet_dummycb(lteresult_e result, void *pCtx) {
  APP_UNUSED(pCtx);
  APP_TRACE();

  APP_ASSERT(LTE_RESULT_OK == result, "result: %lu,\n", (uint32_t)result);

  /* Post-action 3 before sleep */
  /* Turn off gps */
  /* altcom_gps_activate(GPSACT_GPSOFF, ACTIVATE_NO_MODE, ALTCOM_GPSACT_NOTOL);
  altcom_gps_setevent(EVENT_NMEA_TYPE, false, NULL, NULL); */
}

static void dtchnet_dummyInitcb(lteresult_e result, void *pCtx) { APP_TRACE(); }

static void awsConnFailureCb(void *pCtx) { APP_DBG_PRINT("Connection failure, pCtx:%p", pCtx); }

static void awsMessageCb(AWSMessageData *msgData, void *pCtx) {
  APP_DBG_PRINT("[QoS:%u] Receive topic: ==%.*s==\nmessage: ==%.*s==\npCtx:%p", msgData->qos,
                msgData->topicLen, msgData->topic, msgData->msgLen, msgData->message, pCtx);
}

static appMainLoopState_e INIT_INIT_Handler(void *pCtx, void *pParam) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  GPIO_Id gpio_btn = (GPIO_Id)pParam;

  APP_TRACE();

  if(gIoWake == true) {
    EVT_SEND((void *)ctx, EVT_BTN, BTN_GPIO_NUM);
  } else {
    DRV_GPIO_ConfigIrq(gpio_btn, GPIO_IRQ_FALLING_EDGE, GPIO_PULL_UP);
    DRV_GPIO_RegisterIrqCallback(gpio_btn, btn_interrupt_handler, (void *)ctx);
    DRV_GPIO_EnableIrq(gpio_btn);
  }

  return STATE_IDLE;
}

static appMainLoopState_e IDLE_BTN_Handler(void *pCtx, void *pParam) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  APP_UNUSED(pParam);

  APP_TRACE();

  /*After GPIO interrupt, here we start a timer to wait until button status stable */
  APP_ASSERT(osOK == osTimerStart(ctx->btnTimer, MAX_DBC_TIMEOUT_MS / OS_TICK_PERIOD_MS),
             "timer reset fail\n");

  return STATE_BTNDBC_WAIT;
}

static appMainLoopState_e BTNDBCWAIT_BTNEXP_Handler(void *pCtx, void *pParam) {
  uint8_t btn;

  APP_TRACE();

  /*After debouncing timer, here we can read GPIO value to confirm the button pushed */
  DRV_GPIO_GetValue((GPIO_Id)pParam, &btn);
  APP_DBG_PRINT("btn %d is %d\n", (GPIO_Id)pParam, btn);
  if (!btn || (gIoWake == true)) {
    EVT_SEND(pCtx, EVT_LTE_START, NULL);
    gIoWake = false;
  } else {
    EVT_SEND(pCtx, EVT_BTN_FALSE, pParam);
  }

  return STATE_BTN_CHECK;
}

static appMainLoopState_e BTNCHECK_LTESTART_Handler(void *pCtx, void *pParam) {
  int32_t ret;
  APP_UNUSED(pParam);

  APP_TRACE();

  /* Here we start to attach network */
  ret = lte_attach_network(atchnet_cb, pCtx);
  APP_ASSERT(0 == ret, "lte_attach_network fail, ret = %ld\n", ret);

  return STATE_LTE_ATCH_WAIT;
}

static appMainLoopState_e LTEATCHWAIT_LTEATCH_Handler(void *pCtx, void *pParam) {
  lteresult_e ret;
  lte_localtime_t *ltime;
  APP_UNUSED(pParam);

  APP_TRACE();

  ltime = malloc(sizeof(lte_localtime_t));
  APP_ASSERT(NULL != ltime, "malloc fail\n");
  ret = lte_get_localtime(ltime);
  APP_ASSERT(LTE_RESULT_OK == ret, "lte_set_report_localtime fail, ret = %lu\n", (uint32_t)ret);

  EVT_SEND(pCtx, EVT_TIME_READY, ltime);
  return STATE_LTE_ATCH;
}

static appMainLoopState_e LTEATCH_TIMEREADY_Handler(void *pCtx, void *pParam) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  lte_localtime_t *localtime = (lte_localtime_t *)pParam;
  struct tm ltm, oldltm;
  time_t ltime;
  char buff[2][80];

  APP_TRACE();
  /* Convert struct tm into time_t */
  memset((void *)&ltm, 0, sizeof(struct tm));
  ltm.tm_year = localtime->year + 100;
  ltm.tm_mon = localtime->mon - 1;
  ltm.tm_mday = localtime->mday;
  ltm.tm_hour = localtime->hour;
  ltm.tm_min = localtime->min;
  ltm.tm_sec = localtime->sec;
  ltm.tm_isdst = -1;
  free(pParam);
  ltime = mktime(&ltm);
  memset(buff, 0, sizeof(buff));
  strftime(buff[0], sizeof(buff) / 2, "%c", &ltm);
  GMTIME_R(&ctx->timeInfo, &oldltm);
  strftime(buff[1], sizeof(buff) / 2, "%c", &oldltm);
  APP_DBG_PRINT(": Time Information\nnew: %s\nold: %s\n", buff[0], buff[1]);
  ctx->timeInfo = ltime;
  if (MAX_TIME_VALID_PERIOD > difftime(ltime, *ctx->lastDoGNSStime) && !forceGpsRenew) {
    EVT_SEND(ctx, EVT_AWS_START, NULL);
  } else {
    forceGpsRenew = false;
    EVT_SEND(ctx, EVT_TIME_INVALID, NULL);
  }

  return STATE_TIME_CHECK;
}

static appMainLoopState_e TIMECHECK_AWSSTART_Handler(void *pCtx, void *pParam) {
  awsInitParam_t initParam;
  int ret;
  APP_UNUSED(pParam);

  APP_TRACE();
  memset(&initParam, 0, sizeof(awsInitParam_t));
  initParam.certProfileId = 1;
  initParam.clientId = AWS_CLIENTID;
  initParam.endpointUrl = AWS_ENDPOINTURL;
  initParam.keepAliveTime = 20;
  initParam.callback = awsMessageCb;
  initParam.evtCallback = awsConnFailureCb;
  initParam.userPriv = pCtx;

  ret = altcom_awsInitialize(&initParam);
  APP_ASSERT(AWS_SUCCESS == ret, "altcom_awsInitialize fail, ret = %d\n", ret);

  ret = altcom_awsConnect();
  APP_ASSERT(AWS_SUCCESS == ret, "altcom_awsConnect fail, ret = %d\n", ret);

  ret = altcom_awsSubscribe(AWS_QOS0, AWS_TOPIC);
  APP_ASSERT(AWS_SUCCESS == ret, "altcom_awsSubscribe fail, ret = %d\n", ret);

  EVT_SEND(pCtx, EVT_AWS_READY, NULL);
  return STATE_AWS_WAIT;
}

static appMainLoopState_e AWSWAIT_AWSREADY_Handler(void *pCtx, void *pParam) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  int ret;
  AWSError_e awsRet;
  char timeStr[80], preTimeStr[80], *ret_time = NULL, *ret_loc = NULL;
  char locStr[128];
  char buff[256];
  struct tm ltm;
  APP_UNUSED(pParam);

  /* Prepare time string */
  memset(timeStr, 0, sizeof(timeStr));
  GMTIME_R(&ctx->timeInfo, &ltm);
  strftime(timeStr, sizeof(timeStr), "%c", &ltm);

  /* Prepare time string (in fs) */
  memset(preTimeStr, 0, sizeof(preTimeStr));
  ret = get_env(KVP_LTETIME_STR, &ret_time);
  if (0 != ret) {
    APP_DBG_PRINT("getkvp %s fail, ret = %d\n", KVP_LTETIME_STR, ret);
    strncpy(preTimeStr, "N/A", sizeof(preTimeStr) - 1);
  } else {
    strncpy(preTimeStr, ret_time, sizeof(preTimeStr) - 1);
  }

  free(ret_time);

  /* Prepare location string (in fs) */
  memset(locStr, 0, sizeof(locStr));
  ret = get_env(KVP_GPSLOCATION_STR, &ret_loc);
  if (0 != ret) {
    APP_DBG_PRINT("getkvp %s fail, ret = %d\n", KVP_GPSLOCATION_STR, ret);
    strncpy(locStr, "N/A", sizeof(locStr) - 1);
  } else {
    strncpy(locStr, ret_loc, sizeof(locStr) - 1);
  }

  free(ret_loc);

  /* Prepare string to publish */
  snprintf(buff, sizeof(buff),
           "{\n\"Previous\":\n{\"time\": \"%s\",\"location\": \"%s\"\n},\n\"Current\":\n{\"time\": "
           "\"%s\",\"location\": \"%s\"\n}\n}",
           preTimeStr, locStr, timeStr, gLocation);
  /* snprintf(buff, sizeof(buff), "{\n\"Current\":{\"time\": \"%s\",\"location\": \"%s\"\n}\n}",
   * timeStr, gLocation);
   */
  awsRet = altcom_awsPublish(AWS_QOS0, AWS_TOPIC, buff);
  APP_ASSERT(AWS_SUCCESS == awsRet, "altcom_awsPublish fail, ret = %d\n", awsRet);

  /* save time info */
  ret = set_env(KVP_LTETIME_STR, timeStr);
  APP_ASSERT(0 == ret, "fail to set time %d\n", ret);

  /* save location info to retention memory. */
  ret = set_env(KVP_GPSLOCATION_STR, gLocation);
  APP_ASSERT(0 == ret, "fail to set location %d\n", ret);

  /* save to retention memory */
  ret = save_env_to_gpm();
  APP_ASSERT(0 == ret, "fail to set time %d\n", ret);

  /* Note: you may unmask below save_env() when you want to save retented data to flash. */
  // ret = save_env();
  // APP_ASSERT(0 == ret, "fail to save data to flash %d\n", ret);

  EVT_SEND(pCtx, EVT_AWS_FINISH, NULL);
  return STATE_AWS_CONNECTED;
}

static appMainLoopState_e AWSCONNECTED_AWSFINISH_Handler(void *pCtx, void *pParam) {
  AWSError_e awsRet;
  int32_t lteRet;
  APP_UNUSED(pParam);

  APP_TRACE();

  awsRet = altcom_awsDisconnect();
  APP_ASSERT(AWS_SUCCESS == awsRet, "altcom_awsDisconnect fail, ret = %d\n", awsRet);

  lteRet = lte_detach_network(LTE_FLIGHT_MODE, dtchnet_dummyInitcb, pCtx);
  APP_ASSERT(0 == lteRet, "lte_detach_network fail, ret = %ld\n", lteRet);

  return INIT_INIT_Handler(pCtx, (void *)BTN_GPIO_NUM);
}

static appMainLoopState_e TIMECHECK_TIMEINVALID_Handler(void *pCtx, void *pParam) {
  int32_t ret;
  APP_UNUSED(pParam);

  APP_TRACE();

  /* Here we start to attach network */
  ret = lte_detach_network(LTE_FLIGHT_MODE, dtchnet_cb, pCtx);
  APP_ASSERT(0 == ret, "lte_detach_network fail, ret = %ld\n", ret);

  return STATE_LTE_DTCH_WAIT;
}

static appMainLoopState_e LTEDTCHWAIT_LTEDTCH_Handler(void *pCtx, void *pParam) {
  APP_UNUSED(pParam);

  APP_TRACE();

  EVT_SEND(pCtx, EVT_GNSS_START, NULL);
  return STATE_LTE_DTCH;
}

static appMainLoopState_e LTEDTCH_GNSSSTART_Handler(void *pCtx, void *pParam) {
  int ret;
  APP_UNUSED(pParam);

  APP_TRACE();

  /* Start GPS */
  ret = altcom_gps_activate(GPSACT_GPSON, ACTIVATE_HOT_START, ALTCOM_GPSACT_NOTOL);
  APP_ASSERT(0 == ret, "altcom_gps_activate fail, ret = %d\n", ret);

  ret = altcom_gps_setnmeacfg(ALTCOM_GPSCFG_PARAM_GGA);
  APP_ASSERT(0 == ret, "altcom_gps_setnmeacfg fail, ret = %d\n", ret);

  ret = altcom_gps_setevent(EVENT_NMEA_TYPE, true, nmea_cb, pCtx);
  APP_ASSERT(0 == ret, "gps_set_report_nmeagga fail, ret = %d\n", ret);

  return STATE_GNSS_WAIT;
}

static appMainLoopState_e GNSSWAIT_GNSSREADY_Handler(void *pCtx, void *pParam) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  GnssInfo_t *gnssInfo = (GnssInfo_t *)pParam;
  int ret;

  APP_ASSERT(0 != gnssInfo, "GNSSWAIT_GNSSREADY_Handler fail gnssInfo is NULL");
  APP_TRACE();

  /*Here we have GPS information now */
  APP_DBG_PRINT("GPS fixed\n");
  APP_DBG_PRINT("utc=%f lat=%f lon=%f\n", gnssInfo->utc, gnssInfo->lat, gnssInfo->lon);
  APP_DBG_PRINT("lat=%c lon=%c\n", gnssInfo->dirlat, gnssInfo->dirlon);

  /* Turn off gps */
  ret = altcom_gps_activate(GPSACT_GPSOFF, ACTIVATE_NO_MODE, ALTCOM_GPSACT_NOTOL);
  APP_ASSERT(0 == ret, "altcom_gps_activate fail, ret = %d\n", ret);

  ret = altcom_gps_setevent(EVENT_NMEA_TYPE, false, NULL, NULL);
  APP_ASSERT(0 == ret, "altcom_gps_setevent fail, ret = %d\n", ret);

  /* save location info */
  memset(gLocation, 0, sizeof(gLocation));
  snprintf(gLocation, sizeof(gLocation), "%c%f %c%f", gnssInfo->dirlat, gnssInfo->lat,
           gnssInfo->dirlon, gnssInfo->lon);

  /* store time */
  *ctx->lastDoGNSStime = ctx->timeInfo;
  free(gnssInfo);

  EVT_SEND(pCtx, EVT_LTE_START, NULL);
  return STATE_GNSS_COMPLETE;
}

static appMainLoopState_e GNSSCOMPLETE_LTESTART_Handler(void *pCtx, void *pParam) {
  int32_t ret;
  APP_UNUSED(pParam);

  APP_TRACE();

  /* Here we start to attach network */
  ret = lte_attach_network(atchnet_cb, pCtx);
  APP_ASSERT(0 == ret, "lte_attach_network fail, ret = %ld\n", ret);

  return STATE_LTE_ATCH_WAIT;
}

static appMainLoopState_e ANY_EXPIRY_Handler(void *pCtx, void *pParam) {
  APP_UNUSED(pParam);

  APP_TRACE();

  char *ret_time_str = NULL, *ret_loc_str = NULL;
  AWSError_e awsRet;
  int32_t lteRet;
  int ret;

  if (get_env(KVP_LTETIME_STR, &ret_time_str) == 0) {
    APP_DBG_PRINT("gpstime = \"%s\"\n", ret_time_str);
  }
  free(ret_time_str);

  if (get_env(KVP_GPSLOCATION_STR, &ret_loc_str) == 0) {
    APP_DBG_PRINT("gpsloc = \"%s\"\n", ret_loc_str);
  }
  free(ret_loc_str);

  /* Post-action 1 before sleep */
  /* Force disconnect AWS MQTT Client */
  altcom_awsDisconnect();

  /* Re-init AWS MQTT Client */
  awsInitParam_t initParam;

  memset(&initParam, 0x0, sizeof(awsInitParam_t));
  initParam.certProfileId = 1;
  initParam.clientId = AWS_CLIENTID;
  initParam.endpointUrl = AWS_ENDPOINTURL;
  initParam.keepAliveTime = 20;
  initParam.callback = awsMessageCb;
  initParam.evtCallback = awsConnFailureCb;
  initParam.userPriv = pCtx;

  awsRet = altcom_awsInitialize(&initParam);
  APP_ASSERT(AWS_SUCCESS == awsRet, "altcom_awsInitialize fail, ret = %d\n", awsRet);

  /* Post-action 2 before sleep */
  /* Force detach LTE */
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)pCtx;
  if (STATE_LTE_ATCH_WAIT == ctx->currState || STATE_LTE_ATCH == ctx->currState ||
      STATE_TIME_CHECK == ctx->currState || STATE_AWS_WAIT == ctx->currState ||
      STATE_AWS_CONNECTED == ctx->currState) {
    lteRet = lte_detach_network(LTE_FLIGHT_MODE, dtchnet_dummycb, pCtx);
    APP_ASSERT(0 == lteRet, "lte_detach_network fail, ret = %ld\n", lteRet);
  }

  /* Post-action 3 before sleep */
  /* Turn off gps */
  ret = altcom_gps_activate(GPSACT_GPSOFF, ACTIVATE_NO_MODE, ALTCOM_GPSACT_NOTOL);
  APP_ASSERT(0 == ret, "altcom_gps_activate fail, ret = %d\n", ret);

  ret = altcom_gps_setevent(EVENT_NMEA_TYPE, false, NULL, NULL);
  APP_ASSERT(0 == ret, "gps_set_report_nmea turn off fail, ret = %d\n", ret);

  EVT_SEND(pCtx, EVT_SLEEP, NULL);
  return STATE_SLEEP;
}

static int wake_by_io(void) {
  DRV_PM_Statistics stat;
  int res;

  res = DRV_PM_GetStatistics(&stat);
  if (res == 0) {
    APP_DBG_PRINT("[PM]wake up cause: %d\n", stat.last_cause);
    if(stat.last_cause == DRV_PM_WAKEUP_CAUSE_IO_ISR)
      return 1;
  }
  return 0;
}

static appMainLoopState_e SLEEP_Handler(void *pCtx, void *pParam) {
  APP_UNUSED(pParam);

  APP_TRACE();

  char c;
  APP_DBG_PRINT("Going to sleep...\n");

  smngr_dev_busy_clr(pwrawsdemo_smngr_sync_id);

  osStatus_t ret_val;
  ret_val = osMessageQueueGet(wakeq, (void *)&c, 0, osWaitForever);
  APP_ASSERT(osOK == ret_val, "[PM]Wakeup fail, ret=%d\n", ret_val);

  APP_DBG_PRINT("[PM]MCU WakeUp\n");

  smngr_dev_busy_set(pwrawsdemo_smngr_sync_id);
  if(wake_by_io()){
    gIoWake = true;
  }

  return INIT_INIT_Handler(pCtx, (void *)BTN_GPIO_NUM);
}

static void appMainLoopTask(void *arg) {
  appMainLoopCtx_t *ctx = (appMainLoopCtx_t *)arg;
  osStatus_t ret;
  appMainLoopEvt_t evt;

  APP_TRACE();

  /* Main event loop */
  while (ctx->taskRunning) {
    /* Event receiving */
    ret = osMessageQueueGet(ctx->evtQueue, (void *)&evt, 0, osWaitForever);
    if (ret != osOK) {
      APP_DBG_PRINT("xQueueReceive fail\n");
      break;
    }

    /* Event handling and state transition */
#if (PWRMGMTAWS_PRINT_STM_STR == 1)
    APP_DBG_PRINT("[STM]currState: %s, evt: %s\n", state_string[(int)ctx->currState], evt_string[(int)evt.event]);
#else
    APP_DBG_PRINT("[STM]currState: %d, evt: %d\n", (int)ctx->currState, (int)evt.event);
#endif
    if (ctx->stateTransMatrix[ctx->currState][evt.event]) {
      ctx->currState =
          ctx->stateTransMatrix[ctx->currState][evt.event]((void *)ctx, (void *)evt.param);

      osStatus_t ret;

      if (osTimerIsRunning(ctx->sleepTimer)) {
        ret = osTimerStop(ctx->sleepTimer);
        APP_ASSERT(osOK == ret, "[STM]timer stop fail, ret=%d\n", ret);
      }

      if((ctx->currState != STATE_SLEEP) && (evt.event != EVT_EXPIRY)) {
            ret = osTimerStart(ctx->sleepTimer, ctx->expiryTimeoutTable[ctx->currState]);
            APP_ASSERT(osOK == ret, "[STM]timer change period fail, ret=%d\n", ret);
      }
    } else {
#if (PWRMGMTAWS_PRINT_STM_STR == 1) 
      APP_DBG_PRINT("[STM]evt: %s ignored in currState: %s\n", evt_string[(int)evt.event], state_string[ctx->currState]);
#else
      APP_DBG_PRINT("[STM]evt: %d ignored in currState: %d\n", (int)evt.event, ctx->currState);
#endif
    }

    /* Release event element */
#if (PWRMGMTAWS_PRINT_STM_STR == 1)
    APP_DBG_PRINT("[STM]nextState: %s\n", state_string[(int)ctx->currState]);
#else
    APP_DBG_PRINT("[STM]nextState: %d\n", (int)ctx->currState);
#endif
  }

  APP_TRACE();

  /* Task finished */
  osThreadExit();
}

static int init_configuration(void) {
  DRV_PM_Statistics stat;
  int res;

  res = DRV_PM_GetStatistics(&stat);
  if ((res != 0) || (stat.wakeup_state != DRV_PM_WAKEUP_STATEFUL)) {
    gLocalTime = 0;
    memset(gLocation, 0, sizeof(gLocation));
  }

  return 0;
}

static int32_t pwrAwsDemo_sleep_notify(sleep_notify_state pm_state, PWR_MNGR_PwrMode pwr_mode, void *ptr_ctx) {
 if (pm_state == RESUMING) {
    APP_ASSERT(osOK == osMessageQueuePut(wakeq, (const void *)'F', 0, 0),
               "event send fail-3\n");
  }

  return SLEEP_NOTIFY_SUCCESS;
}

static int32_t altcom_config_init() {
  altcom_init_t initCfg;

  memset((void *)&initCfg, 0x0, sizeof(initCfg));
  /* Configure the debugging message level */
  initCfg.dbgLevel = ALTCOM_DBG_NORMAL;

  /* Provide application log lock interface */
  loglock_if_t loglock_if = {appMainLoop_log_lock, appMainLoop_log_unlock};
  initCfg.loglock_if = &loglock_if;

  /* Configure the buffer pool */
  blockset_t blkset[] = {{16, 16}, {32, 12}, {128, 5}, {256, 4}, {512, 2}, {2064, 1}, {5120, 2}};
  initCfg.bufMgmtCfg.blkCfg.blksetCfg = blkset;
  initCfg.bufMgmtCfg.blkCfg.blksetNum = sizeof(blkset) / sizeof(blkset[0]);

  /* Configure the HAL */
  initCfg.halCfg.halType = ALTCOM_HAL_INT_UART;
  initCfg.halCfg.virtPortId = 0;

  return altcom_initialize(&initCfg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int appMainLoopInit(void *arg) {
  app_log_mtx = osMutexNew(NULL);
  if (!app_log_mtx) {
    APP_DBG_PRINT("app_log_mtx init failed\n");
    return -1;
  }

  if (0 != altcom_config_init()) {
    APP_DBG_PRINT("altcom_config_init fail\n");
    return -2;
  }

  memset(&gAppMainLoopCtx, 0, sizeof(appMainLoopCtx_t));

  /* Initialize state transition table and expiry timeout*/
  int i, j;

  for (i = STATE_INIT; i < STATE_MAX; i++) {
    /* Init state transition matrix */
    for (j = EVT_INIT; j < EVT_MAX; j++) {
      gAppMainLoopCtx.stateTransMatrix[i][j] = NULL;
      if (j == EVT_EXPIRY) {
        gAppMainLoopCtx.stateTransMatrix[i][EVT_EXPIRY] = ANY_EXPIRY_Handler;
      }
    }

    /* Init expiry timeout table */
    gAppMainLoopCtx.expiryTimeoutTable[i] = DEFAULT_SLEEP_TIMEOUT_MS / OS_TICK_PERIOD_MS;
  }

  /* Construct state transition table */
  gAppMainLoopCtx.stateTransMatrix[STATE_INIT][EVT_INIT] = INIT_INIT_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_IDLE][EVT_BTN] = IDLE_BTN_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_IDLE][EVT_SWBTN] = BTNCHECK_LTESTART_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_BTNDBC_WAIT][EVT_BTN_EXP] = BTNDBCWAIT_BTNEXP_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_BTN_CHECK][EVT_LTE_START] = BTNCHECK_LTESTART_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_BTN_CHECK][EVT_BTN_FALSE] = INIT_INIT_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_LTE_ATCH_WAIT][EVT_LTE_ATCH] = LTEATCHWAIT_LTEATCH_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_LTE_ATCH][EVT_TIME_READY] = LTEATCH_TIMEREADY_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_TIME_CHECK][EVT_AWS_START] = TIMECHECK_AWSSTART_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_AWS_WAIT][EVT_AWS_READY] = AWSWAIT_AWSREADY_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_AWS_CONNECTED][EVT_AWS_FINISH] =
      AWSCONNECTED_AWSFINISH_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_TIME_CHECK][EVT_TIME_INVALID] =
      TIMECHECK_TIMEINVALID_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_LTE_DTCH_WAIT][EVT_LTE_DTCH] = LTEDTCHWAIT_LTEDTCH_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_LTE_DTCH][EVT_GNSS_START] = LTEDTCH_GNSSSTART_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_GNSS_WAIT][EVT_GNSS_READY] = GNSSWAIT_GNSSREADY_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_GNSS_COMPLETE][EVT_LTE_START] =
      GNSSCOMPLETE_LTESTART_Handler;
  gAppMainLoopCtx.stateTransMatrix[STATE_SLEEP][EVT_EXPIRY] = (stateTransFunc_t)NULL;
  gAppMainLoopCtx.stateTransMatrix[STATE_SLEEP][EVT_SLEEP] = SLEEP_Handler;

  /* Setup expiry timeout table */
  gAppMainLoopCtx.expiryTimeoutTable[STATE_GNSS_WAIT] = GNSS_TIMEOUT_MS / OS_TICK_PERIOD_MS;

  /* Initialize sleep timer */
  gAppMainLoopCtx.sleepTimer =
      osTimerNew(sleep_timeout, osTimerOnce, (void *)&gAppMainLoopCtx, NULL);
  if (!gAppMainLoopCtx.sleepTimer) {
    APP_DBG_PRINT("sleepTimer create fail\n");
    return -3;
  }

  /* Initialize debouncing timer */
  gAppMainLoopCtx.btnTimer = osTimerNew(btn_timeout, osTimerOnce, (void *)&gAppMainLoopCtx, NULL);
  if (!gAppMainLoopCtx.btnTimer) {
    APP_DBG_PRINT("btnTimer create fail\n");
    return -4;
  }

  /* Initialize misc */
  gAppMainLoopCtx.timeInfo = gLocalTime;
  gAppMainLoopCtx.lastDoGNSStime = &gLocalTime;
  gAppMainLoopCtx.taskRunning = true;
  gAppMainLoopCtx.currState = STATE_INIT;
  gAppMainLoopCtx.evtQueue =
      osMessageQueueNew(MAX_APPMAINLOOP_QUENUM, sizeof(appMainLoopEvt_t), NULL);
  if (!gAppMainLoopCtx.evtQueue) {
    APP_DBG_PRINT("evtQueue create fail\n");
    return -5;
  }

  /* copy data from db */
  if (init_configuration() != 0) {
    APP_DBG_PRINT("configuration initialize fail\n");
    return -6;
  }

  /* Register the sleep notify callback function */
  int32_t index;

  if (sleep_notify_register_callback(&pwrAwsDemo_sleep_notify, &index, NULL) != 0) {
    APP_DBG_PRINT("sleep notify insert callback item fail\n");
    return -7;
  }

  /* Register the sleep manager as sync task */
  if (smngr_register_dev_sync(PWRAWSDEMO_SMNGR_DEVNAME_SYNC, &pwrawsdemo_smngr_sync_id) != 0) {
    APP_DBG_PRINT("smngr register dev sync fail\n");
    return -8;
  }
  smngr_dev_busy_set(pwrawsdemo_smngr_sync_id);

  /* Configure power mode */
  if (pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0) != PWR_MNGR_OK) {
    APP_DBG_PRINT("power manager set sleep mode fail\n");
    return -9;
  }

  pwr_mngr_conf_set_cli_inact_time(2);

  /* Enable Power Manager */
  pwr_mngr_enable_sleep(1);

  wakeq = osMessageQueueNew(16, sizeof(char), NULL);
  if (!wakeq) {
    APP_DBG_PRINT("wakeupQueue create fail\n");
    return -11;
  }

  /*Create main loop task */
  osThreadAttr_t attr = {0};

  attr.name = "AppMainLoop";
  attr.stack_size = APPMAINLOOP_TASK_STACKSIZE;
  attr.priority = APPMAINLOOP_TASK_PRI;
  gAppMainLoopCtx.taskHandle = osThreadNew(appMainLoopTask, (void *)&gAppMainLoopCtx, &attr);
  if (!gAppMainLoopCtx.taskHandle) {
    APP_DBG_PRINT("taskHandle create fail\n");
    return -10;
  }

  /*Send init event */
  EVT_SEND(&gAppMainLoopCtx, EVT_INIT, BTN_GPIO_NUM);
  return 0;
}

void sendSwBtnEvt(void) { EVT_SEND(&gAppMainLoopCtx, EVT_SWBTN, NULL); }
