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

#ifndef __IPERF_H_
#define __IPERF_H_

#include "cmsis_os2.h"

#include "apitest_main.h"

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define IPERF_DBG_PRINT(...) \
  do {                       \
    LOCK();                  \
    printf(__VA_ARGS__);     \
    UNLOCK();                \
  } while (0)

/* labels for formats [KMG] */
static const char kLabel[] = {' ', 'K', 'M', 'G'};

/* used as indices into kLabel[] */
enum {
  KCONV_UNIT,
  KCONV_KILO,
  KCONV_MEGA,
  KCONV_GIGA,
};

/* used as type of print */
enum measure_t { BYTES, SPEED };

/* Report Type */
enum report_type {
  INTER_REPORT,      /* The Intermediate report */
  TCP_DONE_CLIENT,   /* The TCP client side test is done */
  TCP_DONE_SERVER,   /* The TCP server side test is done */
  UDP_DONE_CLIENT,   /* The UDP client side test is done */
  UDP_DONE_SERVER,   /* The UDP server side test is done */
  TCP_ABORTED_REMOTE /* Remote side aborted the test */
};

struct interim_report {
  uint64_t start_time;
  uint64_t last_report_time;
  uint32_t total_bytes;
  uint32_t cnt_datagrams;
  uint32_t cnt_dropped_datagrams;
};

struct perf_stats {
  uint8_t client_id;
  uint64_t start_time;
  uint64_t total_bytes;
  uint64_t cnt_datagrams;
  uint64_t cnt_dropped_datagrams;
  uint32_t cnt_out_of_order_datagrams;
  int32_t expected_datagram_id;
  struct interim_report i_report;
};

/* seconds between periodic bandwidth reports */
#define INTERIM_REPORT_INTERVAL 1

#define TCP_SERVER_THREAD_STACKSIZE 4096
#define UDP_SERVER_THREAD_STACKSIZE 4096

/* time in seconds to transmit packets */
#define TCP_TIME_INTERVAL 20
#define UDP_TIME_INTERVAL 20

/* Sleep in microseconds in case of UDP send errors */
#define ERROR_SLEEP 100

/* define default buffer size using default MSS */
#define TCP_SEND_BUFSIZE 1500
#define RECV_BUF_SIZE 1500

/* UDP buffer length in bytes */
#define UDP_SEND_BUFSIZE 1500
#define UDP_RECV_BUFSIZE 1500

/* MAX UDP send retries */
#define MAX_SEND_RETRY 10

/* Number of parallel UDP clients */
#define NUM_OF_PARALLEL_CLIENTS 2

/* priority of receiving task */
#define DEFAULT_THREAD_PRIO (osPriorityNormal)

/* Tick converting */
#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

#endif /* __IPERF_H_ */
