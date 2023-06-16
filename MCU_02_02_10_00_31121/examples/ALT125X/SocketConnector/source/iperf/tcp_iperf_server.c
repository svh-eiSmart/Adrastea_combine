/*
 * Copyright (C) 2018 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "altcom_errno.h"
#include "iperf.h"

static struct perf_stats server;
extern const struct altcom_in6_addr in6addr_any;

/* Interval time in seconds */
#define REPORT_INTERVAL_TIME (INTERIM_REPORT_INTERVAL * 1000)

static void print_tcp_conn_stats(int sock, struct altcom_sockaddr *from) {
  struct altcom_sockaddr_storage local;
  struct altcom_sockaddr_in *local4 = (struct altcom_sockaddr_in *)&local,
                            *from4 = (struct altcom_sockaddr_in *)from;
  struct altcom_sockaddr_in6 *local6 = (struct altcom_sockaddr_in6 *)&local,
                             *from6 = (struct altcom_sockaddr_in6 *)from;
  altcom_socklen_t size = sizeof(local);
  char ipstr[64];
  bool is_v6;

  altcom_getsockname(sock, (struct altcom_sockaddr *)&local, (altcom_socklen_t *)&size);
  is_v6 = (from->sa_family == ALTCOM_AF_INET6);
  size = sizeof(ipstr);
  if (is_v6) {
    IPERF_DBG_PRINT("[%3d] (v6) local %s port %d connected with ", server.client_id,
                    altcom_inet_ntop(ALTCOM_AF_INET6, &local6->sin6_addr, ipstr, size),
                    altcom_ntohs(local6->sin6_port));
    IPERF_DBG_PRINT("%s port %d\r\n",
                    altcom_inet_ntop(ALTCOM_AF_INET6, &from6->sin6_addr, ipstr, size),
                    altcom_ntohs(from6->sin6_port));
  } else {
    IPERF_DBG_PRINT("[%3d] (v4) local %s port %d connected with ", server.client_id,
                    altcom_inet_ntop(ALTCOM_AF_INET, &local4->sin_addr, ipstr, size),
                    altcom_ntohs(local4->sin_port));
    IPERF_DBG_PRINT("%s port %d\r\n",
                    altcom_inet_ntop(ALTCOM_AF_INET, &from4->sin_addr, ipstr, size),
                    altcom_ntohs(from4->sin_port));
  }

  IPERF_DBG_PRINT("[ ID] Interval    Transfer     Bandwidth\n\r");
}

static void stats_buffer(char *outString, double data, enum measure_t type) {
  int conv = KCONV_UNIT;
  const char *format;
  double unit = 1024.0;

  if (type == SPEED) unit = 1000.0;

  while (data >= unit && conv <= KCONV_GIGA) {
    data /= unit;
    conv++;
  }

  /* Fit data in 4 places */
  if (data < 9.995) {        /* 9.995 rounded to 10.0 */
    format = "%4.2f %c";     /* #.## */
  } else if (data < 99.95) { /* 99.95 rounded to 100 */
    format = "%4.1f %c";     /* ##.# */
  } else {
    format = "%4.0f %c"; /* #### */
  }
  sprintf(outString, format, data, kLabel[conv]);
}

/* The report function of a TCP server session */
static void tcp_conn_report(uint64_t diff, enum report_type report_type) {
  uint64_t total_len;
  double duration, bandwidth = 0;
  char data[16], perf[16], time[64];

  if (report_type == INTER_REPORT) {
    total_len = server.i_report.total_bytes;
  } else {
    server.i_report.last_report_time = 0;
    total_len = server.total_bytes;
  }

  /* Converting duration from milliseconds to secs,
   * and bandwidth to bits/sec .
   */
  duration = diff / 1000.0; /* secs */
  if (duration) bandwidth = (total_len / duration) * 8.0;

  stats_buffer(data, total_len, BYTES);
  stats_buffer(perf, bandwidth, SPEED);
  /* On 32-bit platforms, printf is not able to print
   * uint64_t values, so converting these values in strings and
   * displaying results
   */
  sprintf(time, "%4.1f-%4.1f sec", (double)server.i_report.last_report_time,
          (double)(server.i_report.last_report_time + duration));
  IPERF_DBG_PRINT("[%3d] %s  %sBytes  %sbits/sec\n\r", server.client_id, time, data, perf);

  if (report_type == INTER_REPORT) server.i_report.last_report_time += duration;
}

/* thread spawned for each connection */
void tcp_recv_perf_traffic(void *p) {
  char recv_buf[RECV_BUF_SIZE];
  int read_bytes;
  int sock = (int)p;

  server.start_time = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
  server.client_id++;
  server.i_report.last_report_time = 0;
  server.i_report.start_time = 0;
  server.i_report.total_bytes = 0;
  server.total_bytes = 0;

  while (1) {
    /* read a max of RECV_BUF_SIZE bytes from socket */
    if ((read_bytes = altcom_recvfrom(sock, recv_buf, RECV_BUF_SIZE, 0, NULL, NULL)) < 0) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      uint64_t diff_ms = now - server.start_time;
      tcp_conn_report(diff_ms, TCP_ABORTED_REMOTE);
      IPERF_DBG_PRINT("altcom_recvfrom fail, errno = %ld\n\r", altcom_errno());
      break;
    }

    /* break if client closed connection */
    if (read_bytes == 0) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      uint64_t diff_ms = now - server.start_time;
      tcp_conn_report(diff_ms, TCP_DONE_SERVER);
      IPERF_DBG_PRINT("TCP test passed Successfully\n\r");
      break;
    }

    if (REPORT_INTERVAL_TIME) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      server.i_report.total_bytes += read_bytes;
      if (server.i_report.start_time) {
        uint64_t diff_ms = now - server.i_report.start_time;

        if (diff_ms >= REPORT_INTERVAL_TIME) {
          tcp_conn_report(diff_ms, INTER_REPORT);
          server.i_report.start_time = 0;
          server.i_report.total_bytes = 0;
        }
      } else {
        server.i_report.start_time = now;
      }
    }
    /* Record total bytes for final report */
    server.total_bytes += read_bytes;
  }

  /* close connection */
  IPERF_DBG_PRINT("TCP_Recv exit\n\r");
  altcom_close(sock);
  osThreadExit();
}

void start_tcpserver(bool is_v6, uint16_t port) {
  int sock, new_sd;
  struct altcom_sockaddr_storage addr, remote;
  struct altcom_sockaddr_in *addr4 = (struct altcom_sockaddr_in *)&addr;
  struct altcom_sockaddr_in6 *addr6 = (struct altcom_sockaddr_in6 *)&addr;

  if ((sock = altcom_socket(is_v6 ? ALTCOM_AF_INET6 : ALTCOM_AF_INET, ALTCOM_SOCK_STREAM, 0)) < 0) {
    IPERF_DBG_PRINT("UDP server: Error creating Socket\r\n");
    return;
  }

  memset(&addr, 0, sizeof(addr));
  if (is_v6) {
    addr6->sin6_family = ALTCOM_AF_INET6;
    addr6->sin6_port = altcom_htons(port);
    addr6->sin6_addr = in6addr_any;
  } else {
    addr4->sin_family = ALTCOM_AF_INET;
    addr4->sin_port = altcom_htons(port);
    addr4->sin_addr.s_addr = altcom_htonl(ALTCOM_INADDR_ANY);
  }

  if (altcom_bind(sock, (struct altcom_sockaddr *)&addr, sizeof(addr)) < 0) {
    IPERF_DBG_PRINT("TCP server: Unable to bind to port %hu\r\n", port);
    altcom_close(sock);
    return;
  }

  if (altcom_listen(sock, 1) < 0) {
    IPERF_DBG_PRINT("TCP server: tcp_listen failed\r\n");
    altcom_close(sock);
    return;
  }

  IPERF_DBG_PRINT("TCP server listening on port %d\r\n", port);

  IPERF_DBG_PRINT("On Host: Run $iperf -c your.server.ip -i %d -t 300 -w 2M\r\n",
                  INTERIM_REPORT_INTERVAL);

  altcom_socklen_t size;

  size = sizeof(remote);
  if ((new_sd = altcom_accept(sock, (struct altcom_sockaddr *)&remote, (altcom_socklen_t *)&size)) >
      0) {
    print_tcp_conn_stats(sock, (struct altcom_sockaddr *)&remote);

    osThreadAttr_t attr = {0};

    attr.name = "TCP_Recv";
    attr.stack_size = TCP_SERVER_THREAD_STACKSIZE;
    attr.priority = DEFAULT_THREAD_PRIO;

    osThreadId_t tid = osThreadNew(tcp_recv_perf_traffic, (void *)new_sd, &attr);

    if (!tid) {
      altcom_close(new_sd);
    }
  }

  altcom_close(sock);
}