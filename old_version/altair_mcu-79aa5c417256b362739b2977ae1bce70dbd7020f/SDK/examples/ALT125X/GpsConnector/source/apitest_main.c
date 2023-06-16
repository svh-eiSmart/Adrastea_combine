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

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
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
      APITEST_DBG_PRINT("\r\nGSV num:%hd SNR=%hd, PRN=%hd azi=%hd, ele=%hd", i, event->u.gsv.sat[i].SNR,
                        event->u.gsv.sat[i].PRN, event->u.gsv.sat[i].azimuth,
                        event->u.gsv.sat[i].elevation);
  } else if (event->nmeaType == EVENT_NMEA_RMC_TYPE) {
      APITEST_DBG_PRINT("\r\nRMC utc=%f ", event->u.rmc.UTC);
      APITEST_DBG_PRINT("lat=%f dirlat=%c ",event->u.rmc.lat, event->u.rmc.dirlat);
      APITEST_DBG_PRINT("lon=%f dirlon=%c ", event->u.rmc.lon, event->u.rmc.dirlon);
      APITEST_DBG_PRINT("date=%f", event->u.rmc.date);

  } else if (event->nmeaType == EVENT_NMEA_RAW_TYPE) {
      APITEST_DBG_PRINT("\r\nRaw NMEA=%s", event->u.raw.nmea_msg);
  } else
    APITEST_DBG_PRINT("\r\nUnknown NMEA ignored. Type[%d]", event->nmeaType);
}

void sessionst_event_cb(gps_event_t *event, void *userPriv) {
  APITEST_DBG_PRINT("GPS sessionst=%lu, priv:%p\n", (uint32_t)event->u.sessionst, userPriv);
}

void allowedst_event_cb(gps_event_t *event, void *userPriv) {
  APITEST_DBG_PRINT("GPS allowedst=%lu, priv:%p\n", (uint32_t)event->u.allowedst, userPriv);
}

void event_cb(gps_event_t *event, void *userPriv) {
  if (EVENT_NMEA_TYPE == event->eventType) {
    nmeagga_event_cb(event, userPriv);
  } else if (EVENT_SESSIONST_TYPE == event->eventType) {
    sessionst_event_cb(event, userPriv);
  } else if (EVENT_ALLOWEDST_TYPE == event->eventType) {
    allowedst_event_cb(event, userPriv);
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
        // activate
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "act", strlen("act"));
        if (cmp_res == 0) {
          print_retval(0 == altcom_gps_activate(ALTCOM_GPSACT_GPSON, ALTCOM_GPSACT_NOTOL),
                       APITEST_GETFUNCNAME(altcom_gps_activate));
          //print_retval(0 == altcom_gps_setnmeacfg(ALTCOM_GPSCFG_PARAM_GGA),
            //           APITEST_GETFUNCNAME(altcom_gps_setnmeacfg));
          print_retval(
              0 == altcom_gps_setevent(EVENT_NMEA_TYPE, true, event_cb, (void *)0xdeafbeef),
              APITEST_GETFUNCNAME(altcom_gps_setevent));
        }

        // de-activate
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "dact", strlen("dact"));
        if (cmp_res == 0) {
          print_retval(0 == altcom_gps_setevent(EVENT_NMEA_TYPE, false, event_cb, NULL),
                       APITEST_GETFUNCNAME(altcom_gps_setevent));
          print_retval(0 == altcom_gps_activate(ALTCOM_GPSACT_GPSOFF, ALTCOM_GPSACT_NOTOL),
                       APITEST_GETFUNCNAME(altcom_gps_activate));
        }

        // get version
        cmp_res = strncmp(command->argv[APITEST_CMD_PARAM_1], "ver", strlen("ver"));
        if (cmp_res == 0) {
          char version[MAX_GPS_VERSION_DATALEN];
          print_retval(0 == altcom_gps_ver(version), APITEST_GETFUNCNAME(altcom_gps_ver));
          APITEST_DBG_PRINT("version:%s\n", version);
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
          cepResult_t res;

          altcom_gps_cep(atoi(command->argv[APITEST_CMD_PARAM_2]),
                         atoi(command->argv[APITEST_CMD_PARAM_3]), &res);

          APITEST_DBG_PRINT("result:%d\n", (int)res.result);
          if (atoi(command->argv[APITEST_CMD_PARAM_2]) == CEP_STATUS_CMD) {
            APITEST_DBG_PRINT("status:\nresult:%d\n", (int)res.result);
            APITEST_DBG_PRINT("days:%d\n", (int)res.days);
            APITEST_DBG_PRINT("hours:%d\n", (int)res.hours);
            APITEST_DBG_PRINT("minutes:%d\n", (int)res.minutes);
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
