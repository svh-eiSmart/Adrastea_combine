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

/* Driver includes */
#include "DRV_PM.h"

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "altcom_errno.h"
#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "net/altcom_netdb.h"

/* Middleware includes */
#include "mini_console.h"
#include "pwr_mngr.h"

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

#define NETCAT_RECV_BUF_SIZE 128

/****************************************************************************
 * External Functions
 ****************************************************************************/
extern void start_tcpclient(char *ip, uint16_t port);
extern void start_tcpserver(bool is_v6, uint16_t port);
extern void start_udpclient(char *ip, uint16_t port);
extern void start_udpserver(bool is_v6, uint16_t port);

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
 * Public Data
 ****************************************************************************/

osMessageQueueId_t ncq = NULL;
bool netcat_start = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_retval(int32_t val, char *str) {
  if (0 != val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%ld\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%ld\"\n", str, val);
  }
}

void show_addrinfo(struct altcom_addrinfo *info) {
  struct altcom_addrinfo *tmp = info;

  while (tmp) {
    APITEST_DBG_PRINT(
        "ai_flags: %d, ai_family: %d, ai_protocol: %d\nai_socktype:%d, ai_addrlen:%lu\n",
        tmp->ai_flags, tmp->ai_family, tmp->ai_protocol, tmp->ai_socktype, tmp->ai_addrlen);
    if (tmp->ai_addrlen) {
      APITEST_DBG_PRINT("sa_len: %lu, sa_family: %lu\n", (uint32_t)tmp->ai_addr->sa_len,
                        (uint32_t)tmp->ai_addr->sa_family);
      uint8_t i;

      for (i = 0; i < tmp->ai_addr->sa_len; i++) {
        APITEST_DBG_PRINT("%X ", (unsigned int)tmp->ai_addr->sa_data[i]);
      }

      APITEST_DBG_PRINT("\n");
    }

    APITEST_DBG_PRINT("canonical name: %s\n", tmp->ai_canonname);
    tmp = tmp->ai_next;
  }
}

void netcat_recv_task(void *p) {
  char recv_buf[NETCAT_RECV_BUF_SIZE];
  int read_bytes;
  int sock = (int)p;

  APITEST_DBG_PRINT("nctask_%d start\n", sock);
  while (1) {
    /* read a max of RECV_BUF_SIZE bytes from socket */
    if ((read_bytes = altcom_read(sock, recv_buf, NETCAT_RECV_BUF_SIZE)) < 0) {
      APITEST_DBG_PRINT("altcom_read fail on sockfd %d, ret: %d, errno: %ld\n", sock, read_bytes,
                        altcom_errno());
      break;
    }

    console_write(recv_buf, read_bytes);
    if (netcat_start == false) {
      APITEST_DBG_PRINT("netcat terminated\n");
      break;
    }
  }

  APITEST_DBG_PRINT("nctask_%d stop\n", sock);
  osThreadExit();
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
      cmp_res = strncmp(command->argv[APITEST_CMD_ARG], "netcat", strlen("netcat"));
      if (cmp_res == 0) {
        if (4 == command->argc) {
          int sockfd;
          int sock_ret;
          char nctaskname[32];
          char c;
          osStatus_t ncret;
          struct altcom_sockaddr_storage ss;
          struct altcom_sockaddr_in *addr4 = (struct altcom_sockaddr_in *)&ss;
          struct altcom_sockaddr_in6 *addr6 = (struct altcom_sockaddr_in6 *)&ss;
          bool is_v6 = false;

          memset(&ss, 0, sizeof(ss));
          if (altcom_inet_pton(ALTCOM_AF_INET, command->argv[APITEST_CMD_PARAM_1],
                               (void *)&addr4->sin_addr) == 1) {
            is_v6 = false;
            addr4->sin_family = ALTCOM_AF_INET;
            addr4->sin_port = altcom_htons(atoi(command->argv[APITEST_CMD_PARAM_2]));
          } else if (altcom_inet_pton(ALTCOM_AF_INET6, command->argv[APITEST_CMD_PARAM_1],
                                      (void *)&addr6->sin6_addr) == 1) {
            is_v6 = true;
            addr6->sin6_family = ALTCOM_AF_INET6;
            addr6->sin6_port = altcom_htons(atoi(command->argv[APITEST_CMD_PARAM_2]));
          } else {
            APITEST_DBG_PRINT("Invalid IP string: \"%s\"\r\n", command->argv[APITEST_CMD_PARAM_1]);
            goto errout_with_param;
          }

          /* open socket */
          sockfd = altcom_socket((is_v6 ? ALTCOM_AF_INET6 : ALTCOM_AF_INET), ALTCOM_SOCK_STREAM,
                                 ALTCOM_IPPROTO_TCP);
          print_retval(sockfd < 0, APITEST_GETFUNCNAME(altcom_socket));
          if (sockfd >= 0) {
            /* connect */
            sock_ret = altcom_connect(sockfd, (struct altcom_sockaddr *)&ss,
                                      sizeof(struct altcom_sockaddr));
            print_retval(sock_ret != 0, APITEST_GETFUNCNAME(altcom_connect));
            if (0 != sock_ret) {
              sock_ret = altcom_close(sockfd);
              print_retval(0 != sock_ret, APITEST_GETFUNCNAME(altcom_close));
              goto errout_with_param;
            }

            osThreadAttr_t attr = {0};

            snprintf(nctaskname, sizeof(nctaskname), "nctask_%d", sockfd);
            attr.name = nctaskname;
            attr.stack_size = APITEST_TASK_STACKSIZE;
            attr.priority = APITEST_TASK_PRI;

            osThreadId_t tid = osThreadNew(netcat_recv_task, (void *)sockfd, &attr);

            if (!tid) {
              APITEST_DBG_PRINT("osThreadNew failed\n");
              break;
            }

            APITEST_DBG_PRINT("Use the CLI command \"ncmsg\" to send message.\r\n");
            netcat_start = true;
            do {
              ncret = osMessageQueueGet(ncq, (void *)(&c), 0, osWaitForever);
              if (ncret != osOK) {
                APITEST_DBG_PRINT("osMessageQueueGet ncq fail[%d]\n", ncret);
                break;
              }

              if (4 == c) {
                break;
              }
              sock_ret = altcom_write(sockfd, (void *)&c, 1);
              if (1 != sock_ret) {
                print_retval(1 != sock_ret, APITEST_GETFUNCNAME(altcom_write));
                break;
              }
            } while (c != 4);

            netcat_start = false;
            print_retval(altcom_close(sockfd), APITEST_GETFUNCNAME(altcom_close));
            APITEST_DBG_PRINT("netcat Closed.\r\n");
          }
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "tcp", strlen("tcp"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "client", strlen("client"))) {
          if (5 == command->argc) {
            start_tcpclient(command->argv[APITEST_CMD_PARAM_2],
                            atoi(command->argv[APITEST_CMD_PARAM_3]));
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "server", strlen("server"))) {
          if (4 == command->argc) {
            bool is_v6 = false;

            if (strstr(command->argv[APITEST_CMD_PARAM_1], "6")) {
              is_v6 = true;
            }

            start_tcpserver(is_v6, atoi(command->argv[APITEST_CMD_PARAM_2]));
          }
        } else {
          APITEST_DBG_PRINT("iperf tcp <client ip port|server(6) port>\n");
          goto errout_with_param;
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "udp", strlen("udp"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "client", strlen("client"))) {
          if (5 == command->argc) {
            start_udpclient(command->argv[APITEST_CMD_PARAM_2],
                            atoi(command->argv[APITEST_CMD_PARAM_3]));
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "server", strlen("server"))) {
          if (4 == command->argc) {
            bool is_v6 = false;

            if (strstr(command->argv[APITEST_CMD_PARAM_1], "6")) {
              is_v6 = true;
            }

            start_udpserver(is_v6, atoi(command->argv[APITEST_CMD_PARAM_2]));
          }
        } else {
          APITEST_DBG_PRINT("iperf udp <client ip port|server(6) port>\n");
          goto errout_with_param;
        }
      }

      if (!strncmp(command->argv[APITEST_CMD_ARG], "dns", strlen("dns"))) {
        if (3 == command->argc) {
          struct altcom_addrinfo hints, *res;
          int result;

          memset(&hints, 0, sizeof(struct altcom_addrinfo));
          hints.ai_family = ALTCOM_AF_INET;   /* Allow IPv4 */
          hints.ai_flags = ALTCOM_AI_PASSIVE; /* For wildcard IP address */
          hints.ai_protocol = 0;              /* Any protocol */
          hints.ai_socktype = ALTCOM_SOCK_STREAM;
          result = altcom_getaddrinfo(command->argv[APITEST_CMD_PARAM_1], NULL, &hints, &res);
          print_retval(result != 0, APITEST_GETFUNCNAME(altcom_getaddrinfo));
          if (result == 0) {
            show_addrinfo(res);
            altcom_freeaddrinfo(res);
          }

          hints.ai_family = ALTCOM_AF_INET6; /* Allow IPv6 */
          result = altcom_getaddrinfo(command->argv[APITEST_CMD_PARAM_1], NULL, &hints, &res);
          print_retval(result != 0, APITEST_GETFUNCNAME(altcom_getaddrinfo));
          if (result == 0) {
            show_addrinfo(res);
            altcom_freeaddrinfo(res);
          }
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

    ncq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(char), NULL);
    if (!cmdq) {
      APITEST_DBG_PRINT("cmdq init failed\n");
      goto errout;
    }
  } else {
    APITEST_DBG_PRINT("App already initialized\n");
  }

  if (pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0) != PWR_MNGR_OK) {
    APITEST_DBG_PRINT("pwr_mngr_conf_set_mode failed\n");
    goto errout;
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

  if (ncq) {
    osMessageQueueDelete(ncq);
    ncq = NULL;
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
