/*
 * Copyright (C) 2017 - 2019 Xilinx, Inc.
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

/** Connection handle for a UDP Server session */

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "iperf.h"

static struct perf_stats server;
extern const struct altcom_in6_addr in6addr_any;

/* Report interval in ms */
#define REPORT_INTERVAL_TIME (INTERIM_REPORT_INTERVAL * 1000)

static void print_udp_conn_stats(int sock, struct altcom_sockaddr *from) {
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

  IPERF_DBG_PRINT("[ ID] Interval\t     Transfer     Bandwidth\t");
  IPERF_DBG_PRINT("    Lost/Total Datagrams\n\r");
}

static void stats_buffer(char *outString, float data, enum measure_t type) {
  int conv = KCONV_UNIT;
  const char *format;
  float unit = 1024.0;

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

/** The report function of a TCP server session */
static void udp_conn_report(uint64_t diff, enum report_type report_type) {
  uint32_t total_len, cnt_datagrams, cnt_dropped_datagrams, total_packets;
  uint32_t cnt_out_of_order_datagrams;
  float duration, bandwidth = 0;
  char data[16], perf[16], time[64], drop[64];

  if (report_type == INTER_REPORT) {
    total_len = server.i_report.total_bytes;
    cnt_datagrams = server.i_report.cnt_datagrams;
    cnt_dropped_datagrams = server.i_report.cnt_dropped_datagrams;
  } else {
    server.i_report.last_report_time = 0;
    total_len = server.total_bytes;
    cnt_datagrams = server.cnt_datagrams;
    cnt_dropped_datagrams = server.cnt_dropped_datagrams;
    cnt_out_of_order_datagrams = server.cnt_out_of_order_datagrams;
  }

  total_packets = cnt_datagrams + cnt_dropped_datagrams;
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
  sprintf(time, "%4.1f-%4.1f sec", (float)server.i_report.last_report_time,
          (float)(server.i_report.last_report_time + duration));
  sprintf(drop, "%4lu/%5lu (%.2g%%)", cnt_dropped_datagrams, total_packets,
          (100.0 * cnt_dropped_datagrams) / total_packets);
  IPERF_DBG_PRINT("[%3d] %s  %sBytes  %sbits/sec  %s\n\r", server.client_id, time, data, perf,
                  drop);

  if (report_type == INTER_REPORT) {
    server.i_report.last_report_time += duration;
  } else if ((report_type != INTER_REPORT) && cnt_out_of_order_datagrams) {
    IPERF_DBG_PRINT("[%3d] %s  %lu datagrams received out-of-order\n\r", server.client_id, time,
                    cnt_out_of_order_datagrams);
  }
}

static void reset_stats(void) {
  server.client_id++;
  /* Save start time */
  server.start_time = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
  server.total_bytes = 0;
  server.cnt_datagrams = 0;
  server.cnt_dropped_datagrams = 0;
  server.cnt_out_of_order_datagrams = 0;
  server.expected_datagram_id = 0;

  /* Initialize Interim report parameters */
  server.i_report.start_time = 0;
  server.i_report.total_bytes = 0;
  server.i_report.cnt_datagrams = 0;
  server.i_report.cnt_dropped_datagrams = 0;
  server.i_report.last_report_time = 0;
}

/** Receive data on a udp session */
static void udp_recv_perf_traffic(void *p) {
  uint8_t first = 1;
  uint32_t drop_datagrams = 0;
  int32_t recv_id;
  int count;
  char recv_buf[UDP_RECV_BUFSIZE];
  struct altcom_sockaddr_storage from;
  altcom_socklen_t fromlen = sizeof(from);
  int sock = (int)p;

  while (1) {
    if ((count = altcom_recvfrom(sock, recv_buf, UDP_RECV_BUFSIZE, 0,
                                 (struct altcom_sockaddr *)&from, &fromlen)) <= 0) {
      continue;
    }

    /* first, check if the datagram is received in order */
    recv_id = altcom_ntohl(*((int *)recv_buf));

    if (first && (recv_id == 0)) {
      /* First packet should always start with recv id 0.
       * However, If Iperf client is running with parallel
       * thread, then this condition will also avoid
       * multiple print of connection header
       */
      reset_stats();
      /* Print connection statistics */
      print_udp_conn_stats(sock, (struct altcom_sockaddr *)&from);
      first = 0;
    } else if (first) {
      /* Avoid rest of the packets if client
       * connection is already terminated.
       */
      continue;
    }

    if (recv_id < 0) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      uint64_t diff_ms = now - server.start_time;
      /* Send Ack */
      if (altcom_sendto(sock, recv_buf, count, 0, (struct altcom_sockaddr *)&from, fromlen) < 0) {
        IPERF_DBG_PRINT("Error in write\n\r");
      }

      udp_conn_report(diff_ms, UDP_DONE_SERVER);
      IPERF_DBG_PRINT("UDP test passed Successfully\n\r");
      first = 1;
      break;
    }

    /* Update dropped datagrams statistics */
    if (server.expected_datagram_id != recv_id) {
      if (server.expected_datagram_id < recv_id) {
        drop_datagrams = recv_id - server.expected_datagram_id;
        server.cnt_dropped_datagrams += drop_datagrams;
        server.i_report.cnt_dropped_datagrams += drop_datagrams;
        server.expected_datagram_id = recv_id + 1;
      } else if (server.expected_datagram_id > recv_id) {
        server.cnt_out_of_order_datagrams++;
      }
    } else {
      server.expected_datagram_id++;
    }

    server.cnt_datagrams++;

    /* Record total bytes for final report */
    server.total_bytes += count;

    if (REPORT_INTERVAL_TIME) {
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;

      server.i_report.cnt_datagrams++;

      /* Record total bytes for interim report */
      server.i_report.total_bytes += count;
      if (server.i_report.start_time) {
        uint64_t diff_ms = now - server.i_report.start_time;

        if (diff_ms >= REPORT_INTERVAL_TIME) {
          udp_conn_report(diff_ms, INTER_REPORT);
          /* Reset Interim report counters */
          server.i_report.start_time = 0;
          server.i_report.total_bytes = 0;
          server.i_report.cnt_datagrams = 0;
          server.i_report.cnt_dropped_datagrams = 0;
        }
      } else {
        /* Save start time for interim report */
        server.i_report.start_time = now;
      }
    }
  }

  /* close connection */
  IPERF_DBG_PRINT("UDP_Recv exit\n\r");
  altcom_close(sock);
  osThreadExit();
}

void start_udpserver(bool is_v6, uint16_t port) {
  int ret;
  int sock;
  struct altcom_sockaddr_storage addr;
  struct altcom_sockaddr_in *addr4 = (struct altcom_sockaddr_in *)&addr;
  struct altcom_sockaddr_in6 *addr6 = (struct altcom_sockaddr_in6 *)&addr;

  if ((sock = altcom_socket(is_v6 ? ALTCOM_AF_INET6 : ALTCOM_AF_INET, ALTCOM_SOCK_DGRAM, 0)) < 0) {
    IPERF_DBG_PRINT("UDP server: Error creating Socket\r\n");
    return;
  }

  memset(&addr, 0, sizeof(struct altcom_sockaddr_storage));
  if (is_v6) {
    addr6->sin6_family = ALTCOM_AF_INET6;
    addr6->sin6_port = altcom_htons(port);
    addr6->sin6_addr = in6addr_any;
  } else {
    addr4->sin_family = ALTCOM_AF_INET;
    addr4->sin_port = altcom_htons(port);
    addr4->sin_addr.s_addr = altcom_htonl(ALTCOM_INADDR_ANY);
  }

  if ((ret = altcom_bind(
           sock, (struct altcom_sockaddr *)&addr,
           is_v6 ? sizeof(struct altcom_sockaddr_in6) : sizeof(struct altcom_sockaddr_in)))) {
    IPERF_DBG_PRINT("UDP server: Error on bind: %d\r\n", ret);
    altcom_close(sock);
    return;
  }

  IPERF_DBG_PRINT("UDP server listening on port %hu\r\n", port);
  IPERF_DBG_PRINT("On Host: Run $iperf -c your.server.ip -i %d -t 300 -u -b <bandwidth>\r\n",
                  INTERIM_REPORT_INTERVAL);

  osThreadAttr_t attr = {0};

  attr.name = "UDP_Recv";
  attr.stack_size = UDP_SERVER_THREAD_STACKSIZE;
  attr.priority = DEFAULT_THREAD_PRIO;

  osThreadId_t tid = osThreadNew(udp_recv_perf_traffic, (void *)sock, &attr);

  if (!tid) {
    altcom_close(sock);
  }
}