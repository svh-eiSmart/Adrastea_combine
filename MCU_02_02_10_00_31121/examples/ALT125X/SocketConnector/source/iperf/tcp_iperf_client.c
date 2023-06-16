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
/* Connection handle for a TCP Client session */
/* Rerefence URL https://github.com/Xilinx/embeddedsw/tree/master/lib/sw_apps */

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "iperf.h"

static char send_buf[TCP_SEND_BUFSIZE];
static struct perf_stats client;

/* Report interval time in ms */
#define REPORT_INTERVAL_TIME (INTERIM_REPORT_INTERVAL * 1000)
/* End time in ms */
#define END_TIME (TCP_TIME_INTERVAL * 1000)

void print_tcp_txperf_header(int sock) {
  altcom_socklen_t size;
  struct altcom_sockaddr_storage local;
  struct altcom_sockaddr_in *local4 = (struct altcom_sockaddr_in *)&local;
  struct altcom_sockaddr_in6 *local6 = (struct altcom_sockaddr_in6 *)&local;
  bool is_v6;
  char ipstr[64];

  size = sizeof(local);
  altcom_getsockname(sock, (struct altcom_sockaddr *)&local, (altcom_socklen_t *)&size);
  is_v6 = (local.ss_family == ALTCOM_AF_INET6);
  size = sizeof(ipstr);
  if (is_v6) {
    IPERF_DBG_PRINT("[%3d] local %s port %d connected\n\r", client.client_id,
                    altcom_inet_ntop(ALTCOM_AF_INET6, &local6->sin6_addr, ipstr, size),
                    altcom_ntohs(local6->sin6_port));
  } else {
    IPERF_DBG_PRINT("[%3d] local %s port %d connected\n\r", client.client_id,
                    altcom_inet_ntop(ALTCOM_AF_INET, &local4->sin_addr, ipstr, size),
                    altcom_ntohs(local4->sin_port));
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

/** The report function of a TCP client session */
static void tcp_conn_report(uint64_t diff, enum report_type report_type) {
  uint64_t total_len;
  double duration, bandwidth = 0;
  char data[16], perf[16], time[64];

  if (report_type == INTER_REPORT) {
    total_len = client.i_report.total_bytes;
  } else {
    client.i_report.last_report_time = 0;
    total_len = client.total_bytes;
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
  sprintf(time, "%4.1f-%4.1f sec", (double)client.i_report.last_report_time,
          (double)(client.i_report.last_report_time + duration));
  IPERF_DBG_PRINT("[%3d] %s  %sBytes  %sbits/sec\n\r", client.client_id, time, data, perf);

  if (report_type == INTER_REPORT) client.i_report.last_report_time += duration;
}

int tcp_send_perf_traffic(int sock) {
  int bytes_send;
  int apiflags = ALTCOM_MSG_MORE;

  client.start_time = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
  client.client_id++;
  client.i_report.last_report_time = 0;
  client.i_report.start_time = 0;
  client.i_report.total_bytes = 0;
  client.total_bytes = 0;

  print_tcp_txperf_header(sock);

  while (1) {
    if ((bytes_send = altcom_send(sock, send_buf, sizeof(send_buf), apiflags)) < 0) {
      IPERF_DBG_PRINT(
          "TCP Client: Either connection aborted"
          " from remote or Error on tcp_write\r\n");
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      uint64_t diff_ms = now - client.start_time;
      tcp_conn_report(diff_ms, TCP_ABORTED_REMOTE);
      break;
    }

    client.total_bytes += bytes_send;

    if (END_TIME || REPORT_INTERVAL_TIME) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      if (REPORT_INTERVAL_TIME) {
        client.i_report.total_bytes += bytes_send;
        if (client.i_report.start_time) {
          uint64_t diff_ms = now - client.i_report.start_time;
          if (diff_ms >= REPORT_INTERVAL_TIME) {
            tcp_conn_report(diff_ms, INTER_REPORT);
            client.i_report.start_time = 0;
            client.i_report.total_bytes = 0;
          }
        } else {
          client.i_report.start_time = now;
        }
      }

      if (END_TIME) {
        /* this session is time-limited */
        uint64_t diff_ms = now - client.start_time;
        if (diff_ms >= END_TIME) {
          /* time specified is over */
          /* close the connection */
          tcp_conn_report(diff_ms, TCP_DONE_CLIENT);
          IPERF_DBG_PRINT("TCP test passed Successfully\n\r");
          break;
        }
      }
    }
  }

  altcom_close(sock);
  return 0;
}

void start_tcpclient(char *ip, uint16_t port) {
  int sock, i;
  struct altcom_sockaddr_storage ss;
  struct altcom_sockaddr_in *addr4 = (struct altcom_sockaddr_in *)&ss;
  struct altcom_sockaddr_in6 *addr6 = (struct altcom_sockaddr_in6 *)&ss;
  bool is_v6;

  memset(&ss, 0, sizeof(ss));
  if (altcom_inet_pton(ALTCOM_AF_INET, ip, (void *)&addr4->sin_addr) == 1) {
    is_v6 = false;
    addr4->sin_family = ALTCOM_AF_INET;
    addr4->sin_port = altcom_htons(port);
  } else if (altcom_inet_pton(ALTCOM_AF_INET6, ip, (void *)&addr6->sin6_addr) == 1) {
    is_v6 = true;
    addr6->sin6_family = ALTCOM_AF_INET6;
    addr6->sin6_port = altcom_htons(port);
  } else {
    IPERF_DBG_PRINT("Invalid IP string: \"%s\"\r\n", ip);
    return;
  }

  /* set up address to connect to */
  if ((sock = altcom_socket(is_v6 ? ALTCOM_AF_INET6 : ALTCOM_AF_INET, ALTCOM_SOCK_STREAM, 0)) < 0) {
    IPERF_DBG_PRINT("TCP Client: Error in creating Socket\r\n");
    return;
  }

  if (altcom_connect(
          sock, (struct altcom_sockaddr *)&ss,
          is_v6 ? sizeof(struct altcom_sockaddr_in6) : sizeof(struct altcom_sockaddr_in)) < 0) {
    IPERF_DBG_PRINT("TCP Client: Error on tcp_connect\r\n");
    altcom_close(sock);
    return;
  }

  /* initialize data buffer being sent */
  for (i = 0; i < TCP_SEND_BUFSIZE; i++) {
    send_buf[i] = (i % 10) + '0';
  }

  IPERF_DBG_PRINT("TCP client connecting to %s on port %hu\r\n", ip, port);
  IPERF_DBG_PRINT("On Host: Run $iperf -s -i %d -w 2M\r\n", INTERIM_REPORT_INTERVAL);

  tcp_send_perf_traffic(sock);
}
