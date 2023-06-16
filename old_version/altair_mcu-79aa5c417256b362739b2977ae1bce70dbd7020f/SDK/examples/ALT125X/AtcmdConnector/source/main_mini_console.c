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

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* Middleware includes. */
#include "altcom.h"
#include "buffpoolwrapper.h"
#include "altcom_atcmd.h"
#include "mini_console.h"

/* App includes. */
#include "apitest_main.h"

#define CMD_NUM_OF_PARAM_MAX 12
#define CMD_PARAM_LENGTH 128

struct data_s {
  char *data;
  int16_t len;
};

static bool param_field_alloc(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    argv[i] = malloc(CMD_PARAM_LENGTH);
    if (!argv[i]) {
      printf("cmd field alloc failed.\n");
      return false;
    }
    memset(argv[i], 0, CMD_PARAM_LENGTH);
  }
  return true;
}

static void param_field_free(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    if (argv[i]) {
      free(argv[i]);
    }
  }
  return;
}

static int32_t parse_arg(char *s, char *argv[]) {
  int argc = 1; /* At least, include "apitest" or other */
  char *tmp_s;
  int i;

  /* Parse arg */
  if (strlen(s)) {
    for (i = 1; i < CMD_NUM_OF_PARAM_MAX; i++) {
      tmp_s = strchr(s, ' ');
      if (!tmp_s) {
        tmp_s = strchr(s, '\0');
        if (!tmp_s) {
          printf("Invalid arg.\n");
          break;
        }
        memcpy(argv[i], s, tmp_s - s);
        break;
      }
      memcpy(argv[i], s, tmp_s - s);
      s += tmp_s - s + 1;
    }
    argc += i;
  }

  printf("Number of arg = %d\n", argc);
  for (i = 0; i < argc; i++) {
    if (argv[i]) {
      printf("arg[%d] = %s\n", i, argv[i]);
    }
  }
  return argc;
}

int do_apitest(char *s) {
  int32_t argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};

  if (false == param_field_alloc(argv)) {
    goto exit;
  }

  memcpy(argv[0], "apitest", strlen("apitest"));

  argc = parse_arg(s, argv);
  apitest_main(argc, argv);
exit:
  param_field_free(argv);

  return 0;
}

int do_atcmd(char *s) {
  char c;
  struct data_s *atcmdData;
  int idx;

  if (true != atmode_start || NULL == atq) {
    printf("atmode not started\n");
    return 0;
  }

  printf("Send ATCMD to MAP, echo mode: %d (Press Ctrl+D to exit)\r\n", echo_enable);
  while (true) {
    idx = 0;
    atcmdData = malloc(sizeof(struct data_s));
    if (!atcmdData) {
      printf("malloc fail!\n");
      return 0;
    }

    atcmdData->data = malloc(ATCMD_MAX_CMD_LEN);
    if (!atcmdData->data) {
      printf("malloc fail!\n");
      free(atcmdData);
      return 0;
    }

    while (true) {
      console_read(&c, 1);
      if (echo_enable) {
        printf("%c", c);
      }

      if (c == 0xD) {
        atcmdData->data[idx] = '\0';
        atcmdData->len = idx;
        if (osOK != osMessageQueuePut(atq, (const void *)&atcmdData, 0, osWaitForever)) {
          printf("atcmd message send failed.\r\n");
          goto leave;
        }

        break;
      } else if (c == 0xA) {
        // just ignore LF
      } else if (c == 0x4) {
        char *nullCmd = NULL;
        if (osOK != osMessageQueuePut(atq, (const void *)&nullCmd, 0, osWaitForever)) {
          printf("atcmd message send failed.\r\n");
          goto leave;
        }

        goto leave;
      } else {
        atcmdData->data[idx++] = c;
      }
    }
  }

leave:
  free(atcmdData->data);
  free(atcmdData);
  printf("atmode stop.\r\n");

  return 0;
}

int do_altcom_buffpool_statistic(char *s) {
  BUFFPOOL_SHOW_STATISTICS();
  return 0;
}

int do_altcomlog(char *s) {
  int argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};

  if (false == param_field_alloc(argv)) {
    goto exit;
  }

  argc = parse_arg(s, argv);
  if (argc == 2) {
    dbglevel_e level = (dbglevel_e)strtol(argv[argc - 1], NULL, 10);

    if (level > ALTCOM_DBG_DEBUG) {
      printf("Invalid log level %lu!\n", (uint32_t)level);
    } else {
      altcom_set_log_level(level);
      printf("Set ALTCOM log level to %lu!\n", (uint32_t)level);
    }
  } else {
    printf("Invalid argument!\n");
  }

exit:
  param_field_free(argv);

  return 0;
}
