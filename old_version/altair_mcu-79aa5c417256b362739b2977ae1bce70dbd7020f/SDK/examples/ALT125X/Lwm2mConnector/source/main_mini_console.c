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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Middleware includes. */
#include "altcom.h"
#include "buffpoolwrapper.h"
#include "sfplogger.h"

/* Middleware includes. */
#include "apitest_main.h"

#define CMD_NUM_OF_PARAM_MAX 12
#define CMD_PARAM_LENGTH 128

#define CONFIG_MAX_SIZE_OF_CLI_OUTPUT_BUFFER (8192)
static char pcWriteBuffer[CONFIG_MAX_SIZE_OF_CLI_OUTPUT_BUFFER];
int xWriteBufferLen = CONFIG_MAX_SIZE_OF_CLI_OUTPUT_BUFFER;

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

static int parse_arg(char *s, char *argv[]) {
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
  int argc;
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

int do_altcom_buffpool_statistic(char *s) {
  BUFFPOOL_SHOW_STATISTICS();
  return 0;
}

unsigned long long _strtoull(char *str, char **p1, int d) {
  unsigned long long retval = 0;
  int i = 0;

  if (d == 0 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) i = 2, d = 16;
  if (d == 0 && (str[0] == 'h' || str[0] == 'H')) i = 1, d = 16;
  if (d == 0 && (str[0] >= '0' && str[0] <= '9')) i = 0, d = 10;

  for (;; i++) {
    if (str[i] >= '0' && str[i] <= '9')
      retval = retval * d + (str[i] - '0');
    else if (str[i] >= 'a' && str[i] <= 'f')
      retval = retval * d + (str[i] - 'a' + 10);
    else if (str[i] >= 'A' && str[i] <= 'F')
      retval = retval * d + (str[i] - 'A' + 10);
    else {
      if (p1 != NULL) *p1 = &str[i];
      break;
    }
  }
  return retval;
}

unsigned int _xatou(char *str) {
  unsigned int retval = 0;
  if (str != NULL) retval = _strtoull(str, NULL, 0);
  return retval;
}

void flush_printf(void *buf, void *arg) {
  if (CONFIG_MAX_SIZE_OF_CLI_OUTPUT_BUFFER <= strlen(buf)) {
    printf("%s CLI Buffer overrun\n", __func__);
  }
  printf("%s", (char *)buf);
  *((char *)buf) = '\0';
}

int do_slog(char *s) {
  int argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};

  if (false == param_field_alloc(argv)) {
    goto exit3;
  }
  argc = parse_arg(s, argv);
  if (argc > 1) {
    if (((char *)argv[1])[0] == 'm') {
      // slog m : Show module information
      sfplogger_print_registered_modules((char *)pcWriteBuffer, sizeof(pcWriteBuffer),
                                         (void *)flush_printf, (void *)NULL);
    }

    else if (((char *)argv[1])[0] == 'c')  // clear either all or specific buffer
    {
      // slog c <0-30> : Clear dbid or all when no dbid is specified
      char buffer_to_clear[4] = {'a', 0, 0, 0};  // default is all buffers
      if (argc == 3) strncpy(buffer_to_clear, argv[2], 3);
      sfplogger_clear_buffers(buffer_to_clear, (char *)pcWriteBuffer, sizeof(pcWriteBuffer),
                              (void *)flush_printf, (void *)NULL);
    } else if ((((char *)argv[1])[0] == 'p') && (argc == 4)) {
      // slog p <module id> <0/1> : Set print state
      sfplogger_set_module_print_state((SFP_log_module_id_t)atoi(argv[2]), (char)atoi(argv[3]), 1);
    } else if ((((char *)argv[1])[0] == 's') && (argc == 4)) {
      // slog s <module id> <0-6> : Set module severity
      sfplogger_set_module_severity((SFP_log_module_id_t)atoi(argv[2]), (char)atoi(argv[3]), 1);
    } else if ((((char *)argv[1])[0] == 'r') && (argc == 4))  // resize database
    {
      // slog r <module id> <new size>: Resize module db size
      sfplogger_resize_db((SFP_log_module_id_t)atoi(argv[2]), _xatou(argv[3]), 1);
      printf("SFP resizing module id[%d] to size[%d]\r\n", atoi(argv[2]), _xatou(argv[3]));
    } else if (((char *)argv[1])[0] == 'h')  // dump hex data
    {
      // slog  h : Dump all dbs in hex
      sfplogger_dump_hex((char *)pcWriteBuffer, sizeof(pcWriteBuffer), (void *)flush_printf,
                         (void *)NULL);
    } else if ((((char *)argv[1])[0] == 'l') && (argc == 3)) {
      sfplogger_print_buffer_locked((char *)pcWriteBuffer, sizeof(pcWriteBuffer),
                                    (void *)flush_printf, (void *)NULL, argv[2]);
    } else if (argc == 2) {
      sfplogger_print_buffer((char *)pcWriteBuffer, sizeof(pcWriteBuffer), (void *)flush_printf,
                             (void *)NULL, argv[1]);
    } else {
      printf("Usage: slog <m>|<c>|<p>|<s>|<r>|<h>|<l> <num>\r\n");
    }
  } else {
    printf("Usage: slog <m>|<c>|<p>|<s>|<r>|<h>|<l> <num>\r\n");
  }

exit3:
  param_field_free(argv);

  return 0;
}

#ifndef __ALTCOM_USE_SFP_LOG__
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
#endif