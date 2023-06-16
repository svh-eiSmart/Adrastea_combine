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

/* Connection handle for a UDP Client session */
#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "net/altcom_socket.h"
#include "net/altcom_in.h"
#include "iperf.h"

extern struct netif server_netif;
static struct perf_stats client;
static char send_buf[UDP_SEND_BUFSIZE];
static struct altcom_sockaddr_storage ss;
static int sock[NUM_OF_PARALLEL_CLIENTS];
#define FINISH 1
/* Report interval time in ms */
#define REPORT_INTERVAL_TIME (INTERIM_REPORT_INTERVAL * 1000)
/* End time in ms */
#define END_TIME (UDP_TIME_INTERVAL * 1000)

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

/* The report function of a UDP client session */
static void udp_conn_report(uint64_t diff, enum report_type report_type) {
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

  if (report_type == INTER_REPORT)
    client.i_report.last_report_time += duration;
  else
    IPERF_DBG_PRINT("[%3d] sent %lu datagrams\n\r", client.client_id,
                    (uint32_t)client.cnt_datagrams);
}

static void reset_stats(void) {
  client.client_id++;

  /* Print connection statistics */
  IPERF_DBG_PRINT("[ ID] Interval\t\tTransfer   Bandwidth\n\r");

  /* Save start time for final report */
  client.start_time = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
  client.total_bytes = 0;
  client.cnt_datagrams = 0;

  /* Initialize Interim report parameters */
  client.i_report.start_time = 0;
  client.i_report.total_bytes = 0;
  client.i_report.last_report_time = 0;
}

static int udp_packet_send(uint8_t finished) {
  static int packet_id;
  int i, count, *payload;
  uint8_t retries = MAX_SEND_RETRY;
  altcom_socklen_t len = (ss.ss_family == ALTCOM_AF_INET ? sizeof(struct altcom_sockaddr_in)
                                                         : sizeof(struct altcom_sockaddr_in6));

  payload = (int *)(send_buf);
  if (finished == FINISH) packet_id = -1;
  *payload = altcom_htonl(packet_id);

  /* always increment the id */
  packet_id++;

  for (i = 0; i < NUM_OF_PARALLEL_CLIENTS; i++) {
    while (retries) {
      count =
          altcom_sendto(sock[i], send_buf, sizeof(send_buf), 0, (struct altcom_sockaddr *)&ss, len);
      if (count <= 0) {
        retries--;
        osDelay(ERROR_SLEEP / OS_TICK_PERIOD_MS);
      } else {
        client.total_bytes += count;
        client.cnt_datagrams++;
        client.i_report.total_bytes += count;
        break;
      }
    }

    if (!retries) {
      /* Terminate this app */
      uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
      uint64_t diff_ms = now - client.start_time;
      IPERF_DBG_PRINT("Too many udp_send() retries, ");
      IPERF_DBG_PRINT("Terminating application\n\r");
      udp_conn_report(diff_ms, UDP_DONE_CLIENT);
      IPERF_DBG_PRINT("UDP test failed\n\r");
      return -1;
    }
    retries = MAX_SEND_RETRY;
  }
  return 0;
}

/* Transmit data on a udp session */
int transfer_data(void) {
  if (END_TIME || REPORT_INTERVAL_TIME) {
    uint64_t now = osKernelGetTickCount() / OS_TICK_PERIOD_MS;
    if (REPORT_INTERVAL_TIME) {
      if (client.i_report.start_time) {
        uint64_t diff_ms = now - client.i_report.start_time;
        if (diff_ms >= REPORT_INTERVAL_TIME) {
          udp_conn_report(diff_ms, INTER_REPORT);
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
        /* Add last shot statistic */
        uint64_t last_diff_ms = now - client.i_report.start_time;

        udp_conn_report(last_diff_ms, INTER_REPORT);
        client.i_report.start_time = 0;
        client.i_report.total_bytes = 0;

        /* time specified is over,
         * close the connection */
        udp_packet_send(FINISH);
        udp_conn_report(diff_ms, UDP_DONE_CLIENT);
        IPERF_DBG_PRINT("UDP test passed Successfully\n\r");
        return FINISH;
      }
    }
  }

  if (udp_packet_send(!FINISH) < 0) return FINISH;

  return 0;
}

void start_udpclient(char *ip, uint16_t port) {
  uint32_t i, j;
  bool is_v6;
  struct altcom_sockaddr_in *addr = (struct altcom_sockaddr_in *)&ss;
  struct altcom_sockaddr_in6 *addr6 = (struct altcom_sockaddr_in6 *)&ss;

  memset(&ss, 0, sizeof(struct altcom_sockaddr_storage));
  if (altcom_inet_pton(ALTCOM_AF_INET, ip, (void *)&addr->sin_addr) == 1) {
    is_v6 = false;
    addr->sin_family = ALTCOM_AF_INET;
    addr->sin_port = altcom_htons(port);

  } else if (altcom_inet_pton(ALTCOM_AF_INET6, ip, (void *)&addr6->sin6_addr) == 1) {
    is_v6 = true;
    addr6->sin6_family = ALTCOM_AF_INET6;
    addr6->sin6_port = altcom_htons(port);
  } else {
    IPERF_DBG_PRINT("Invalid IP string: \"%s\"\r\n", ip);
    return;
  }

  for (i = 0; i < NUM_OF_PARALLEL_CLIENTS; i++) {
    if ((sock[i] = altcom_socket(is_v6 ? ALTCOM_AF_INET6 : ALTCOM_AF_INET, ALTCOM_SOCK_DGRAM, 0)) <
        0) {
      IPERF_DBG_PRINT("UDP client: Error creating Socket\r\n");
      for (j = 0; j < i; j++) {
        altcom_close(sock[j]);
      }
      return;
    }
  }

  IPERF_DBG_PRINT("UDP client connecting to %s on port %hu\r\n", ip, port);
  IPERF_DBG_PRINT("On Host: Run $iperf -s -u -i %d -u\r\n\r\n", INTERIM_REPORT_INTERVAL);

  /* Wait for successful connections */
  osDelay(10 / OS_TICK_PERIOD_MS);

  reset_stats();

  /* initialize data buffer being sent with same as used in iperf */
  for (i = 0; i < UDP_SEND_BUFSIZE; i++) {
    send_buf[i] = (i % 10) + '0';
  }

  while (!transfer_data()) {
    ;
  }
  for (i = 0; i < NUM_OF_PARALLEL_CLIENTS; i++) {
    altcom_close(sock[i]);
  }
}