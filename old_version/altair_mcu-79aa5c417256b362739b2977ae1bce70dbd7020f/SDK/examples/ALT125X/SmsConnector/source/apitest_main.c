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
#include "sms/altcom_sms.h"
#include "atcmd/altcom_atcmd.h"

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
static void print_api_test_help() {
  printf("\"atchnet\" : lte_attach_network()\n");
  printf("\"dtchnet\" : lte_detach_network()\n");
  printf("\"gtnetst\" : lte_get_netstat()\n");

  printf("\"smsinit\" : altcom_sms_initialize()\n");
  printf("\"smsfin\" : altcom_sms_finalize()\n");
  printf("\"smsdel\" : altcom_sms_delete()\n");
  printf("\"smsstrginfo\" : altcom_sms_get_storage_info()\n");
  printf("\"smsread\" : altcom_sms_read()\n");
  printf("\"smsgetlist\" : altcom_sms_get_list()\n");
  printf("\"smssend\" : altcom_sms_send()\n");
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

static void sms_print_time(struct altcom_sms_time_s *time) {
  APITEST_DBG_PRINT("%02d/%02d/%02d,%02d:%02d:%02d", time->year, time->mon, time->mday, time->hour,
                    time->min, time->sec);
  if (time->tz >= 0) {
    APITEST_DBG_PRINT("+");
  }
  APITEST_DBG_PRINT("%d\n", time->tz);

  if (time->year > 99) {
    APITEST_DBG_PRINT("[ERR] Years over.\n");
  }

  if (time->mon < 1) {
    APITEST_DBG_PRINT("[ERR] Month under.\n");
  }

  if (time->mon > 12) {
    APITEST_DBG_PRINT("[ERR] Month over.\n");
  }

  if (time->mday < 1) {
    APITEST_DBG_PRINT("[ERR] Day of the month under.\n");
  }

  if (time->mday > 31) {
    APITEST_DBG_PRINT("[ERR] Day of the month over.\n");
  }

  if (time->hour > 23) {
    APITEST_DBG_PRINT("[ERR] Hours over.\n");
  }

  if (time->min > 59) {
    APITEST_DBG_PRINT("[ERR] Minutes over.\n");
  }

  if (time->sec > 59) {
    APITEST_DBG_PRINT("[ERR] Seconds over.\n");
  }

  if (time->tz < -24) {
    APITEST_DBG_PRINT("[ERR] Time zone in hour under.\n");
  }

  if (time->tz > 24) {
    APITEST_DBG_PRINT("[ERR] Time zone in hour over.\n");
  }
}

static void sms_print_msg(struct altcom_sms_msg_s *msg) {
  int i = 0;

  APITEST_DBG_PRINT("[RECV] type : %02X\n", msg->type);

  if (msg->type == ALTCOM_SMS_MSG_TYPE_RECV) {
    APITEST_DBG_PRINT("[RECV] valid_indicator : %02X\n", msg->u.recv.valid_indicator);

    if (msg->u.recv.valid_indicator &
        ~(ALTCOM_SMS_MSG_VALID_UD | ALTCOM_SMS_MSG_VALID_CONCAT_HDR | ALTCOM_SMS_MSG_VALID_TOA)) {
      APITEST_DBG_PRINT("[ERR] Valid indicator invalid bit up.\n");
    }

    APITEST_DBG_PRINT("[RECV] sc_time : ");
    sms_print_time(&msg->u.recv.sc_time);

    // APITEST_DBG_PRINT("[RECV] sc_addr.length : %d\n", msg->sc_addr.);
    APITEST_DBG_PRINT("[RECV] sc_addr.toa : %02X\n", msg->sc_addr.toa);
    APITEST_DBG_PRINT("[RECV] sc_addr.address : %s\n", msg->sc_addr.address);

    if ((msg->u.recv.src_addr.toa & 0x80) == 0) {
      APITEST_DBG_PRINT("[ERR] TOA invalid format.\n");
    }

    APITEST_DBG_PRINT("[RECV] addr.toa : %02X\n", msg->u.recv.src_addr.toa);
    APITEST_DBG_PRINT("[RECV] addr.address : %s\n", msg->u.recv.src_addr.address);

    if ((msg->u.recv.valid_indicator & ALTCOM_SMS_MSG_VALID_CONCAT_HDR) != 0) {
      APITEST_DBG_PRINT("[RECV] concat_hdr.ref_num : %d\n", msg->u.recv.concat_hdr.ref_num);
      APITEST_DBG_PRINT("[RECV] concat_hdr.max_num : %d\n", msg->u.recv.concat_hdr.max_num);
      APITEST_DBG_PRINT("[RECV] concat_hdr.seq_num : %d\n", msg->u.recv.concat_hdr.seq_num);
    }
    if ((msg->u.recv.valid_indicator & ALTCOM_SMS_MSG_VALID_UD) != 0) {
      APITEST_DBG_PRINT("[RECV] userdata.data_len : %d\n", msg->u.recv.userdata.data_len);
      APITEST_DBG_PRINT("[RECV] userdata.chset : %d\n", msg->u.recv.userdata.chset);
      if (msg->u.recv.userdata.chset >= ALTCOM_SMS_MSG_CHSET_RESERVED) {
        APITEST_DBG_PRINT("[ERR] chset over.\n");
      }
      APITEST_DBG_PRINT("[RECV] userdata.data in hex : ");
      for (i = 0; i < msg->u.recv.userdata.data_len; i++) {
        APITEST_DBG_PRINT("%02X", msg->u.recv.userdata.data[i]);
        if (msg->u.recv.userdata.data[i] == 0) {
        }
      }
      APITEST_DBG_PRINT("\n");

      APITEST_DBG_PRINT("[RECV] userdata.data in hex : ");
      for (i = 0; i < msg->u.recv.userdata.data_len; i++) {
        APITEST_DBG_PRINT("%c", msg->u.recv.userdata.data[i]);
        if (msg->u.recv.userdata.data[i] == 0) {
        }
      }
      APITEST_DBG_PRINT("\n");
    }
  } else {
    if (msg->u.delivery_report.status & ~ALTCOM_SMS_DELIVERY_STAT_CAT_MASK) {
      APITEST_DBG_PRINT("[ERR] Status invalid bit up.\n");
    }
    APITEST_DBG_PRINT("[RECV] sc_time : ");
    sms_print_time(&msg->u.delivery_report.sc_time);
    APITEST_DBG_PRINT("[RECV] discharge_time : ");
    sms_print_time(&msg->u.delivery_report.discharge_time);
    APITEST_DBG_PRINT("[RECV] ref_id : %d\n", msg->u.delivery_report.ref_id);
    APITEST_DBG_PRINT("[RECV] status : %d\n", msg->u.delivery_report.status);
  }
}

static int sms_recv_cb(struct altcom_sms_msg_s *msg, uint16_t index, void *userPriv) {
  APITEST_DBG_PRINT("[CB_VAL] sms recv index: %u, priv: %p\n", index, userPriv);
  sms_print_msg(msg);
  return 0;
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
    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "atcmd", strlen("atcmd"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          char *resBuf = NULL;
          osStatus_t atRet;

          resBuf = malloc(ATCMD_MAX_CMD_LEN);
          if (NULL == resBuf) {
            APITEST_DBG_PRINT("malloc fail\n");
            break;
          }

          resBuf[0] = '\0';
          atRet = altcom_atcmdSendCmd(command->argv[APITEST_CMD_PARAM_1], ATCMDCFG_MAX_CMDRES_LEN,
                                      resBuf);
          if (0 <= atRet) {
            APITEST_DBG_PRINT("%s", resBuf);
          } else {
            APITEST_DBG_PRINT("\r\nERROR\r\n");
          }
        }
      }

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

      /* altcom_sms_initialize */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsinit", strlen("smsinit"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          print_retval(altcom_sms_initialize(sms_recv_cb, (void *)0xdeadbeef,
                                             (bool)atoi(command->argv[APITEST_CMD_PARAM_1])),
                       APITEST_GETFUNCNAME(altcom_sms_initialize));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_finalize */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsfin", strlen("smsfin"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          print_retval(altcom_sms_finalize(), APITEST_GETFUNCNAME(altcom_sms_finalize));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_delete */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsdel", strlen("smsdel"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          uint16_t index;
          uint8_t types;

          index = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          types = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_2]);
          print_retval(altcom_sms_delete(index, types), APITEST_GETFUNCNAME(altcom_sms_delete));
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_get_storage_info */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsstrginfo", strlen("smsstrginfo"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          struct altcom_sms_storage_info_s strg_info;
          smsresult_e result;
          print_retval(result = altcom_sms_get_storage_info(&strg_info),
                       APITEST_GETFUNCNAME(altcom_sms_get_storage_info));

          if (SMS_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] SMS GET STORAGE max_record : %d\n", strg_info.max_record);
            APITEST_DBG_PRINT("[RET_VAL] SMS GET STORAGE used_record : %d\n",
                              strg_info.used_record);
          }
        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_read */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsread", strlen("smsread"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          uint16_t index;
          static struct altcom_sms_msg_s *msg = NULL;
          smsresult_e result;
          static uint8_t *data;

          if (!msg) {
            msg = malloc(sizeof(struct altcom_sms_msg_s));
            assert(msg);
            data = malloc(ALTCOM_SMS_READ_USERDATA_MAXLEN * sizeof(uint8_t));
            assert(data);
            msg->u.recv.userdata.data = data;
            APITEST_DBG_PRINT("msg,data ptr allocated: %p,%p\n", msg, data);
          }

          index = (uint16_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          print_retval(result = altcom_sms_read(index, msg), APITEST_GETFUNCNAME(altcom_sms_read));
          if (SMS_RESULT_OK == result) {
            sms_print_msg(msg);
          }

        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_get_list */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smsgetlist", strlen("smsgetlist"));
      if (cmp_res == 0) {
        if (3 == command->argc) {
          smsresult_e result;
          uint8_t types;
          static struct altcom_sms_msg_list_s *list = NULL;
          uint16_t i;

          if (!list) {
            list = malloc(sizeof(struct altcom_sms_msg_list_s));
            assert(list);

            APITEST_DBG_PRINT("list ptr allocated: %p\n", list);
          }

          types = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_1]);
          print_retval(result = altcom_sms_get_list(types, list),
                       APITEST_GETFUNCNAME(altcom_sms_get_list));
          if (SMS_RESULT_OK == result) {
            APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST num : %d\n", list->num);
            for (i = 0; i < list->num; i++) {
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d type : %02X\n", i, list->msg[i].type);
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d ref_id : %d\n", i, list->msg[i].ref_id);
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d index : %d\n", i, list->msg[i].index);
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d index : sc_time : ", i);
              sms_print_time(&list->msg[i].sc_time);
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d sc_addr.toa : %02X\n", i,
                                list->msg[i].addr.toa);
              APITEST_DBG_PRINT("[RET_VAL] SMS GET LIST %d sc_addr.address : %s\n", i,
                                list->msg[i].addr.address);
            }
          }

        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* altcom_sms_send */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "smssend", strlen("smssend"));
      if (cmp_res == 0) {
        if (5 == command->argc) {
          smsresult_e result;
          static struct altcom_sms_msg_s *msg = NULL;
          uint8_t mr_list[ALTCOM_SMS_MSG_REF_ID_MAX_NUM] = {0};
          uint16_t i = 0;
          uint32_t mr_list_len;

          if (!msg) {
            msg = malloc(sizeof(struct altcom_sms_msg_s));
            APITEST_DBG_PRINT("msg ptr allocated: %p\n", msg);
            assert(msg);
          }

          msg->type = ALTCOM_SMS_MSG_TYPE_SEND;
          msg->u.send.valid_indicator = ALTCOM_SMS_MSG_VALID_UD;
          strncpy(msg->u.send.dest_addr.address, command->argv[APITEST_CMD_PARAM_1],
                  ALTCOM_SMS_ADDR_MAX_LEN);
          msg->u.send.userdata.chset = (uint8_t)atoi(command->argv[APITEST_CMD_PARAM_2]);
          msg->u.send.userdata.data_len = strlen(command->argv[APITEST_CMD_PARAM_3]);
          msg->u.send.userdata.data = (uint8_t *)command->argv[APITEST_CMD_PARAM_3];

          APITEST_DBG_PRINT("[VAL] SMS SEND to %d %s data_len %d data: %s\n",
                            strlen(msg->u.send.dest_addr.address), msg->u.send.dest_addr.address,
                            msg->u.send.userdata.data_len, msg->u.send.userdata.data);

          msg->sc_addr.address[0] = 0;

          print_retval(result = altcom_sms_send(msg, mr_list, &mr_list_len),
                       APITEST_GETFUNCNAME(altcom_sms_send));
          if (SMS_RESULT_OK == result) {
            for (i = 0; i < (uint16_t)mr_list_len; i++) {
              APITEST_DBG_PRINT("[RET_VAL] SMS SEND mr_list %d mr: %d\n", i, mr_list[i]);
            }
          }

        } else {
          APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
          goto errout_with_param;
        }
      }

      /* help */

      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "help", strlen("help"));
      if (cmp_res == 0) {
        if (2 == command->argc) {
          print_api_test_help();
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
