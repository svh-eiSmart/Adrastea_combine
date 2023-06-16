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

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "gps/altcom_gps.h"

/* App includes */
#include "apitest_main.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APITEST_RESULT_OK (0)
#define APITEST_RESULT_ERROR (1)

#define APITEST_CMD_ARG (1)
#define APITEST_CMD_PARAM_1 (2)
#define APITEST_CMD_PARAM_2 (3)
#define APITEST_CMD_PARAM_3 (4)
#define APITEST_CMD_PARAM_4 (5)
#define APITEST_CMD_PARAM_5 (6)
#define APITEST_CMD_PARAM_6 (7)
#define APITEST_CMD_PARAM_7 (8)
#define APITEST_CMD_PARAM_8 (9)

#define APITEST_MAX_NUMBER_OF_PARAM (12)
#define APITEST_MAX_API_MQUEUE (16)

#define APITEST_TASK_STACKSIZE (2048)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_INVALID_ARG ("INVALID")
#define APITEST_NULL_ARG ("NULL")

#define APITEST_GETFUNCNAME(func) (#func)

const int GPS_ROLLOVER_MAX = 6;
const int GPS_ROLLOVER_MIN = 0;
const int BLANKING_GUARDTIME_MAX_MSEC = 5000;

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
    printf("\r\n");            \
    printf(__VA_ARGS__);       \
    UNLOCK();                  \
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

#define STR_BUF_LEN 512
/****************************************************************************
 * Private Data Type
 ****************************************************************************/
struct apitest_command_s {
  int argc;
  char *argv[APITEST_MAX_NUMBER_OF_PARAM];
};

struct data_s {
  char *data;
  int16_t len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;
static char *gps_evt_str[EVENT_MAX_TYPE] = {
    "EVENT_NMEA_TYPE",   "EVENT_SESSIONST_TYPE", "EVENT_ALLOWSTAT_TYPE", "EVENT_COLDSTART_TYPE",
    "EVENT_EPHUPD_TYPE", "EVENT_FIX_TYPE",       "EVENT_BLANKING_TYPE",  "EVENT_TTFF_TYPE"};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
void nmeagga_event_cb(gps_event_t *event, void *userPriv) {
  if (event->nmeaType == EVENT_NMEA_GGA_TYPE) {
    if (event->u.gga.quality == 1) {
      APITEST_DBG_PRINT("\r\nGPS Fixed");
    }

    APITEST_DBG_PRINT("\r\nGGA utc=%f ", event->u.gga.UTC);
    APITEST_DBG_PRINT("lat=%f dirlat=%c ", event->u.gga.lat, event->u.gga.dirlat);
    APITEST_DBG_PRINT("lon=%f dirlon=%c ", event->u.gga.lon, event->u.gga.dirlon);
    APITEST_DBG_PRINT("sv=%hd ", event->u.gga.numsv);
    APITEST_DBG_PRINT("priv:%p", userPriv);
  } else if (event->nmeaType == EVENT_NMEA_GSV_TYPE) {
    for (int i = 0; i < event->u.gsv.numSV; i++)
      APITEST_DBG_PRINT("\r\nGSV num:%hd SNR=%hd, PRN=%hd azi=%hd, ele=%hd", i,
                        event->u.gsv.sat[i].SNR, event->u.gsv.sat[i].PRN,
                        event->u.gsv.sat[i].azimuth, event->u.gsv.sat[i].elevation);
  } else if (event->nmeaType == EVENT_NMEA_RMC_TYPE) {
    APITEST_DBG_PRINT("\r\nRMC utc=%f ", event->u.rmc.UTC);
    APITEST_DBG_PRINT("lat=%f dirlat=%c ", event->u.rmc.lat, event->u.rmc.dirlat);
    APITEST_DBG_PRINT("lon=%f dirlon=%c ", event->u.rmc.lon, event->u.rmc.dirlon);
    APITEST_DBG_PRINT("date=%f", event->u.rmc.date);

  } else if (event->nmeaType == EVENT_NMEA_RAW_TYPE) {
    APITEST_DBG_PRINT("\r\nRaw NMEA=%s", event->u.raw.nmea_msg);
  } else
    APITEST_DBG_PRINT("\r\nUnknown NMEA ignored. Type[%d]", event->nmeaType);
}

void urc_event_cb(gps_event_t *event, void *userPriv) {
  switch (event->eventType) {
    case EVENT_SESSIONST_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.sessionst, userPriv);
      break;

    case EVENT_ALLOWSTAT_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.allowst, userPriv);
      break;

    case EVENT_COLDSTART_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.coldstart, userPriv);
      break;

    case EVENT_EPHUPD_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.ephupd, userPriv);
      break;

    case EVENT_FIX_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.fixst, userPriv);
      break;

    case EVENT_BLANKING_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.blanking, userPriv);
      break;

    case EVENT_TTFF_TYPE:
      APITEST_DBG_PRINT("GPS %s event, status=%d, priv:%p\n", gps_evt_str[event->eventType],
                        event->u.ttff, userPriv);
      break;

    default:
      break;
  }
}

void event_cb(gps_event_t *event, void *userPriv) {
  if (EVENT_NMEA_TYPE == event->eventType) {
    nmeagga_event_cb(event, userPriv);
  } else if (EVENT_SESSIONST_TYPE <= event->eventType && EVENT_MAX_TYPE > event->eventType) {
    urc_event_cb(event, userPriv);
  } else {
    APITEST_DBG_PRINT("Invalid GPS event %lu\n", (uint32_t)event->eventType);
  }
}

static void print_retval(bool val, char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

static void apitest_task(void *arg) {
  int cmp_res;
  osStatus_t ret;
  char *dummystr;
  unsigned int tolerance;
  unsigned int timeout = 0;
  unsigned int guardtime = 0;
  int activate_upon_disallow = 0;
  bool guard_time_enabled;
  bool activate_upon_disallow_enabled;
  cepDay_e days = CEP_DAY_CUSTOM_URL;
  gpsActFlag_e mode = GPSACT_GPSOFF;
  gpsActMode_e activation_mode = ACTIVATE_HOT_START;
  struct apitest_command_s *command;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until send commands */

    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "gps", strlen("gps"));
      if (cmp_res == 0) {

        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "act", strlen("act"));
        if (cmp_res == 0) {

          if (command->argv[APITEST_CMD_PARAM_2]) {
            mode = atoi(command->argv[APITEST_CMD_PARAM_2]);
          }
          if (mode == GPSACT_GPSON) {

            if (command->argv[APITEST_CMD_PARAM_3]) {
              activation_mode = atoi(command->argv[APITEST_CMD_PARAM_3]);
            }
            if ((activation_mode == ACTIVATE_COLD_START) ||
                (activation_mode == ACTIVATE_HOT_START)) {

              print_retval(0 == altcom_gps_activate(mode, activation_mode, ALTCOM_GPSACT_NOTOL),
                           APITEST_GETFUNCNAME(altcom_gps_activate));

              // print_retval(0 == altcom_gps_setnmeacfg(ALTCOM_GPSCFG_PARAM_GGA),
              //           APITEST_GETFUNCNAME(altcom_gps_setnmeacfg));
              print_retval(
                  0 == altcom_gps_setevent(EVENT_NMEA_TYPE, true, event_cb, (void *)0xdeafbeef),
                  APITEST_GETFUNCNAME(altcom_gps_setevent));
            } else {
              APITEST_DBG_PRINT("Arguments Error: Wrong activation mode\n");
              continue;
            }
          } else if (mode == GPSACT_GPSON_TOL) {
            tolerance = strtoul(command->argv[APITEST_CMD_PARAM_3], &dummystr, 10);
            if (tolerance > 99999) {
              APITEST_DBG_PRINT("Arguments Error: Tolerance out of range\n");
              continue;
            }
            print_retval(0 == altcom_gps_activate(mode, ACTIVATE_HOT_START, tolerance),
                         APITEST_GETFUNCNAME(altcom_gps_activate));
            // print_retval(0 == altcom_gps_setnmeacfg(ALTCOM_GPSCFG_PARAM_GGA),
            //           APITEST_GETFUNCNAME(altcom_gps_setnmeacfg));
            print_retval(
                0 == altcom_gps_setevent(EVENT_NMEA_TYPE, true, event_cb, (void *)0xdeafbeef),
                APITEST_GETFUNCNAME(altcom_gps_setevent));
          } else if (mode == GPSACT_GPSOFF) {
            print_retval(0 == altcom_gps_activate(mode, ACTIVATE_NO_MODE, ALTCOM_GPSACT_NOTOL),
                         APITEST_GETFUNCNAME(altcom_gps_activate));
            print_retval(0 == altcom_gps_setevent(EVENT_NMEA_TYPE, false, event_cb, NULL),
                         APITEST_GETFUNCNAME(altcom_gps_setevent));
          } else {
            APITEST_DBG_PRINT("Arguments Error: Wrong mode\n");
            continue;
          }
        }

        // set NMEA filter
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "setnmea", strlen("setnmea"));
        if (cmp_res == 0) {
          print_retval(0 == altcom_gps_setnmeacfg(atoi(command->argv[APITEST_CMD_PARAM_2])),
                       APITEST_GETFUNCNAME(altcom_gps_setnmeacfg));
        }

        // CEP
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "cep", strlen("cep"));
        if (cmp_res == 0) {
          int ret;
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "dld", strlen("dld"))) {
            uint8_t result;

            if (!(command->argv[APITEST_CMD_PARAM_3])) {
              APITEST_DBG_PRINT("Arguments Error: Need to insert number of days to download\n");
              continue;
            }
            days = atoi(command->argv[APITEST_CMD_PARAM_3]);
            if (!((days == CEP_DAY_CUSTOM_URL) || (days == CEP_DAY_1) || (days == CEP_DAY_2) ||
                  (days == CEP_DAY_3) || (days == CEP_DAY_7) || (days == CEP_DAY_14) ||
                  (days == CEP_DAY_28))) {
              APITEST_DBG_PRINT("Arguments Error: Wrong days number\n");
              continue;
            }
            if (command->argv[APITEST_CMD_PARAM_4]) {
              timeout = atoi(command->argv[APITEST_CMD_PARAM_4]);
            }
            print_retval((ret = altcom_gps_cep_download(days, timeout, &result)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_cep_download));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "erase", strlen("erase"))) {
            uint8_t result;

            print_retval((ret = altcom_gps_cep_erase(&result)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_cep_erase));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "stat", strlen("stat"))) {
            cepStatusResult_t response;

            print_retval((ret = altcom_gps_cep_status(&response)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_cep_status));
            if (ret == 0) {
              APITEST_DBG_PRINT("status: %d\n", (int)response.result);
              if ((int)response.result == 1) {
                APITEST_DBG_PRINT("days: %d\n", (int)response.days);
                APITEST_DBG_PRINT("hours: %d\n", (int)response.hours);
                APITEST_DBG_PRINT("minutes: %d\n", (int)response.minutes);
              }
            }
          } else {
            APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
            continue;
          }
        }

        // set event filter
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "event", strlen("event"));
        if (cmp_res == 0) {
          print_retval(
              0 == altcom_gps_setevent((eventType_e)atoi(command->argv[APITEST_CMD_PARAM_2]),
                                       (bool)atoi(command->argv[APITEST_CMD_PARAM_3]), event_cb,
                                       (void *)0xdeafbeef),
              APITEST_GETFUNCNAME(altcom_gps_setevent));
        }

        // memory erase
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "memerase", strlen("memerase"));
        if (cmp_res == 0) {
          print_retval(0 == altcom_gps_memerase(atoi(command->argv[APITEST_CMD_PARAM_2])),
                       APITEST_GETFUNCNAME(altcom_gps_memerase));
        }

        // rollover configure
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "rollover", strlen("rollover"));
        if (cmp_res == 0) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "set", strlen("set"))) {
            if (!(command->argv[APITEST_CMD_PARAM_3])) {
              APITEST_DBG_PRINT("Arguments error: No Rollover input value\n");
              continue;
            }
            int val = atoi(command->argv[APITEST_CMD_PARAM_3]);
            if ((val > GPS_ROLLOVER_MAX) || (val < GPS_ROLLOVER_MIN)) {
              APITEST_DBG_PRINT("Arguments error: Input out of range\n");
              continue;
            }
            print_retval(0 == altcom_gps_setrollover(val),
                         APITEST_GETFUNCNAME(altcom_gps_setrollover));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "get", strlen("get"))) {
            unsigned char result;
            print_retval(0 == altcom_gps_getrollover(&result),
                         APITEST_GETFUNCNAME(altcom_gps_getrollover));
            APITEST_DBG_PRINT("rollover: %d\n", result);
          } else {
            APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
            continue;
          }
        }

        // gps info
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "info", strlen("info"));
        if (cmp_res == 0) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "eph", strlen("eph"))) {
            infoEphResult_t response;

            print_retval((ret = altcom_gps_info_get_ephemeris_status(&response)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_info_get_ephemeris_status));
            if (ret == 0) {
              APITEST_DBG_PRINT("status: %d\n", (int)response.status);
              if (response.status == 1) {
                APITEST_DBG_PRINT("min sat age: %d\n", (int)response.minsatage);
                APITEST_DBG_PRINT("num of satellites: %d\n", (int)response.numofsats);
                APITEST_DBG_PRINT("satellites with ephemeris:");
                for (int i = 0; i < response.numofsats; i++) {
                  APITEST_DBG_PRINT("%d", (int)response.satlist[i]);
                }
              }
            }
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "ttff", strlen("ttff"))) {
            int32_t response;

            print_retval((ret = altcom_gps_info_get_ttff(&response)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_info_get_ttff));
            if (ret == 0) {
              APITEST_DBG_PRINT("ttff: %ld\n", response);
            }
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "fix", strlen("fix"))) {
            infoFixResult_t response;

            print_retval((ret = altcom_gps_info_get_fix(&response)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_info_get_fix));
            if (ret == 0) {
              APITEST_DBG_PRINT("fix type: %ld", response.fixtype);
              if (response.fixtype != 0) {
                APITEST_DBG_PRINT("latitude: %lf", response.latitude);
                APITEST_DBG_PRINT("longitude: %lf", response.longitude);
                APITEST_DBG_PRINT("altitude: %lf", response.altitude);
                APITEST_DBG_PRINT("utc timestamp: %lld", response.utc);
                if (response.accuracy_is_present == 1) {
                  APITEST_DBG_PRINT("accuracy: %f", response.accuracy);
                }
                if (response.speed_is_present == 1) {
                  APITEST_DBG_PRINT("speed: %f", response.speed);
                }
                APITEST_DBG_PRINT("eph type: %c", response.ephtype);
                APITEST_DBG_PRINT("num of sats: %ld", response.num_of_sats_for_fix);
              }
            }
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "sat", strlen("sat"))) {
            infoSatResult_t response;

            print_retval((ret = altcom_gps_info_get_sat_in_view(&response)) == 0,
                         APITEST_GETFUNCNAME(altcom_gps_info_get_sat_in_view));
            if (ret == 0) {
              APITEST_DBG_PRINT("number of satellites in view: %lu", response.numofsats);
              for (unsigned int i = 0; i < response.numofsats; i++) {
                APITEST_DBG_PRINT("PRN: %ld, elevation: %ld, azimuth: %ld, SNR: %ld",
                                  response.satlist[i].PRN, response.satlist[i].elevation,
                                  response.satlist[i].azimuth, response.satlist[i].SNR);
              }
            }
          } else {
            APITEST_DBG_PRINT("Arguments error - Wrong command\n");
          }
        }

        // blanking configure
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "blanking", strlen("blanking"));
        if (cmp_res == 0) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "set", strlen("set"))) {
            if (!(command->argv[APITEST_CMD_PARAM_3])) {
              APITEST_DBG_PRINT("Arguments error: No Blanking input value\n");
              continue;
            }
            int mode = atoi(command->argv[APITEST_CMD_PARAM_3]);
            if ((mode != BLANKING_DISABLE) && (mode != BLANKING_ENABLE)) {
              APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
              continue;
            }
            if (command->argv[APITEST_CMD_PARAM_4]) {
              guard_time_enabled = true;
              guardtime = atoi(command->argv[APITEST_CMD_PARAM_4]);
              if (guardtime > BLANKING_GUARDTIME_MAX_MSEC) {
                APITEST_DBG_PRINT("Arguments error: Guard Time out of range\n");
                continue;
              }
            }
            else {
            	guard_time_enabled = false;
            }
            if (command->argv[APITEST_CMD_PARAM_5]) {
              activate_upon_disallow_enabled = true;
              activate_upon_disallow = atoi(command->argv[APITEST_CMD_PARAM_5]);
              if ((activate_upon_disallow != BLANKING_DISABLE) &&
                  (activate_upon_disallow != BLANKING_ENABLE)) {
                APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
                continue;
              }
            }
            else {
            	activate_upon_disallow_enabled = false;
            }
            print_retval(
                0 == altcom_gps_setblankingcfg(mode, guardtime, activate_upon_disallow,
                                               guard_time_enabled, activate_upon_disallow_enabled),
                APITEST_GETFUNCNAME(altcom_gps_setblankingcfg));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "get", strlen("get"))) {
            blankingResult_t response;
            print_retval(0 == altcom_gps_getblankingcfg(&response),
                         APITEST_GETFUNCNAME(altcom_gps_getblankingcfg));
            APITEST_DBG_PRINT("mode: %ld", response.mode);
            APITEST_DBG_PRINT("guard time: %ld", response.guardtime);
            APITEST_DBG_PRINT("blanking upon RF disallowed: %ld\n", response.activateupondisallow);
          } else {
            APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
            continue;
          }
        }

        // tow algorithm configure
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "tow", strlen("tow"));
        if (cmp_res == 0) {
          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "set", strlen("set"))) {
            if (!(command->argv[APITEST_CMD_PARAM_3])) {
              APITEST_DBG_PRINT("Arguments error: No Tow Algorithm input value\n");
              continue;
            }

            int mode = atoi(command->argv[APITEST_CMD_PARAM_3]);
            if ((mode != SKIP_TOW_VERIFICATION_OFF) && (mode != SKIP_TOW_VERIFICATION_ON)) {
              APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
              continue;
            }
            print_retval(0 == altcom_gps_set_time_of_week_algorithm(mode),
                         APITEST_GETFUNCNAME(altcom_gps_set_time_of_week_algorithm));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "get", strlen("get"))) {
            int32_t result;
            print_retval(0 == altcom_gps_get_time_of_week_algorithm(&result),
                         APITEST_GETFUNCNAME(altcom_gps_get_time_of_week_algorithm));
            APITEST_DBG_PRINT("time of week algorithm mode: %ld\n", result);
          } else {
            APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
            continue;
          }
        }

        // gnss satellites configure
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "sat", strlen("sat"));
        if (cmp_res == 0) {
          satsConfigSystems_e systems;

          if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "set", strlen("set"))) {
            bool gps_config_enabled = false;
            bool glonass_config_enabled = false;
            if (!(command->argv[APITEST_CMD_PARAM_3])) {
              APITEST_DBG_PRINT("Arguments error: Not enough araguments\n");
              continue;
            }
            if (0 == strncmp(command->argv[APITEST_CMD_PARAM_3], "gps", strlen("gps"))) {
              gps_config_enabled = true;
            } else if (0 ==
                       strncmp(command->argv[APITEST_CMD_PARAM_3], "glonass", strlen("glonass"))) {
              glonass_config_enabled = true;
            } else {
              APITEST_DBG_PRINT("Arguments error: Wrong satellites system\n");
              continue;
            }

            if (command->argv[APITEST_CMD_PARAM_4]) {
              if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "gps", strlen("gps"))) {
                gps_config_enabled = true;
              } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_4], "glonass",
                                      strlen("glonass"))) {
                glonass_config_enabled = true;
              } else {
                APITEST_DBG_PRINT("Arguments error: Wrong satellites system\n");
                continue;
              }
            }

            if (gps_config_enabled == true) {
              if (glonass_config_enabled == true) {
                systems = GNSS_SYSTEMS_BOTH;
              } else {
                systems = GNSS_SYSTEMS_GPS;
              }
            } else if (glonass_config_enabled == true) {
              systems = GNSS_SYSTEMS_GLONASS;
            } else {
              systems = GNSS_SYSTEMS_NONE;
            }

            print_retval(0 == altcom_gps_set_satellites_cfg(systems),
                         APITEST_GETFUNCNAME(altcom_gps_set_satellites_cfg));
          } else if (0 == strncmp(command->argv[APITEST_CMD_PARAM_2], "get", strlen("get"))) {
            print_retval(0 == altcom_gps_get_satellites_cfg(&systems),
                         APITEST_GETFUNCNAME(altcom_gps_get_satellites_cfg));
            APITEST_DBG_PRINT("satellites systems in use:");
            if (systems == GNSS_SYSTEMS_GPS) {
              APITEST_DBG_PRINT("GPS\n");
            } else if (systems == GNSS_SYSTEMS_GLONASS) {
              APITEST_DBG_PRINT("GLONASS\n");
            } else if (systems == GNSS_SYSTEMS_BOTH) {
              APITEST_DBG_PRINT("GPS\n");
              APITEST_DBG_PRINT("GLONASS\n");
            }
          } else {
            APITEST_DBG_PRINT("Arguments error: Wrong mode\n");
            continue;
          }
        }
      }
    }

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
