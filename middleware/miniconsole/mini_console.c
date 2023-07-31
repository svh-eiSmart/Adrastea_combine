/****************************************************************************
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

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "serial.h"
#include "newlibPort.h"
#include "hifc_api.h"
#include "serial_altDbg.h"
#include "timex.h"
#include "mini_console.h"
#include "125X_mcu.h"

#if defined(__GNUC__)
#define CTIME_R(time, buf) ctime_r(time, buf)
#elif defined(__ICCARM__)
#define CTIME_R(time, buf) !ctime_s(buf, sizeof(buf), time)
#else
#error time function not supported in this toolchain
#endif

typedef int cli_function_t(char *);

/* CLI functions declarations */
#define DECLARE_COMMAND(name, func, help) cli_function_t func;
#include "main_mini_console_commands.h"
#undef DECLARE_COMMAND

/* CLI functions array */
#define DECLARE_COMMAND(name, func, help) &func,
cli_function_t *cli_functions[] = {
#include "main_mini_console_commands.h"
    NULL};
#undef DECLARE_COMMAND

/* CLI function names array */
#define DECLARE_COMMAND(name, func, help) name,
char *cli_names[] = {
#include "main_mini_console_commands.h"
    NULL};
#undef DECLARE_COMMAND

/* CLI function names array */
#define DECLARE_COMMAND(name, func, help) help,
char *cli_help[] = {
#include "main_mini_console_commands.h"
    NULL};
#undef DECLARE_COMMAND

/* Priorities at which the tasks are created. */
#define mainQUEUE_SEND_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

/* The rate at which data is sent to the queue.  The 200ms value is converted
to ticks using the portTICK_PERIOD_MS constant. */
#define mainQUEUE_SEND_FREQUENCY_MS (200 / portTICK_PERIOD_MS)

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH (1)

static serial_handle *sHandle = NULL;

#define CMDBUF_LENGTH 160

// History buffer
#define GETS_HISTORY_DEPTH 5
/* Configuration of CLI UART port. Default is external UART0 and can override in
 * mini_console_commands.h of each project.*/
#ifndef MINICONSOLE_UART_INSTANCE
#define MINICONSOLE_UART_INSTANCE ACTIVE_UARTF0
#endif
static char gets_history[GETS_HISTORY_DEPTH][CMDBUF_LENGTH] = {0};
static int gets_history_len[GETS_HISTORY_DEPTH] = {0};
static int gets_history_inx = 0;

extern const char ver_string[];

TaskHandle_t gConsoleHandle = NULL;
/*-----------------------------------------------------------*/

static void prvMiniConsoleTask(void *pvParameters);
/*-----------------------------------------------------------*/
void mini_console() { mini_console_ext(1); }

void mini_console_ext(int show_header) {
  xTaskCreate(prvMiniConsoleTask, /* The function that implements the task. */
              "Console", /* The text name assigned to the task - for debug only as it is not used by
                            the kernel. */
              configMINIMAL_STACK_SIZE * 2, /* The size of the stack to allocate to the task. */
              (void *)show_header,          /* The parameter passed to the task */
              configMAX_PRIORITIES - 1, /* The priority assigned to the task - minimal priority. */
              &gConsoleHandle);                    /* The task handle is not required, so NULL is passed. */
}

/*-----------------------------------------------------------*/
void mini_console_terminate(){
  if( gConsoleHandle != NULL ){
    vTaskDelete( gConsoleHandle );
  }
}

/*-----------------------------------------------------------*/
void print_menu_header() {
  do_ver(NULL);
  printf("\r\n");

  printf("MCU menu -- %s\r\n", MCU_PROJECT_NAME);
  printf("========\r\n");
}

void print_help_menu() {
  int n;

  printf("help or ? - Print this help menu\r\n");
  for (n = 0; cli_help[n] != NULL; n++) {
    printf(cli_help[n]);
    printf("\r\n");
  }
}

static char pcWriteBuffer[1024];

uint32_t console_write(char *buf, uint32_t len) {
  if (!sHandle) {
    return 0;
  }

  return serial_write(sHandle, buf, len);
}

uint32_t console_read(char *buf, uint32_t len) {
  if (!sHandle) {
    return 0;
  }

  return serial_read(sHandle, buf, len);
}

/* Reports basic memory info
 * To be expanded when needs rise up */
int do_mem_info(char *s) {
  printf("Free memory: %d\r\n", xPortGetFreeHeapSize());
#if defined(__GNUC__)
  extern double newlib_malloc_fragmentation();
  printf("Memory fragmentation: %.2f%%\n", newlib_malloc_fragmentation() * 100);
#endif
  return 0;
}

static void prvMiniConsoleTask(void *pvParameters) {
  int n, w;
  int commandfound = 0;
  char s[CMDBUF_LENGTH];
  int show_header = (int)pvParameters;

  sHuart uartInit;
  if (serial_load_defconfig(MINICONSOLE_UART_INSTANCE, &uartInit) != 0) {
    /*UARTF0 default configuration error*/
    while (1) {
      ;
    }
  }

  sHandle = serial_open(&uartInit);
  newlib_set_stdout_port(MINICONSOLE_UART_INSTANCE);
  serial_write(sHandle, "Welcome to MiniConsole\r\n", strlen("Welcome to MiniConsole\r\n"));


  if (show_header) {
    print_menu_header();
    print_help_menu();
  }


 for(int i=0;i<1;i++)
 {
	 //AdrasteaExamplesPrint("yes",1);

  for(int i=0;i<25;i++)
  {
  printf("doing temp read from TIDS iteration %d ", i);
  do_tids("temp");
  }
  printf(do_tids("id"));

  printf("\n");
  printf("\n");


  for(int i=0;i<25;i++)
   {
	  printf("doing temp read from ITDS iteration %d ", i);
	  do_itds("temp");
	  printf("doing accel read from ITDS iteration %d ", i);
	  do_itds("accel");
   }
  printf( do_itds("id"));

   printf("\n");
   printf("\n");


   for(int i=0;i<25;i++)
    {
 	  printf("doing temp read from PADS iteration %d ", i);
 	  do_pads("temp");
    }
   printf(do_pads("id"));
   printf("\n");
   printf("\n");
  }





  for (;;) {

    /*  To get here something must have been received from the queue */
    commandfound = 0;

    printf(">> ");
    serial_gets(s, CMDBUF_LENGTH, cli_names);

    if ((s[0] == '?') || (strncmp("help", s, 4)) == 0) {
      print_help_menu();
      commandfound = 1;
    } else {
      for (n = 0; cli_names[n] != NULL; n++) {
        w = strlen(cli_names[n]);
        if ((strncmp(cli_names[n], s, w) == 0) && ((s[w] == ' ') || (s[w] == 0))) {
          s[strlen(s) + 1] = 0;
          if (cli_functions[n](s + 1 + w) != 0) {  // Print help
            printf(cli_help[n]);
            printf("\r\n");
          }

          commandfound = 1;
          break;
        }
      }
    }

    if (!commandfound) {
      printf("unknown command (type ? for help): %s\r\f", s);
    }

  }
}
/*-----------------------------------------------------------*/

int leagal_address(unsigned int address) {  // Check is address is leagal
  address = address & 0xcfffffff;
  if (address <= 4096) {
    return 0;  // ROM
  }

  if ((address >= 0x20000) && (address < 0x40000)) {
    return 0;  // RAM
  }

  if ((address >= 0x100000) && (address < 0x106000)) {
    return 0;  // Peripherals
  }

  if ((address >= 0x200000) && (address < 0x250000)) {
    return 0;  // Peripherals
  }

  if ((address >= 0x1000000) && (address < 0x1300000)) {
    return 0;  // Peripherals
  }

  if ((address >= 0x1400000) && (address < 0x1700000)) {
    return 0;  // Peripherals
               //  if ((address>=0x4000000) && (address<0x8000000))
               //    return 0; // SF1
  }

  if ((address >= 0x8000000) && (address < 0xc000000)) {
    return 0;  // PMP SF
  }

  if ((address >= 0xc000000) && (address < 0xc100000)) {
    return 0;  // Retention memory
  }

  if ((address >= 0xc110000) && (address < 0xc120000)) {
    return 0;  // REGGPM
  }

  if ((address >= 0xd000000) && (address < 0xd040000)) {
    return 0;  // PMP
  }

  return -1;
}

int do_read(char *s) {
  unsigned int n;

  sscanf(s, "%x", &n);
  if (leagal_address(n) == 0) {
    printf("%08lx\r\n", REGISTER(n));
    return 0;
  } else {
    return -1;
  }
}

int do_write(char *s) {
  unsigned int n, w;

  sscanf(s, "%x %x", &n, &w);
  if (leagal_address(n) == 0) {
    REGISTER(n) = w;
    return 0;
  } else {
    return -1;
  }
}

int do_ps(char *s) {
  vTaskList((char *)pcWriteBuffer);
  printf("Task name    Status    Prio   Pid   StackSize   StackMax  StackCur\r\n");
  printf(pcWriteBuffer);
  return 0;
}

int do_ver(char *s) {
  printf(
      "\r\n%s\r\n"
      "%s Compiled on "__DATE__
      " at "__TIME__
      "\r\n",
      ver_string, MCU_PROJECT_NAME);
  return 0;
}

int do_serialinfo(char *s) {
#define MAX_PARAM (2)
  char *pch;
  int argc = 0;
  char *argv[MAX_PARAM];  // two parameters at most
  int ret = 1;

  argv[argc++] = "serialinfo";
  pch = strtok(s, " ");
  while (pch != NULL) {
    if (argc == MAX_PARAM) {
      return ret;
    }

    argv[argc++] = pch;
    pch = strtok(NULL, " ");
  }

  cliSerialStat_cmd(argc, argv);

  return 0;
}

static void hifc_print_stat() {
  struct hifc_status_report rep;

  get_hifc_status_report(&rep);
  printf("----HIFC Status Report----\r\n");
  switch (rep.eMode) {
    case hifc_mode_a:
      printf("Mode: A\r\n");
      break;

    case hifc_mode_b:
      printf("Mode: B\r\n");
      break;

    case hifc_mode_c:
      printf("Mode: C\r\n");
      break;

    case hifc_mode_off:
      printf("Mode: Off\r\n");
      break;

    default:
      printf("Mode: Unknown\r\n");
  }

  switch (rep.eIf_state) {
    case IfUp:
      printf("HIFC state: IfUp\r\n");
      break;

    case IfDown:
      printf("HIFC state: IfDown\r\n");
      break;

    case IfSuspending:
      printf("HIFC state: IfSuspending\r\n");
      break;

    case IfResuming:
      printf("HIFC state: IfResuming\r\n");
      break;

    case IfMdmResuming:
      printf("HIFC state: IfMdmResuming\r\n");
      break;

    default:
      printf("HIFC state: Undefined\r\n");
  }

  printf("Hw flow control: %s\r\n", rep.hw_flow_ctl ? "Enabled" : "Disabled");

  printf("Host Resume Timeout: %lu\r\n", rep.hres_timeout);
  printf("Host Suspend Timeout: %lu\r\n", rep.hsusp_timeout);
  printf("Modem Resume Timeout: %lu\r\n", rep.mres_timeout);

  printf("Host Resume request: %lu\r\n", rep.hres_req);
  printf("Host Resume complete: %lu\r\n", rep.hres_comp);

  printf("Host suspend request: %lu\r\n", rep.hsus_req);
  printf("Host suspend complete: %lu\r\n", rep.hsus_comp);

  printf("Modem Resume request: %lu\r\n", rep.mres_req);
  printf("Modem Resume complete: %lu\r\n", rep.mres_comp);

  printf("Inactivity Time: %lu\r\n", rep.inactivity_time);

  printf("H2M pin: %lu\r\n", rep.h2m_value);
  printf("M2H pin: %lu\r\n", rep.m2h_value);
}

int do_hifc(char *s) {
#define MAX_PARAM (2)
  char *pch;
  int argc = 0;
  char *argv[MAX_PARAM];  // two parameters at most
  int ret = 1;
  uint32_t inactivity_timer;

  pch = strtok(s, " ");
  while (pch != NULL) {
    if (argc == MAX_PARAM) {
      return ret;
    }

    argv[argc++] = pch;
    pch = strtok(NULL, " ");
  }

  switch (argc) {
    case 1:
      if (!strncmp(argv[0], "status", strlen("status"))) {
        hifc_print_stat();
        ret = 0;
      } else if (!strncmp(argv[0], "res", strlen("res"))) {
        force_host_resume();
        ret = 0;
      } else if (!strncmp(argv[0], "sus", strlen("sus"))) {
        force_host_suspend();
        ret = 0;
      }

      break;

    case 2:
      if ((!strncmp(argv[0], "status", strlen("status"))) &&
          (!strncmp(argv[1], "clr", strlen("clr")))) {
        clear_hifc_status_report();
        ret = 0;
      }

      else if (!strncmp(argv[0], "timer", strlen("timer"))) {
        inactivity_timer = atoi(argv[1]);
        if (set_inactivity_timer(inactivity_timer)) {
          printf("Failed to set HIFC inactivity timer to %lu\r\n", inactivity_timer);
          ret = 1;
        } else {
          printf("Set HIFC inactivity timer to %lu\r\n", inactivity_timer);
          ret = 0;
        }
      }

      break;
  }

  if (ret) {
    printf("Command Argument Error!\r\n");
  }

  return ret;
}

void replace_line(char **dst, char **p, char **l, char *rep_str, int m) {
  int n;
  char esc = 27;

  // Clear line
  for (n = 0; n < (*p - *dst); n++) {  // Move back cursor
    serial_write(sHandle, &esc, 1);
    serial_write(sHandle, "[D", strlen("[D"));
  }

  for (n = 0; n < (*l - *dst); n++) {
    serial_write(sHandle, " ", 1);
  }

  strncpy(*dst, rep_str, m);  // Copy replacement string to dst

  for (n = 0; n < (*l - *dst); n++) {  // Move back cursor
    serial_write(sHandle, &esc, 1);
    serial_write(sHandle, "[D", strlen("[D"));
  }

  // print history on screen
  for (n = 0; n < m; n++) {
    serial_write(sHandle, &rep_str[n], 1);
  }

  *l = *dst + m;  // Set last position
  *p = *l;        // Set current position
}

char *serial_gets(char *dst, int max, char *names[]) {
  char c = 0, m;
  char *p;  // Current cursor position
  char *l;  // Last char position
  char *t;
  int tab_location;
  int name_location = 0;
  int name_found = 0;
  int n;
  char esc = 27;

  /* get max bytes or upto a newline */
  l = dst;
  tab_location = 0;

  for (p = dst, max--; max > 0; max--) {
    if (serial_read(sHandle, &c, 1) && c == 0) {
      break;
    }

    if (c == '\t')  // auto complete
    {
      if (tab_location == 0) {     // First tab was pressed after some other char
        tab_location = (p - dst);  // Set locaiton
        name_location = 0;         // Start search from beginning
        name_found = 0;
      }

      // Search for name
      do {
        if (strncmp(names[name_location], dst, tab_location) == 0) {  // Found one
          name_found = 1;
          // Clear line
          for (n = 0; n < (p - dst); n++) {  // Move back cursor
            serial_write(sHandle, &esc, 1);
            serial_write(sHandle, "[D", strlen("[D"));
          }

          for (n = 0; n < (l - dst); n++) {
            // serial_putc_f(' ');
            serial_write(sHandle, " ", 1);
          }

          m = strlen(names[name_location]);
          strncpy(dst, names[name_location], m);  // Copy full name to dst
          for (n = 0; n < (l - dst); n++) {       // Move back cursor
            serial_write(sHandle, &esc, 1);
            serial_write(sHandle, "[D", strlen("[D"));
          }

          // print full name on screen
          printf("%s", names[name_location]);
          l = dst + m;  // Set last position
          p = l;        // Set current position
          name_location++;
          if ((names[name_location]) == NULL) {
            name_location = 0;
          }

          break;
        }
        name_location++;
        if ((names[name_location]) == NULL) {
          name_location = 0;
          if (name_found == 0) {
            break;
          }
        }
      } while (1);
    } else {
      tab_location = 0;
      if (c == 8) {  // backspace
        if (p > dst) {
          serial_write(sHandle, &esc, 1);
          serial_write(sHandle, "[D", strlen("[D"));

          for (t = p; t < l; t++) {
            m = *t;
            *(t - 1) = m;
            serial_write(sHandle, &m, 1);
          }
          // serial_putc_f(' ');
          serial_write(sHandle, " ", 1);
          for (t = p; t <= l; t++) {  // Move back cursor
            // serial_putc_f(27);
            // serial_putc_f('[');
            // serial_putc_f('D');
            serial_write(sHandle, &esc, 1);
            serial_write(sHandle, "[D", strlen("[D"));
          }

          p--;
          l--;
        }
      } else if (c == 27)  // Escape sequence identified
      {
        serial_read(sHandle, &c, 1);
        if (c == '[') {  // Arrow sequence identified
          serial_read(sHandle, &c, 1);
          switch (c) {
            case 'C':       // Cursor Forward
              if (l > p) {  // Move forward
                serial_write(sHandle, &esc, 1);
                serial_write(sHandle, "[C", strlen("[C"));
                p++;
              }

              break;

            case 'D':  // Cursor Back
              if (p > dst) {
                serial_write(sHandle, &esc, 1);
                serial_write(sHandle, "[D", strlen("[D"));
                p--;
              }

              break;

            case 'B':  // Cursor Down
              n = gets_history_inx;
              do {
                gets_history_inx++;
                if (gets_history_inx == GETS_HISTORY_DEPTH) {
                  gets_history_inx = 0;
                }

                m = gets_history_len[gets_history_inx];
              } while ((m == 0) && (n != gets_history_inx));
              replace_line(&dst, &p, &l, gets_history[gets_history_inx], m);
              break;

            case 'A':  // Cursor Up
              n = gets_history_inx;
              do {
                gets_history_inx--;
                if (gets_history_inx < 0) {
                  gets_history_inx = GETS_HISTORY_DEPTH - 1;
                }

                m = gets_history_len[gets_history_inx];
              } while ((m == 0) && (n != gets_history_inx));
              replace_line(&dst, &p, &l, gets_history[gets_history_inx], m);
              break;
          }
        }
      } else  // receive non-control charactor
      {
        if (c != '\r' && c != '\n') {
          // serial_putc_f(c);
          serial_write(sHandle, &c, 1);
          if (p == l) {
            *p = c;
          } else {  // cursor is not at the end of the line. shift right hand side of p to one char
            for (t = p; t < l; t++) {
              m = *t;
              *t = c;
              c = m;
              serial_write(sHandle, &m, 1);
            }
            *t = c;
            for (t = p; t < l; t++) {  // Move back cursor
              serial_write(sHandle, &esc, 1);
              serial_write(sHandle, "[D", strlen("[D"));
            }
          }

          p++;
          l++;
        } else if (c == '\n') {
          ;
        } else {
          *l = c;
          serial_write(sHandle, "\r\n", strlen("\r\n"));
          break;
        }
      }
    }
  }

  *l = 0;
  m = l - dst;
  if (m > 0) {
    gets_history_len[gets_history_inx] = m;
    strncpy(gets_history[gets_history_inx], dst, gets_history_len[gets_history_inx]);
    gets_history_inx++;
    if (gets_history_inx == GETS_HISTORY_DEPTH) {
      gets_history_inx = 0;
    }
  }

  if (p == dst || c == 0) {
    return 0;
  }

  return (p);
}

int do_time(char *s) {
#define MAX_TIME_PARAM (3)
  char *pch;
  int argc = 0;
  char *argv[MAX_TIME_PARAM];  // two parameters at most
  int ret = 1;

  argv[argc++] = "time";
  pch = strtok(s, " ");
  while (pch != NULL) {
    if (argc == MAX_TIME_PARAM) {
      return ret;
    }

    argv[argc++] = pch;
    pch = strtok(NULL, " ");
  }

  if (argc < 2) {
    return -1;
  }

#define MAX_TIMESTR (64)
  time_t local_time, upt;
  char timeStr[MAX_TIMESTR];

  if (!strncmp(argv[1], "set", strlen("set"))) {
    if (3 == argc) {
      local_time = strtoll(argv[2], NULL, 10);
      printf("Set \"%s\" to local_time\n", argv[2]);
      stime(&local_time);
    } else {
      printf("unexpected parameter number %d\n", argc);
      return -1;
    }
  }

  if (!strncmp(argv[1], "show", strlen("show"))) {
    if (2 == argc) {
      unsigned int updays, uphours, upmins, upseconds, uptotalsec;

      time(&local_time);
      if (CTIME_R(&local_time, timeStr)) {
        printf("Local time: %s", timeStr);
      } else {
        printf("Something wrong in local_time or timeStr\n");
      }

      uptime(&upt);
      uptotalsec = upt;
      updays = uptotalsec / 86400;
      uphours = (uptotalsec - (updays * 86400)) / 3600;
      upmins = (uptotalsec - (updays * 86400) - (uphours * 3600)) / 60;
      upseconds = (uptotalsec - (updays * 86400) - (uphours * 3600) - (upmins * 60));
      snprintf(timeStr, sizeof(timeStr), "%d %s %2d:%02d:%02d:%02d", updays,
               updays == 1 ? "day" : "days", updays, uphours, upmins, upseconds);
      printf("Up time: %s\n", timeStr);
    } else {
      printf("unexpected parameter number %d\n", argc);
      return -1;
    }
  }

  return 0;
}

int do_reset(char *s) {
  if (strlen(s) == 3 && !strncmp(s, "all", 3)) {
    printf("Reset All Cores\r\n");
    machine_reset(MACHINE_RESET_CAUSE_USER);
  } else {
    printf("Reset MCU\r\n");
    NVIC_SystemReset();
  }
  return 0;
}
