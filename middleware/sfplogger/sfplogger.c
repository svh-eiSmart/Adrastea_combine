
/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include "timers.h"

#include "portmacro.h"
#include "sfplogger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "timers.h"
#include "task.h"
#include "semphr.h"
#include "newlibPort.h"
#include "altcom_misc.h"
#include "altcom_osal_opt.h"

time_t gLtime;
unsigned int gLtimeMsec = 0;
bool gLtimeValid = false;

typedef void (*pdFLUSH_CALLBACK)(void *buff, void *arg);

#define CLI_PF(fmt, args...)                                         \
  snprintf((char *)pcWriteBuffer, xWriteBufferLen - 1, fmt, ##args); \
  ((pdFLUSH_CALLBACK)flush_func)(pcWriteBuffer, flush_arg);

#define WAIT_TIME_FOR_DICT_RELEASE 5
#define SFP_STREAM_BUFFER_SIZE 10000
#define POWER_MANAGEMENT_DB_SIZE 3000

int sfpDatabaseEnabled = 1;
char sfpEnableOutputToStream = 0;
char sfpEnableHibernateClear = 0;
char sfpEnableCommercial = 0;  // during init phase we are at debug mode , on FS init we will change
// to commercial if needed

char sfpPreventEntries = 0;
spflogdbentry_t sfpdbasearr[SFPLOGGER_MAX_NUM_DBS];
spflogmoduleentry_t sfplogmodarr[SFPLOG_MAX_MODULE_ID];
int sfpnumregistereddbs = 0;

char *pSfpStreamPrintBuffer = 0;
char *pSfpStreamWrite = 0;
char *pSfpStreamPrint = 0;
char *pSfpDictionaryLocation = SFP_DICTIONARY_LOCATION;
static unsigned int streambufferfull = 0;
static SemaphoreHandle_t sfpStreamSignal = 0;
TaskHandle_t sfpPrintThandle = 0;

#define SFP_ENTER_CRITICAL(dbid) \
  if (dbid != interrupt_db_id) { \
    portENTER_CRITICAL();        \
  }

#define SFP_EXIT_CRITICAL(dbid)  \
  if (dbid != interrupt_db_id) { \
    portEXIT_CRITICAL();         \
  }

// default databases IDs
int defaultInitDBid = -1;
int defaultDBid = -1;
int defaultAtDBid = -1;
int defaultEcmDBid = -1;
int mainMemoryDBid = -1;
int interrupt_db_id = -1;
int powerMngDBid = -1;

char *spfseverarr[SFPLOG_SEVERITY_NUM] = {"CRITICAL", "ERROR", "WARNING", "NORMAL",
                                          "INFO",     "DEBUG", "VERBOSE"};

char *spfprintseverarr[SFPLOG_SEVERITY_NUM] = {"[CRI]", "[ERR]", "[D2] ", "[D3] ",
                                               "[D4] ", "[D5] ", "[D6] "};

unsigned char sfplogpt[(SFPLOG_STRING_ENTRYHEADER_SIZE + SFPLOG_STRING_DATA_MAX_SIZE)] = {0};
unsigned char str[SFPLOG_STRING_INPUT_MAX_SIZE] = {0};

static int sfplogger_printf_entry(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                                  void *flush_arg, char *pentry, char *optstring);

static int sfp_add_db_id() {
  sfpnumregistereddbs++;

  if (sfpnumregistereddbs == SFPLOGGER_MAX_NUM_DBS) {
    printf("Exceeded maximal number of modules  - initialization failed\r\n");
    return -1;
  } else {
    return sfpnumregistereddbs - 1;
  }
}
void sfplogger_send_stream_to_output() {
  while (sfpEnableOutputToStream != 0) {
    xSemaphoreTake(sfpStreamSignal, portMAX_DELAY);

    char *currentwrite = pSfpStreamWrite;

    while (pSfpStreamPrint < currentwrite) {
      portENTER_CRITICAL();
      pSfpStreamPrint = currentwrite;
      if (currentwrite == pSfpStreamWrite) {
        pSfpStreamPrint = pSfpStreamWrite = pSfpStreamPrintBuffer;
      }
      currentwrite = pSfpStreamWrite;

      portEXIT_CRITICAL();
    }
  }

  pSfpStreamPrint = pSfpStreamWrite = 0;
  free(pSfpStreamPrintBuffer);
  pSfpStreamPrintBuffer = 0;

  vTaskDelete(NULL);
}
void sfplogger_osal_convert_time(unsigned int seconds, char *timestr, char strmaxsize) {
#if defined(__GNUC__)
  struct tm tm_struct;
  time_t local = (time_t)seconds;

  localtime_r((const time_t *)&local, &tm_struct);

  strftime(timestr, strmaxsize, "%b %d %H:%M:%S", &tm_struct);
#elif defined(__ICCARM__)
  struct tm *tm_struct;
  time_t local = (time_t)seconds;

  tm_struct = localtime((const time_t *)&local);

  strftime(timestr, strmaxsize, "%b %d %H:%M:%S", tm_struct);
#else
#error sfplogger_osal_convert_time function not supported in this toolchain
#endif
}

static int sfplogger_printf_entry(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                                  void *flush_arg, char *pentry, char *optstring) {
  unsigned int timetag;
  char timestr[20];
  unsigned int seconds;

  if (((*pentry) & SFP_MSB_STRING_MASK) == SFP_MSB_STRING_TYPE) {
    memcpy(&timetag, pentry + 2, 4);
    timetag = htonl(timetag);
    char lengthofstrin;
    memcpy(&lengthofstrin, pentry + 1, 1);
    if (gLtimeValid)
    {
        seconds = (unsigned int)gLtime  + ((timetag - gLtimeMsec) / 1000);
    }
    else
    {
        seconds = timetag / 1000;
    }
    sfplogger_osal_convert_time(seconds, timestr, 20);
    int sevirityval = *pentry & 0x7;

    // remove extra \r\n
    char *strptr = optstring == NULL ? pentry + SFPLOG_STRING_ENTRYHEADER_SIZE : optstring;
    int printedStringLength = strlen(strptr);

    if (printedStringLength != 0) {
      if (((strptr[printedStringLength - 1] == '\r') &&
           (strptr[printedStringLength - 2] == '\n')) ||
          ((strptr[printedStringLength - 1] == '\n') &&
           (strptr[printedStringLength - 2] == '\r'))) {
        strptr[printedStringLength - 2] = 0;
      }
    }

    if (pcWriteBuffer == 0) {
      printf("%s.%03d %s %s\r\n", timestr, timetag % 1000, spfprintseverarr[sevirityval],
             optstring == NULL ? pentry + SFPLOG_STRING_ENTRYHEADER_SIZE : optstring);
    } else {
      CLI_PF("%s.%03d %s %s\r\n", timestr, timetag % 1000, spfprintseverarr[sevirityval],
             optstring == NULL ? pentry + SFPLOG_STRING_ENTRYHEADER_SIZE : optstring);
    }
  }
  return 0;
}

unsigned int sfp_calc_hash_from_filename(void *filename) {
  int sindex = strlen((char *)filename) - 1;
  unsigned int hash = 1, a = 1, b = 0, num_slash = 0;

  while ((num_slash < 3) && (sindex >= 0)) {
    if (((char *)filename)[sindex] == '/') {
      num_slash += 1;
    }
    if ((((char *)filename)[sindex] != '.') && (((char *)filename)[sindex] != '/')) {
      a = (a + ((char *)filename)[sindex]) % 65521;
      b = (b + a) % 65521;
    }
    sindex -= 1;
  }

  hash = (b << 16) | a;
  return hash;
}

int sfp_log(int param1, int param2, int param3, unsigned int severitymoduleid,
            unsigned int line_num, void *pfilename) {
  unsigned int moduleid, data, entry_size = SFPLOG_ENTRY_SIZE;
  char *entryp, severity, sfplogproto[SFPLOG_EXTENDED_ENTRY_SIZE];
  int dbid = SFPLOG_ENTRY_SIZE;

  moduleid = severitymoduleid & 0xFFFFFFF;
  dbid = sfplogmodarr[moduleid].dbid;
  severity = (severitymoduleid & 0xF0000000) >> 28;

  if (severity > sfplogmodarr[moduleid].severity) return 0;

  if (dbid >= SFPLOGGER_MAX_NUM_DBS) {
    printf("Incorrect database id [%d]. %s failed\r\n", dbid, __func__);
    return -1;
  }

  if (line_num > 0x7ff) {
    entry_size = SFPLOG_EXTENDED_ENTRY_SIZE;
  }

  // Bits 20-30 - 11 bits of line number, bits 0-19 - filename pointer
  data = ((line_num & 0xFFF) << 20) |
         (sfp_calc_hash_from_filename(pfilename) &
          0xFFFFF);  // line number: upper 12 bits. hashed filename : lower 20 bits
  memcpy(sfplogproto, &data, 4);
  // MSB '0' bit indicates type of entry is SFP.
  *(sfplogproto) &= ~SFP_MSB_SFP_TYPE_MASK;

  // TODO
  data = xTaskGetTickCount();  // time in msec - will wrap around every 49.7 days
  memcpy(sfplogproto + 4, &data, 4);
  memcpy(sfplogproto + 8, &param1, 4);
  memcpy(sfplogproto + 12, &param2, 4);
  memcpy(sfplogproto + 16, &param3, 4);

  if (entry_size == SFPLOG_EXTENDED_ENTRY_SIZE) {
    memcpy(sfplogproto + 20, &line_num, 4);            // last entry holds line number (32 bits)
    *(sfplogproto) |= SFP_MSB_EXTENDED_SFP_TYPE;       // Set bit 7 - '1'
    *(sfplogproto) &= SFP_MSB_EXTENDED_SFP_TYPE_ZERO;  // Set bit 6 - '0'
  }
  // TODO add support of interrupt context
  if (sfplogmodarr[moduleid].printtostd != 0) {
    sfplogger_printf_entry(0, 0, 0, 0, sfplogproto, NULL);
  }

  if (entry_size > sfpdbasearr[dbid].buffer_size) {
    return -1;
  }

  if (sfpdbasearr[dbid].sfpdbp == 0) {
    if (sfpDatabaseEnabled == 1) {
      printf("Module id [%d] message buffer is not allocated. %s failed\r\n", dbid, __func__);
    }
    return -1;
  }

  /* clearing not allowed if new entry to log is taking place*/
  sfpdbasearr[dbid].lock = 1;  // signal to clearing/resizing that log is currently written

  // thread safe access to sfp logger binary bin
  if (0 == sfpPreventEntries) {
    SFP_ENTER_CRITICAL(dbid)
    entryp = (char *)sfpdb_get_next_entry(&(sfpdbasearr[dbid]), entry_size);
    SFP_EXIT_CRITICAL(dbid)

    memcpy(entryp, sfplogproto, entry_size);
  }

  sfpdbasearr[dbid].lock = 0;  // signal to clearing/resizing that log write completed

  return 0;
}

int sfp_log_string(const char *format, ...) {
  va_list argp;
  int ret;

  va_start(argp, format);
  ret = vsnprintf((char *)str, SFPLOG_STRING_INPUT_MAX_SIZE - 1, format, argp);
  va_end(argp);

  if (ret >= 0 && ret < SFPLOG_STRING_INPUT_MAX_SIZE - 1) {
    str[ret + 1] = '\0';
  }

  sfp_log_str(SFPLOG_DBG_PRINT, SFPLOG_INFO, (char *)str);

  return (altcom_SendAppLog(sfpdbasearr->buffer_size, (char *)sfpdbasearr->sfpdbp));
  ;
}

int sfp_log_formatted(SFP_log_module_id_t moduleid, char severity, const char *format, ...) {
  va_list argp;
  int ret;

  va_start(argp, format);
  ret = vsnprintf((char *)str, SFPLOG_STRING_INPUT_MAX_SIZE - 1, format, argp);
  va_end(argp);

  if (ret >= 0 && ret < SFPLOG_STRING_INPUT_MAX_SIZE - 1) {
    str[ret + 1] = '\0';
  }

  sfp_log_str(moduleid, severity, (char *)str);

  return 0;
}

int sfp_log_str(SFP_log_module_id_t moduleid, char severity, char *logstring) {
  char clength, *entryp;
  unsigned int timedata;
  int dbid;

  dbid = sfplogmodarr[moduleid].dbid;

  char moduleNameLen = strlen(sfplogmodarr[moduleid].modulename);
  moduleNameLen = moduleNameLen > 11 ? 11 : moduleNameLen;  //

  if (severity > sfplogmodarr[moduleid].severity) return 0;

  if (dbid >= SFPLOGGER_MAX_NUM_DBS) {
    printf("Incorrect database id [%d]. %s failed\r\n", dbid, __func__);
    return -1;
  }

  unsigned int entrylength = strlen(logstring) + 1;   // leave extra byte for null terminator
  entrylength = entrylength > 80 ? 80 : entrylength;  // limit size to 255

  *sfplogpt =
      SFP_MSB_STRING_TYPE | (severity & 0x7);  // upper 4 bits is type and lower 3 is severity

  // TODO take time from timer0 timer 1
  timedata = htonl(xTaskGetTickCount());  // time in msec - will wrap around every 49.7 days

  memcpy((char *)sfplogpt + 2, &timedata, 4);

  if (moduleNameLen > 0) {
    entrylength += 2 + moduleNameLen;
    entrylength = entrylength > 80 ? 80 : entrylength;  // limit size to 255
    snprintf((char *)sfplogpt + 6, SFP_MODULE_NAME_LEN + 2, "[%s]",
             sfplogmodarr[moduleid].modulename);
    memcpy((char *)sfplogpt + 8 + moduleNameLen, logstring, entrylength - (moduleNameLen + 2));
  } else {
    memcpy((char *)sfplogpt + 6, logstring, entrylength);
  }

  clength = (char)entrylength;
  memcpy(sfplogpt + 1, &clength, 1);

  // insure null termination in case string is larger than 80
  if (clength == 80)
    sfplogpt[SFPLOG_STRING_ENTRYHEADER_SIZE + (SFPLOG_STRING_DATA_MAX_SIZE - 2)] = 0;

  if (sfplogmodarr[moduleid].printtostd != 0) {
    sfplogger_printf_entry(0, 0, 0, 0, (char *)sfplogpt, logstring);
  }

  if (sfpdbasearr[dbid].sfpdbp == 0) {
    if (sfpDatabaseEnabled == 1) {
      printf("Module id [%d] message buffer is not allocated. %s failed\r\n", dbid, __func__);
    }
    return -1;
  }

  if (entrylength > sfpdbasearr[dbid].buffer_size) {
    return -1;
  }

  /* clearing not allowed if new entry to log is taking place*/
  sfpdbasearr[dbid].lock = 1;  // signal to clearing/resizing that log is currently written

  // thread safe access to sfp logger binary bin

  if (0 == sfpPreventEntries) {
    SFP_ENTER_CRITICAL(dbid);
    entryp = (char *)sfpdb_get_next_entry(&(sfpdbasearr[dbid]),
                                          entrylength + SFPLOG_STRING_ENTRYHEADER_SIZE);
    SFP_EXIT_CRITICAL(dbid);
    // sfpPreventEntries--;
    memcpy(entryp, sfplogpt, entrylength + SFPLOG_STRING_ENTRYHEADER_SIZE);
  }

  sfpdbasearr[dbid].lock = 0;  // signal to clearing/resizing that log write completed

  return 0;
}
static void print_db_buffer(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                            void *flush_arg, spflogdbentry_t *plogdb) {
  int delta = 0, wrap = 0;
  char *entryp = sfpdb_get_earliest_entry(plogdb);
  char *start = entryp;

  while (((wrap == 0) && (entryp < (plogdb->sfpdbp + plogdb->buffer_size))) ||
         ((wrap == 1) && (start > entryp))) {
    if ((entryp[0] == 0) && (entryp[1] == 0)) {  // empty end reached - exit
      return;
    } else if (((*entryp) & SFP_MSB_EMPTY_FILLER_TYPE_MASK) ==
               SFP_MSB_EMPTY_FILLER_TYPE)  // Empty filler entry
    {
      int fillersize =
          ((((*entryp) & (~(char)SFP_MSB_EMPTY_FILLER_TYPE_MASK))) * 256) + (*(entryp + 1));
      delta = fillersize;
    } else if (((*entryp) & SFP_MSB_SFP_TYPE_MASK) == 0)  // Regular SFP log entry type
    {
      sfplogger_printf_entry(pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg, entryp, NULL);
      delta = SFPLOG_ENTRY_SIZE;
    } else if (((*entryp) & SFP_MSB_EXTENDED_SFP_TYPE_MASK) ==
               SFP_MSB_EXTENDED_SFP_TYPE)  // Extended SFP log entry type
    {
      sfplogger_printf_entry(pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg, entryp, NULL);
      delta = SFPLOG_EXTENDED_ENTRY_SIZE;
    } else if (((*entryp) & SFP_MSB_STRING_MASK) == SFP_MSB_STRING_TYPE)  // String SFP log entry
    {
      sfplogger_printf_entry(pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg, entryp, NULL);
      delta = entryp[1] + SFPLOG_STRING_ENTRYHEADER_SIZE;
    } else if (((*entryp) & SFP_MSB_COMPLEX_STRING_MASK) ==
               SFP_MSB_COMPLEX_STRING_TYPE)  // String SFP log entry
    {
      sfplogger_printf_entry(pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg, entryp, NULL);
      delta = entryp[1] + SFPLOG_COMPLEX_STRING_ENTRYHEADER_SIZE_PLUS_NULL_TERM;
    } else {
      if (pcWriteBuffer != 0) {
        CLI_PF("ERROR: %s: Unknown sfp DB entry: [0x%x]\r\n", __func__, *entryp);
      }
      delta = plogdb->buffer_size;
    }
    entryp = delta + entryp;
    if (entryp >= (plogdb->sfpdbp + plogdb->buffer_size)) {
      entryp = plogdb->sfpdbp;
      wrap = 1;
    }
  }
}

void sfplogger_dump_hex(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                        void *flush_arg) {
  int i, j;

  for (i = 0; i < SFPLOGGER_MAX_NUM_DBS; i++) {
    if (sfpdbasearr[i].sfpdbp != 0) {
      CLI_PF("\nSFP Logger - Dumping buffer[%d]\r\n", i);
      int delta = 0, wrap = 0, line_length = 0;
      char *entryp = sfpdb_get_earliest_entry(&(sfpdbasearr[i]));
      char *start = entryp;

      while (((wrap == 0) && (entryp < (sfpdbasearr[i].sfpdbp + sfpdbasearr[i].buffer_size))) ||
             ((wrap == 1) && (start > entryp))) {
        if ((entryp[0] == 0) && (entryp[1] == 0)) {  // empty end reached - exit
          break;
        } else if (((*entryp) & SFP_MSB_EMPTY_FILLER_TYPE_MASK) ==
                   SFP_MSB_EMPTY_FILLER_TYPE)  // Empty filler entry
        {
          delta = ((((*entryp) & (~SFP_MSB_EMPTY_FILLER_TYPE_MASK))) * 256) + (*(entryp + 1));
        } else if (((*entryp) & SFP_MSB_SFP_TYPE_MASK) == 0)  // Regular SFP log entry type
        {
          delta = SFPLOG_ENTRY_SIZE;
        } else if (((*entryp) & SFP_MSB_EXTENDED_SFP_TYPE_MASK) ==
                   SFP_MSB_EXTENDED_SFP_TYPE)  // Extended SFP log entry type
        {
          delta = SFPLOG_EXTENDED_ENTRY_SIZE;
        } else if (((*entryp) & SFP_MSB_STRING_MASK) ==
                   SFP_MSB_STRING_TYPE)  // String SFP log entry
        {
          delta = entryp[1] + SFPLOG_STRING_ENTRYHEADER_SIZE;
        } else if (((*entryp) & SFP_MSB_COMPLEX_STRING_MASK) ==
                   SFP_MSB_COMPLEX_STRING_TYPE)  // String SFP log entry
        {
          delta = entryp[1] + SFPLOG_COMPLEX_STRING_ENTRYHEADER_SIZE_PLUS_NULL_TERM;
        } else {
          if (pcWriteBuffer != 0) {
            CLI_PF("ERROR: %s: Unknown sfp DB entry: [0x%x]\r\n", __func__, *entryp);
          }
          break;
        }

        for (j = 0; j < delta; j++) {
          CLI_PF("%02x", entryp[j]);
          line_length += 2;
          if (line_length > 128) {
            CLI_PF("\r\n");
            line_length = 0;
          }
        }

        CLI_PF("::");
        line_length += 2;
        if (line_length > 128) {
          CLI_PF("\r\n");
          line_length = 0;
        }

        entryp = delta + entryp;
        if (entryp >= (sfpdbasearr[i].sfpdbp + sfpdbasearr[i].buffer_size)) {
          entryp = sfpdbasearr[i].sfpdbp;
          wrap = 1;
        }
      }
    }
  }
}

void sfplogger_print_buffer_locked(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                                   void *flush_arg, void *psbufferid) {
  SFP_LOG_WARNING(SFPLOG_DBG_PRINT, "SFP user print start - Locking internal database");

  sfpPreventEntries = 1;

  if (*(char *)psbufferid == 'h') {
    sfplogger_dump_hex((char *)pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg);
  } else {
    // TODO
    sfplogger_print_buffer((char *)pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg,
                           psbufferid);
  }

  sfpPreventEntries = 0;

  // SFP_LOG_WARNING(SFPLOG_DEFAULT, "SFP user print exit - Releasing internal database");
}

void sfplogger_print_buffer(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                            void *flush_arg, void *psbufferid) {
  unsigned int j, bufferid, lastprintid = 0;
  unsigned char i;
  if (*(char *)psbufferid == 'a') {
    if (pcWriteBuffer != 0) {
      if (1 == sfpDatabaseEnabled) {
        CLI_PF("SFP Logger - Printing all buffers\r\n");
      } else {
        CLI_PF("SFP Logger Internal Database Disabled\r\n");
      }
    }
    bufferid = 0;
    lastprintid = SFPLOGGER_MAX_NUM_DBS - 1;
  } else {
    bufferid = atoi(psbufferid);
    lastprintid = bufferid;

    if (bufferid >= (SFPLOGGER_MAX_NUM_DBS - 1)) {
      if (pcWriteBuffer != 0) {
        CLI_PF("Incorrect module id [%d]. %s failed\r\n", bufferid, __func__);
      }
      return;
    }

    if (sfpdbasearr[bufferid].sfpdbp == 0) {
      if (pcWriteBuffer != 0) {
        if (sfpDatabaseEnabled == 1) {
          CLI_PF("Module id [%d] message buffer is not allocated. %s failed\r\n", bufferid,
                 __func__);
        }
      }
      return;
    }
  }

  for (i = bufferid; i <= lastprintid; i++) {
    if (sfpdbasearr[i].sfpdbp != 0) {
      if (pcWriteBuffer != 0) {
        CLI_PF("SFP Buffer id [%d]. Registered modules:", i);
        for (j = 0; j < SFPLOG_MAX_MODULE_ID; j++) {
          if (sfplogmodarr[j].dbid == i) {
            CLI_PF(" %s", sfplogmodarr[j].modulename);
          }
        }
        CLI_PF("\r\n");
      }

      print_db_buffer(pcWriteBuffer, xWriteBufferLen, flush_func, flush_arg, &(sfpdbasearr[i]));
    }
  }
}

void sfplogger_print_registered_modules(char *pcWriteBuffer, size_t xWriteBufferLen,
                                        void *flush_func, void *flush_arg) {
  unsigned int i, totalbufferssize = 0;

  CLI_PF(
      "\r\nModule name | Database id (dbid) | Module ID | Severity | Print to screen | Size "
      "(Bytes)\n\r");

  for (i = 0; i < SFPLOG_MAX_MODULE_ID; i++) {
    if (sfplogmodarr[i].dbid != (char)SFP_DBID_ILLEGAL) {
      CLI_PF("%-12s           %2d            %2d        %-8s        %3s           %d\n\r",
             sfplogmodarr[i].modulename, sfplogmodarr[i].dbid, i,
             spfseverarr[(int)sfplogmodarr[i].severity],
             sfplogmodarr[i].printtostd != 0 ? "YES" : "NO",
             sfpdbasearr[(unsigned int)sfplogmodarr[i].dbid].buffer_size);
    }
  }
  for (i = 0; i < SFPLOGGER_MAX_NUM_DBS; i++) {
    totalbufferssize += sfpdbasearr[i].buffer_size;
  }

  CLI_PF("\r\nTotal allocated buffers size: %d Bytes\r\n", totalbufferssize);
  CLI_PF("\r\nInternal database [%s]\r\n", sfpDatabaseEnabled == 1 ? "Enabled" : "Disabled");

  CLI_PF("\r\nPrint to stream [%s]\r\n", sfpEnableOutputToStream == 0 ? "Disabled" : "Enabled");
  if (sfpEnableOutputToStream != 0) {
    CLI_PF("\r\nNumber of lost stream logs [%d]\r\n", streambufferfull);
  }
}

void sfplogger_clear_buffers(char *buffer_to_clear, char *pcWriteBuffer, size_t xWriteBufferLen,
                             void *flush_func, void *flush_arg) {
  int first = 0, end = SFPLOGGER_MAX_NUM_DBS, i;
  CLI_PF("\r\nClearing buffer id [%s]\r\n", buffer_to_clear);
  if (buffer_to_clear[0] != 'a') {
    first = atoi(buffer_to_clear);
    if (first >= SFPLOGGER_MAX_NUM_DBS - 1) return;
    end = first + 1;
  }
  for (i = first; i < end; i++) {
    // Insure clear of database is not in middle of obtaining an entry in the database
    while (sfpdbasearr[i].lock != 0) vTaskDelay(1);
    portENTER_CRITICAL();
    memset(sfpdbasearr[i].sfpdbp, 0, sfpdbasearr[i].buffer_size);
    sfpdbasearr[i].next_entry_delta = 0;
    portEXIT_CRITICAL();
  }
}

int sfplogger_set_module_print_state(SFP_log_module_id_t moduleid, char printstate,
                                      int write_conf_to_file) {
  if (moduleid >= SFPLOG_MAX_MODULE_ID) {
    printf("%s failed to set print to stdout for module id[%d]\r\n", __func__, moduleid);
    return (-1);
  }
  // on commercial mode ignore this command
  if ((sfpEnableCommercial == 1) && (sfplogmodarr[moduleid].dbid != defaultAtDBid) &&
      (sfplogmodarr[moduleid].dbid != defaultEcmDBid)) {
    sfplogmodarr[moduleid].printtostd = 0;
    return (-2);
  }

  if(sfplogmodarr[moduleid].printtostd != printstate){
    sfplogmodarr[moduleid].printtostd = printstate;
  }
  return 0;
}

char sfplogger_get_module_print_state(SFP_log_module_id_t moduleid) {
  if (moduleid >= SFPLOG_MAX_MODULE_ID) {
    printf("%s failed to get print state for module id[%d]\r\n", __func__, moduleid);
    return 0;
  }
  return sfplogmodarr[moduleid].printtostd;
}

int sfplogger_set_module_severity(SFP_log_module_id_t moduleid, char severity,
                                   int write_conf_to_file) {
  if (moduleid >= SFPLOG_MAX_MODULE_ID) {
    printf("%s failed to set severity for module id[%d]\r\n", __func__, moduleid);
    return (-1);
  }
  if (severity >= SFPLOG_SEVERITY_NUM) {
    printf("%s failed to set severity for module id[%d] severity[%d]\r\n", __func__, moduleid,
           severity);
    return (-2);
  }

  // on commercial mode ignore this command
  if ((sfpEnableCommercial == 1) && (sfplogmodarr[moduleid].dbid != defaultAtDBid) &&
      (sfplogmodarr[moduleid].dbid != defaultEcmDBid)) {
    sfplogmodarr[moduleid].severity = SFPLOG_ERROR;
    return (-3);
  }

  if(sfplogmodarr[moduleid].severity != severity){
    sfplogmodarr[moduleid].severity = severity;
  }
  return 0;
}

char sfplogger_get_module_severity(SFP_log_module_id_t moduleid) {
  if (moduleid >= SFPLOG_MAX_MODULE_ID) {
    printf("%s failed to get severity for module id[%d]\r\n", __func__, moduleid);
    return SFPLOG_SEVERITY_NUM;
  }
  return sfplogmodarr[moduleid].severity;
}

int sfplogger_register_moduletodb(SFP_log_module_id_t moduleid, char dbid, char *modulename) {
  if ((moduleid >= SFPLOG_MAX_MODULE_ID) || (dbid >= SFPLOGGER_MAX_NUM_DBS)) {
    if (sfpDatabaseEnabled == 1) {
      printf("%s failed to register module id[%d] with Database id[%d]\r\n", __func__, moduleid,
             dbid);
    }
    return -1;
  }

  sfplogmodarr[moduleid].dbid = dbid;
  memcpy(sfplogmodarr[moduleid].modulename, modulename, SFP_MODULE_NAME_LEN - 1);

  return 0;
}

int sfplogger_get_module_names(char *buffer, int buffersize) {
  int i = 0, totalwritten = 0;
  if (buffer == 0) {
    return -1;
  }

  for (i = 0; i < SFPLOG_MAX_MODULE_ID; i++) {
    if (sfplogmodarr[i].modulename[0] != 0) {
      if (buffersize - totalwritten < 0) {
        return -1;
      }
      totalwritten += snprintf(&(buffer[totalwritten]), buffersize - totalwritten, "%s ",
                               sfplogmodarr[i].modulename);
      if ((buffersize - totalwritten) < 0) {
        return -1;
      }
    }
  }

  return totalwritten - 1;  // remove last whitespace
}

void sfplogger_get_module_severity_and_name(int moduleId, char **mduleName, char **severity) {
  *mduleName = sfplogmodarr[moduleId].modulename;
  *severity = &(sfplogmodarr[moduleId].severity);
}

int sfplogger_get_severity_by_name(char **mduleName, char **severity) {
  int i;

  for (i = 0; i < SFPLOG_MAX_MODULE_ID; i++) {
    if (strncmp(*mduleName, sfplogmodarr[i].modulename, SFP_MODULE_NAME_LEN) == 0) {
      *severity = &(sfplogmodarr[i].severity);
      return 0;
    }
  }
  return -1;
}

int sfplogger_set_severity_by_name(char **mduleName, char severity) {
  int i;

  if (severity >= SFPLOG_SEVERITY_NUM) {
    return -1;
  }

  for (i = 0; i < SFPLOG_MAX_MODULE_ID; i++) {
    if (strncmp(*mduleName, sfplogmodarr[i].modulename, SFP_MODULE_NAME_LEN) == 0) {
      break;
    }
  }

  if (i == SFPLOG_MAX_MODULE_ID) {
    return -1;
  }

  sfplogmodarr[i].severity = severity;

  return 0;
}

int sfplogger_create_db(int size) {
  int ret;

  // on commercial mode use default db
  if (sfpEnableCommercial == 1) {
    return (sfplogger_get_default_db_id());
  }

  ret = sfp_add_db_id();

  if ((ret >= 0) && (sfpDatabaseEnabled == 1)) {
    sfpdbasearr[ret].sfpdbp = malloc(size);
    if (sfpdbasearr[ret].sfpdbp == 0) {
      printf("%s failed to allocate buffer\r\n", __func__);
      ret = -1;
    } else {
      memset(sfpdbasearr[ret].sfpdbp, 0, size);
      sfpdbasearr[ret].buffer_size = size;
    }
  }

  return ret;
}

int sfplogger_resize_db(int moduleid, unsigned int size, int write_conf_to_file) {
  int ret = 0;
  unsigned int dbid;

  if (sfpDatabaseEnabled == 0) {
    return -1;
  }
  if (moduleid >= SFPLOG_MAX_MODULE_ID) {
    printf("%s failed to register module id[%d]\r\n", __func__, moduleid);
    return -1;
  }

  if (sfplogmodarr[moduleid].dbid >= SFPLOGGER_MAX_NUM_DBS) {
    printf("%s failed to register module id[%d] with Database id[%d]\r\n", __func__, moduleid,
           sfplogmodarr[moduleid].dbid);
    return -1;
  }

  dbid = sfplogmodarr[moduleid].dbid;

  if (size <
      SFPLOG_RESIZE_MINIMAL_DB_SIZE) {  // Don't allow buffer size less than the minimal db size
    char *placeholder = sfpdbasearr[dbid].sfpdbp;
    sfpdbasearr[dbid].sfpdbp = 0;
    sfpdbasearr[dbid].buffer_size = 0;

    free(placeholder);
    return 0;
  }

  char *new_db_buffer = malloc(size);

  if (new_db_buffer == 0) {
    printf("%s failed to allocate buffer\r\n", __func__);
    ret = -1;
  } else {
    // Insure resize of database is not in middle of obtaining an entry in the database
    while (sfpdbasearr[dbid].lock != 0) vTaskDelay(1);
    portENTER_CRITICAL();
    memset(new_db_buffer, 0, size);
    // Copy content into new buffer only if bigger
    if (size > sfpdbasearr[dbid].buffer_size) {
      memcpy(new_db_buffer, sfpdbasearr[dbid].sfpdbp, sfpdbasearr[dbid].buffer_size);
    } else {
      sfpdbasearr[dbid].next_entry_delta = 0;
    }
    sfpdbasearr[dbid].buffer_size = size;

    free(sfpdbasearr[dbid].sfpdbp);
    sfpdbasearr[dbid].sfpdbp = new_db_buffer;
    portEXIT_CRITICAL();
  }

  return ret;
}

int sfplogger_save_buffer_in_db(unsigned int dbid, char *binaryBuff, unsigned int size) {
  if (dbid >= SFPLOGGER_MAX_NUM_DBS) {
    return -1;
  }

  if (sfpdbasearr[dbid].buffer_size >= size) {
    memset(sfpdbasearr[dbid].sfpdbp, 0, sfpdbasearr[dbid].buffer_size);
    memcpy(sfpdbasearr[dbid].sfpdbp, binaryBuff, size);
  } else {
    return -1;
  }

  return 0;
}

int sfp_log_init() {
  int i;

  for (i = 0; i < SFPLOGGER_MAX_NUM_DBS; i++) {
    sfpdbasearr[i].next_entry_delta = 0;
    sfpdbasearr[i].sfpdbp = 0;
    sfpdbasearr[i].lock = 0;
  }

  for (i = 0; i < SFPLOG_MAX_MODULE_ID; i++) {
    sfplogmodarr[i].dbid = SFP_DBID_ILLEGAL;
    sfplogmodarr[i].severity = SFPLOG_CRITICAL;
    memset(sfplogmodarr[i].modulename, 0, SFP_MODULE_NAME_LEN);
    sfplogmodarr[i].printtostd = 0;
  }
  // Use large size for init/boot client. later on after boot end minimize it.
  defaultInitDBid = sfplogger_create_db(SFPLOG_DB_SIZE);
  sfplogger_register_moduletodb(SFPLOG_TRACE, defaultInitDBid, "APP_TRACE");
  sfplogger_set_module_severity(SFPLOG_TRACE, SFPLOG_DEBUG, 0);
  sfplogger_set_module_print_state(SFPLOG_TRACE, 1, 0);

  sfplogger_register_moduletodb(SFPLOG_DBG_PRINT, defaultInitDBid, "DEBUG_PRINT");
  sfplogger_set_module_severity(SFPLOG_DBG_PRINT, SFPLOG_DEBUG, 0);
  sfplogger_set_module_print_state(SFPLOG_DBG_PRINT, 1, 0);

  sfplogger_register_moduletodb(SFPLOG_APP_ASSERT, defaultInitDBid, "APP_ASSERT");
  sfplogger_set_module_severity(SFPLOG_APP_ASSERT, SFPLOG_WARNING, 0);
  sfplogger_set_module_print_state(SFPLOG_APP_ASSERT, 1, 0);

  sfplogger_register_moduletodb(SFPLOG_DBG_ERROR, defaultInitDBid, "ERROR");
  sfplogger_set_module_severity(SFPLOG_DBG_ERROR, SFPLOG_DEBUG, 0);
  sfplogger_set_module_print_state(SFPLOG_DBG_ERROR, 1, 0);

  sfplogger_register_moduletodb(SFPLOG_ASSERT_ISR, defaultInitDBid, "INTERRUPT");
  sfplogger_set_module_severity(SFPLOG_ASSERT_ISR, SFPLOG_INFO, 0);

  /*Register power management module to power management data base*/
  sfplogger_register_moduletodb(SFPLOG_POWER_MNG, defaultInitDBid, "PWRMNG");
  sfplogger_set_module_severity(SFPLOG_POWER_MNG, SFPLOG_INFO, 0);

  sfplogger_register_moduletodb(SFPLOG_ALTCOM, defaultInitDBid, "ALTCOM");
  sfplogger_set_module_severity(SFPLOG_ALTCOM, SFPLOG_NORMAL, 0);
  sfplogger_set_module_print_state(SFPLOG_ALTCOM, 1, 0);

  return 0;
}

int sfplogger_send_AppLog() {
  return (altcom_SendAppLog(sfpdbasearr->buffer_size, (char *)sfpdbasearr->sfpdbp));
}

int sfplogger_get_default_db_id() { return defaultDBid; }

int sfplogger_get_at_db_id() { return defaultAtDBid; }
int sfplogger_get_ecm_db_id() { return defaultEcmDBid; }

int sfplogger_get_is_commercial_mode() { return sfpEnableCommercial; }

char sfplogger_get_db_id_by_module_id(char module_id) { return sfplogmodarr[(int)module_id].dbid; }
