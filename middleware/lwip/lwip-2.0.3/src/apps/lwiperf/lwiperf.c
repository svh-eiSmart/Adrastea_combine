/**
 * @file
 * lwIP iPerf server implementation
 */

/**
 * @defgroup iperf Iperf server
 * @ingroup apps
 *
 * This is a simple performance measuring server to check your bandwith using
 * iPerf2 on a PC as client.
 * It is currently a minimal implementation providing an IPv4 TCP server only.
 *
 * @todo: implement UDP mode and IPv6
 */

/*
 * Copyright (c) 2014 Simon Goldschmidt
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
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/apps/lwiperf.h"

#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"

/* Currently, only TCP-over-IPv4 is implemented (does iperf support IPv6 anyway?) */
#if LWIP_IPV4 && LWIP_TCP && LWIP_CALLBACK_API

/** Specify the idle timeout (in seconds) after that the test fails */
#ifndef LWIPERF_TCP_MAX_IDLE_SEC
#define LWIPERF_TCP_MAX_IDLE_SEC    10U
#endif
#if LWIPERF_TCP_MAX_IDLE_SEC > 255
#error LWIPERF_TCP_MAX_IDLE_SEC must fit into an u8_t
#endif

/* File internal memory allocation (struct lwiperf_*): this defaults to
   the heap */
#ifndef LWIPERF_ALLOC
#define LWIPERF_ALLOC(type)         mem_malloc(sizeof(type))
#define LWIPERF_FREE(type, item)    mem_free(item)
#endif

/** If this is 1, check that received data has the correct format */
#ifndef LWIPERF_CHECK_RX_DATA
#define LWIPERF_CHECK_RX_DATA       0
#endif

#define 	HEADER_VERSION1   0x80000000

/** This is the Iperf settings struct sent from the client */
typedef struct _lwiperf_settings {
#define LWIPERF_FLAGS_ANSWER_TEST 0x80000000
#define LWIPERF_FLAGS_ANSWER_NOW  0x00000001
  u32_t flags;
  u32_t num_threads; /* unused for now */
  u32_t remote_port;
  u32_t buffer_len; /* unused for now */
  u32_t win_band; /* TCP window / UDP rate: unused for now */
  u32_t amount; /* pos. value: bytes?; neg. values: time (unit is 10ms: 1/100 second) */
} lwiperf_settings_t;

/** Basic connection handle */
struct _lwiperf_state_base;
typedef struct _lwiperf_state_base lwiperf_state_base_t;
struct _lwiperf_state_base {
  /* 1=tcp, 0=udp */
  u8_t tcp;
  /* 1=server, 0=client */
  u8_t server;
  lwiperf_state_base_t* next;
  lwiperf_state_base_t* related_server_state;
};

/** Connection handle for a TCP iperf session */
typedef struct _lwiperf_state_tcp {
  lwiperf_state_base_t base;
  struct tcp_pcb* server_pcb;
  struct tcp_pcb* conn_pcb;
  u32_t time_started;
  lwiperf_report_fn report_fn;
  void* report_arg;
  u8_t poll_count;
  u8_t next_num;
  u32_t bytes_transferred;
  lwiperf_settings_t settings;
  u8_t have_settings_buf;
} lwiperf_state_tcp_t;

/** List of active iperf sessions */
static lwiperf_state_base_t* lwiperf_all_connections;
/** A const buffer to send from: we want to measure sending, not copying! */
static const u8_t lwiperf_txbuf_const[1600] = {
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
  '0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9',
};

static err_t lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb);
static void lwiperf_tcp_err(void *arg, err_t err);

/** Add an iperf session to the 'active' list */
static void
lwiperf_list_add(lwiperf_state_base_t* item)
{
  if (lwiperf_all_connections == NULL) {
    lwiperf_all_connections = item;
  } else {
    item = lwiperf_all_connections;
  }
}

/** Remove an iperf session from the 'active' list */
static void
lwiperf_list_remove(lwiperf_state_base_t* item)
{
  lwiperf_state_base_t* prev = NULL;
  lwiperf_state_base_t* iter;
  for (iter = lwiperf_all_connections; iter != NULL; prev = iter, iter = iter->next) {
    if (iter == item) {
      if (prev == NULL) {
        lwiperf_all_connections = iter->next;
      } else {
        prev->next = item;
      }
      /* @debug: ensure this item is listed only once */
      for (iter = iter->next; iter != NULL; iter = iter->next) {
        LWIP_ASSERT("duplicate entry", iter != item);
      }
      break;
    }
  }
}

/** Call the report function of an iperf tcp session */
static void
lwip_tcp_conn_report(lwiperf_state_tcp_t* conn, enum lwiperf_report_type report_type)
{
  if ((conn != NULL) && (conn->report_fn != NULL)) {
    u32_t now, duration_ms, bandwidth_kbitpsec;
    now = sys_now();
    duration_ms = now - conn->time_started;
    if (duration_ms == 0) {
      bandwidth_kbitpsec = 0;
    } else {
      bandwidth_kbitpsec = (conn->bytes_transferred / duration_ms) * 8U;
    }
    conn->report_fn(conn->report_arg, report_type,
      &conn->conn_pcb->local_ip, conn->conn_pcb->local_port,
      &conn->conn_pcb->remote_ip, conn->conn_pcb->remote_port,
      conn->bytes_transferred, duration_ms, bandwidth_kbitpsec);
  }
}

/** Close an iperf tcp session */
static void
lwiperf_tcp_close(lwiperf_state_tcp_t* conn, enum lwiperf_report_type report_type)
{
  err_t err;

  lwip_tcp_conn_report(conn, report_type);
  lwiperf_list_remove(&conn->base);
  if (conn->conn_pcb != NULL) {
    tcp_arg(conn->conn_pcb, NULL);
    tcp_poll(conn->conn_pcb, NULL, 0);
    tcp_sent(conn->conn_pcb, NULL);
    tcp_recv(conn->conn_pcb, NULL);
    tcp_err(conn->conn_pcb, NULL);
    err = tcp_close(conn->conn_pcb);
    if (err != ERR_OK) {
      /* don't want to wait for free memory here... */
      tcp_abort(conn->conn_pcb);
    }
  } else {
    /* no conn pcb, this is the server pcb */
    err = tcp_close(conn->server_pcb);
    LWIP_ASSERT("error", err != ERR_OK);
  }
  LWIPERF_FREE(lwiperf_state_tcp_t, conn);
}

/** Try to send more data on an iperf tcp session */
static err_t
lwiperf_tcp_client_send_more(lwiperf_state_tcp_t* conn)
{
  int send_more;
  err_t err;
  u16_t txlen;
  u16_t txlen_max;
  void* txptr;
  u8_t apiflags;

  LWIP_ASSERT("conn invalid", (conn != NULL) && conn->base.tcp && (conn->base.server == 0));

  do {
    send_more = 0;
    if (conn->settings.amount & PP_HTONL(0x80000000)) {
      /* this session is time-limited */
      u32_t now = sys_now();
      u32_t diff_ms = now - conn->time_started;
      u32_t time = (u32_t)-(s32_t)lwip_htonl(conn->settings.amount);
      u32_t time_ms = time * 10;
      if (diff_ms >= time_ms) {
        /* time specified by the client is over -> close the connection */
        lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
        return ERR_OK;
      }
    } else {
      /* this session is byte-limited */
      u32_t amount_bytes = lwip_htonl(conn->settings.amount);
      /* @todo: this can send up to 1*MSS more than requested... */
      if (amount_bytes >= conn->bytes_transferred) {
        /* all requested bytes transferred -> close the connection */
        lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_CLIENT);
        return ERR_OK;
      }
    }

    if (conn->bytes_transferred < 24) {
      /* transmit the settings a first time */
      txptr = &((u8_t*)&conn->settings)[conn->bytes_transferred];
      txlen_max = (u16_t)(24 - conn->bytes_transferred);
      apiflags = TCP_WRITE_FLAG_COPY;
    } else if (conn->bytes_transferred < 48) {
      /* transmit the settings a second time */
      txptr = &((u8_t*)&conn->settings)[conn->bytes_transferred - 24];
      txlen_max = (u16_t)(48 - conn->bytes_transferred);
      apiflags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;
      send_more = 1;
    } else {
      /* transmit data */
      /* @todo: every x bytes, transmit the settings again */
      txptr = LWIP_CONST_CAST(void*, &lwiperf_txbuf_const[conn->bytes_transferred % 10]);
      txlen_max = TCP_MSS;
      if (conn->bytes_transferred == 48) { /* @todo: fix this for intermediate settings, too */
        txlen_max = TCP_MSS - 24;
      }
      apiflags = 0; /* no copying needed */
      send_more = 1;
    }
    txlen = txlen_max;
    do {
      err = tcp_write(conn->conn_pcb, txptr, txlen, apiflags);
      if (err ==  ERR_MEM) {
        txlen /= 2;
      }
    } while ((err == ERR_MEM) && (txlen >= (TCP_MSS/2)));

    if (err == ERR_OK) {
      conn->bytes_transferred += txlen;
    } else {
      send_more = 0;
    }
  } while(send_more);

  tcp_output(conn->conn_pcb);
  return ERR_OK;
}

/** TCP sent callback, try to send more data */
static err_t
lwiperf_tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;
  /* @todo: check 'len' (e.g. to time ACK of all data)? for now, we just send more... */
  LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  LWIP_UNUSED_ARG(len);

  conn->poll_count = 0;

  return lwiperf_tcp_client_send_more(conn);
}

/** TCP connected callback (active connection), send data now */
static err_t
lwiperf_tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;
  LWIP_ASSERT("invalid conn", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  if (err != ERR_OK) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
    return ERR_OK;
  }
  conn->poll_count = 0;
  conn->time_started = sys_now();
  return lwiperf_tcp_client_send_more(conn);
}

/** Start TCP connection back to the client (either parallel or after the
 * receive test has finished.
 */
static err_t
lwiperf_tx_start(lwiperf_state_tcp_t* conn)
{
  err_t err;
  lwiperf_state_tcp_t* client_conn;
  struct tcp_pcb* newpcb;
  ip_addr_t remote_addr;
  u16_t remote_port;

  client_conn = (lwiperf_state_tcp_t*)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (client_conn == NULL) {
    return ERR_MEM;
  }
  newpcb = tcp_new();
  if (newpcb == NULL) {
    LWIPERF_FREE(lwiperf_state_tcp_t, client_conn);
    return ERR_MEM;
  }

  MEMCPY(client_conn, conn, sizeof(lwiperf_state_tcp_t));
  client_conn->base.server = 0;
  client_conn->server_pcb = NULL;
  client_conn->conn_pcb = newpcb;
  client_conn->time_started = sys_now(); /* @todo: set this again on 'connected' */
  client_conn->poll_count = 0;
  client_conn->next_num = 4; /* initial nr is '4' since the header has 24 byte */
  client_conn->bytes_transferred = 0;
  client_conn->settings.flags = 0; /* prevent the remote side starting back as client again */

  tcp_arg(newpcb, client_conn);
  tcp_sent(newpcb, lwiperf_tcp_client_sent);
  tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
  tcp_err(newpcb, lwiperf_tcp_err);

  ip_addr_copy(remote_addr, conn->conn_pcb->remote_ip);
  remote_port = (u16_t)lwip_htonl(client_conn->settings.remote_port);

  err = tcp_connect(newpcb, &remote_addr, remote_port, lwiperf_tcp_client_connected);
  if (err != ERR_OK) {
    lwiperf_tcp_close(client_conn, LWIPERF_TCP_ABORTED_LOCAL);
    return err;
  }
  lwiperf_list_add(&client_conn->base);
  return ERR_OK;
}

/** Receive data on an iperf tcp session */
static err_t
lwiperf_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  u8_t tmp;
  u16_t tot_len;
  u32_t packet_idx;
  struct pbuf* q;
  lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;

  LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);

  if (err != ERR_OK) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
    return ERR_OK;
  }
  if (p == NULL) {
    /* connection closed -> test done */
    if ((conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST|LWIPERF_FLAGS_ANSWER_NOW)) ==
        PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST)) {
      /* client requested transmission after end of test */
      lwiperf_tx_start(conn);
    }
    lwiperf_tcp_close(conn, LWIPERF_TCP_DONE_SERVER);
    return ERR_OK;
  }
  tot_len = p->tot_len;

  conn->poll_count = 0;
  //printf("p->len: %d, p->tot_len: %d\r\n", p->len, p->tot_len);

  if ((!conn->have_settings_buf) || ((conn->bytes_transferred -24) % (1024*128) == 0)) {
    /* wait for 24-byte header */
    if (p->tot_len < sizeof(lwiperf_settings_t)) {
      lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
      pbuf_free(p);
      return ERR_VAL;
    }
    if (!conn->have_settings_buf) {
      if (pbuf_copy_partial(p, &conn->settings, sizeof(lwiperf_settings_t), 0) != sizeof(lwiperf_settings_t)) {
        lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
        pbuf_free(p);
        return ERR_VAL;
      }
      conn->have_settings_buf = 1;
      if ((conn->settings.flags & PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST|LWIPERF_FLAGS_ANSWER_NOW)) ==
        PP_HTONL(LWIPERF_FLAGS_ANSWER_TEST|LWIPERF_FLAGS_ANSWER_NOW)) {
          /* client requested parallel transmission test */
          err_t err2 = lwiperf_tx_start(conn);
          if (err2 != ERR_OK) {
            lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_TXERROR);
            pbuf_free(p);
            return err2;
          }
      }
    } else {
      if (pbuf_memcmp(p, 0, &conn->settings, sizeof(lwiperf_settings_t)) != 0) {
        lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
        pbuf_free(p);
        return ERR_VAL;
      }
    }
    conn->bytes_transferred += sizeof(lwiperf_settings_t);
    if (conn->bytes_transferred <= 24) {
      conn->time_started = sys_now();
      tcp_recved(tpcb, p->tot_len);
      pbuf_free(p);
      return ERR_OK;
    }
    conn->next_num = 4; /* 24 bytes received... */
    tmp = pbuf_header(p, -24);
    LWIP_ASSERT("pbuf_header failed", tmp == 0);
  }

  packet_idx = 0;
  for (q = p; q != NULL; q = q->next) {
#if LWIPERF_CHECK_RX_DATA
    const u8_t* payload = (const u8_t*)q->payload;
    u16_t i;
    for (i = 0; i < q->len; i++) {
      u8_t val = payload[i];
      u8_t num = val - '0';
      if (num == conn->next_num) {
        conn->next_num++;
        if (conn->next_num == 10) {
          conn->next_num = 0;
        }
      } else {
        lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL_DATAERROR);
        pbuf_free(p);
        return ERR_VAL;
      }
    }
#endif
    packet_idx += q->len;
  }
  LWIP_ASSERT("count mismatch", packet_idx == p->tot_len);
  conn->bytes_transferred += packet_idx;
  tcp_recved(tpcb, tot_len);
  pbuf_free(p);
  return ERR_OK;
}

/** Error callback, iperf tcp session aborted */
static void
lwiperf_tcp_err(void *arg, err_t err)
{
  lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;
  LWIP_UNUSED_ARG(err);
  lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_REMOTE);
}

/** TCP poll callback, try to send more data */
static err_t
lwiperf_tcp_poll(void *arg, struct tcp_pcb *tpcb)
{
  lwiperf_state_tcp_t* conn = (lwiperf_state_tcp_t*)arg;
  LWIP_ASSERT("pcb mismatch", conn->conn_pcb == tpcb);
  LWIP_UNUSED_ARG(tpcb);
  if (++conn->poll_count >= LWIPERF_TCP_MAX_IDLE_SEC) {
    lwiperf_tcp_close(conn, LWIPERF_TCP_ABORTED_LOCAL);
    return ERR_OK; /* lwiperf_tcp_close frees conn */
  }

  if (!conn->base.server) {
    lwiperf_tcp_client_send_more(conn);
  }

  return ERR_OK;
}

/** This is called when a new client connects for an iperf tcp session */
static err_t
lwiperf_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  lwiperf_state_tcp_t *s, *conn;
  if ((err != ERR_OK) || (newpcb == NULL) || (arg == NULL)) {
    return ERR_VAL;
  }

  s = (lwiperf_state_tcp_t*)arg;
  conn = (lwiperf_state_tcp_t*)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (conn == NULL) {
    return ERR_MEM;
  }
  memset(conn, 0, sizeof(lwiperf_state_tcp_t));
  conn->base.tcp = 1;
  conn->base.server = 1;
  conn->base.related_server_state = &s->base;
  conn->server_pcb = s->server_pcb;
  conn->conn_pcb = newpcb;
  conn->time_started = sys_now();
  conn->report_fn = s->report_fn;
  conn->report_arg = s->report_arg;

  /* setup the tcp rx connection */
  tcp_arg(newpcb, conn);
  tcp_recv(newpcb, lwiperf_tcp_recv);
  tcp_poll(newpcb, lwiperf_tcp_poll, 2U);
  tcp_err(conn->conn_pcb, lwiperf_tcp_err);

  lwiperf_list_add(&conn->base);
  return ERR_OK;
}

/** 
 * @ingroup iperf
 * Start a TCP iperf server on the default TCP port (5001) and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void*
lwiperf_start_tcp_server_default(lwiperf_report_fn report_fn, void* report_arg)
{
  return lwiperf_start_tcp_server(IP_ADDR_ANY, LWIPERF_TCP_PORT_DEFAULT,
    report_fn, report_arg);
}

/**
 * @ingroup iperf
 * Start a TCP iperf server on a specific IP address and port and listen for
 * incoming connections from iperf clients.
 *
 * @returns a connection handle that can be used to abort the server
 *          by calling @ref lwiperf_abort()
 */
void*
lwiperf_start_tcp_server(const ip_addr_t* local_addr, u16_t local_port,
  lwiperf_report_fn report_fn, void* report_arg)
{
  err_t err;
  struct tcp_pcb* pcb;
  lwiperf_state_tcp_t* s;

  if (local_addr == NULL) {
    return NULL;
  }

  s = (lwiperf_state_tcp_t*)LWIPERF_ALLOC(lwiperf_state_tcp_t);
  if (s == NULL) {
    return NULL;
  }
  memset(s, 0, sizeof(lwiperf_state_tcp_t));
  s->base.tcp = 1;
  s->base.server = 1;
  s->report_fn = report_fn;
  s->report_arg = report_arg;

  pcb = tcp_new();
  if (pcb != NULL) {
    err = tcp_bind(pcb, local_addr, local_port);
    if (err == ERR_OK) {
      s->server_pcb = tcp_listen_with_backlog(pcb, 1);
    }
  }
  if (s->server_pcb == NULL) {
    if (pcb != NULL) {
      tcp_close(pcb);
    }
    LWIPERF_FREE(lwiperf_state_tcp_t, s);
    return NULL;
  }
  pcb = NULL;

  tcp_arg(s->server_pcb, s);
  tcp_accept(s->server_pcb, lwiperf_tcp_accept);

  lwiperf_list_add(&s->base);
  return s;
}

/**
 * @ingroup iperf
 * Abort an iperf session (handle returned by lwiperf_start_tcp_server*())
 */
void
lwiperf_abort(void* lwiperf_session)
{
  lwiperf_state_base_t* i, *dealloc, *last = NULL;

  for (i = lwiperf_all_connections; i != NULL; ) {
    if ((i == lwiperf_session) || (i->related_server_state == lwiperf_session)) {
      dealloc = i;
      i = i->next;
      if (last != NULL) {
        last->next = i;
      }
      tcp_close(((lwiperf_state_tcp_t *)lwiperf_session)->server_pcb);
      LWIPERF_FREE(lwiperf_state_tcp_t, dealloc); /* @todo: type? */
    } else {
      last = i;
      i = i->next;
    }
  }
}

#endif /* LWIP_IPV4 && LWIP_TCP && LWIP_CALLBACK_API */

#if LWIP_IPV4 && LWIP_UDP && LWIP_CALLBACK_API

typedef struct _lwip_udp_client_header {
	u32_t id;
	u32_t tv_sec;
	u32_t tv_usec;
} lwip_udp_client_header;

typedef struct _lwip_server_header {
	u32_t tv_sec;
	u32_t tv_usec;
	u32_t packet_count;

	u32_t flags;
	u32_t total_len1;
	u32_t total_len2;
	u32_t stop_sec;
	u32_t stop_usec;
	u32_t error_cnt;
	u32_t outorder_cnt;
	u32_t datagrams;
	u32_t jitter1;
	u32_t jitter2;
	char data[0];

}lwip_server_header;

typedef struct
{
	u32_t					packetCount;
	u32_t					cntError;
	u32_t					outOfOrder;
	u16_t 					local_port;
	u32_t 					bytes_received;
	u32_t					flag_get_info;
	TickType_t 				start_timeTick;
	u32_t					testPeriod;
	u32_t					datagrams;
	u32_t					lastdatagrams;
	u32_t					total_len;
	unsigned long long		jitter;
	unsigned long long		prev_diff;
}lwiperfStat;

typedef struct
{
	lwip_udp_client_header  udp_header;
	lwiperf_settings_t 	    iperf_header;
	lwiperf_report_fn 		report_fn;
	struct udp_pcb 			*pcb;

	lwiperfStat				stat;

}lwiperf_udp_db;

void lwiperf_udp_reply(lwiperf_udp_db *udpDb,struct udp_pcb *pcb,const ip_addr_t *addr,u16_t port)
{
	struct pbuf *p=NULL;
	lwip_server_header *udp_reply;

	p = pbuf_alloc(PBUF_IP, 1450, PBUF_POOL);

	if(p == NULL)
	{
		printf("failed to create pbuf");
		return;
	}
	udp_reply = (lwip_server_header *)p->payload;

	udp_reply->tv_sec=0;
	udp_reply->tv_usec=0;
	udp_reply->packet_count=0;
	udp_reply->flags=lwip_ntohl(HEADER_VERSION1);
	udp_reply->total_len1=0;
	udp_reply->total_len2=lwip_ntohl(udpDb->stat.total_len);
	udp_reply->stop_sec=0;
	udp_reply->stop_usec=0;
	udp_reply->error_cnt=lwip_ntohl(udpDb->stat.cntError);
	udp_reply->outorder_cnt=lwip_ntohl(udpDb->stat.outOfOrder);
	udp_reply->datagrams=lwip_ntohl(udpDb->stat.datagrams);
	udp_reply->jitter1=lwip_ntohl((u32_t)(udpDb->stat.jitter/1000000));
	udp_reply->jitter2=lwip_ntohl((u32_t)(udpDb->stat.jitter%1000000));


	memcpy(&udp_reply->data[0],LWIP_CONST_CAST(void*, &lwiperf_txbuf_const[sizeof(lwip_server_header) % 10]),1450-sizeof(lwip_server_header));


	if(udp_sendto(pcb,p,addr,port) != ERR_OK)
	{
		printf("failed to send message");
	}

	if(p != NULL)
	{
		pbuf_free(p);
	}
}

unsigned long long lwip_time_diff(struct timeval *first, struct timeval *second)
{
	unsigned long long x_ms, y_ms;

  x_ms = (unsigned long long)first->tv_sec * 1000000 + (unsigned long long)first->tv_usec;
  y_ms = (unsigned long long)second->tv_sec * 1000000 + (unsigned long long)second->tv_usec;

  return y_ms - x_ms;
}

static void timer_nd6_tmr(void *arg)
{
	lwiperf_udp_db *udpDb=(lwiperf_udp_db *)arg;
	if(udpDb == NULL)
	{
		return;
	}
	if(udpDb->stat.flag_get_info == 1)
	{
		if(udpDb->stat.lastdatagrams == 0)
		{
			printf("traffic not received during 2 seconds\r\n");
			udp_disconnect (udpDb->pcb);
			memset(&udpDb->stat,0,sizeof(lwiperfStat));
			return;
		}
		udpDb->stat.lastdatagrams=0;
		sys_timeout(2000, timer_nd6_tmr ,udpDb);
	}
}

static void lwiperf_recv_udp_session(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	lwiperf_udp_db *udpDb=(lwiperf_udp_db *)arg;
	lwip_udp_client_header  curr_header;
	struct timeval sent_time;
	struct timeval received_time;
	unsigned long long timeDiff;
	int report_now=0;
	u32_t client_sent=0;

	if(p == NULL)
	{
		printf("pbuf is NULL\r\n");
		udp_remove(pcb);
		return;
	}

	if(udpDb == NULL)
	{
		printf("udpDb is NULL\r\n");
		udp_remove(pcb);
		return;
	}

	if(p->payload == NULL)
	{
		printf("pbuf payload is NULL\r\n");
		udp_remove(pcb);
		goto out_udp_recv;
	}


	if (p->tot_len < ( sizeof(lwiperf_settings_t) + sizeof(lwip_udp_client_header)) )
	{
		udp_disconnect(pcb);
		goto out_udp_recv;
	}

	gettimeofday(&received_time, NULL);
	if (pbuf_copy_partial(p,&curr_header, sizeof(lwip_udp_client_header), 0) != sizeof(lwip_udp_client_header))
	{
		printf("failed to copy setting buffer\r\n");
		udp_remove(pcb);
		goto out_udp_recv;
	}
	sent_time.tv_sec = lwip_htonl(curr_header.tv_sec);
	sent_time.tv_usec = lwip_htonl(curr_header.tv_usec);

	timeDiff = lwip_time_diff(&sent_time, &received_time);

	if(udpDb->stat.prev_diff == 0)
	{
		timeDiff = timeDiff - udpDb->stat.prev_diff;
		udpDb->stat.jitter += (timeDiff - udpDb->stat.jitter) / 16.0;
	}
	udpDb->stat.prev_diff = timeDiff;

	udpDb->stat.total_len = p->tot_len;


	if(udpDb->stat.flag_get_info == 0)
	{
		memcpy(&udpDb->udp_header,&curr_header,sizeof(lwip_udp_client_header));

		//sent_time.tv_sec = lwip_htonl(udpDb->udp_header.tv_sec);
		//sent_time.tv_usec = lwip_htonl(udpDb->udp_header.tv_usec);

		if (pbuf_copy_partial(p,&udpDb->iperf_header, sizeof(lwiperf_settings_t),sizeof(lwip_udp_client_header) ) != sizeof(lwiperf_settings_t))
		{
			printf("failed to copy udp client header buffer\r\n");
			udp_remove(pcb);
			goto out_udp_recv;
		}
		if( (lwip_htonl(curr_header.id) & LWIPERF_FLAGS_ANSWER_TEST) == 0)
		{
			udpDb->stat.packetCount = lwip_htonl(udpDb->udp_header.id);
			udpDb->stat.flag_get_info = 1;
			udpDb->stat.start_timeTick = xTaskGetTickCount( );
			udpDb->stat.testPeriod = (0xFFFFFFFF - lwip_htonl(udpDb->iperf_header.amount))+1; // time in 10 milliseconds

			printf("traffic period time:%ld milliseconds\r\n",udpDb->stat.testPeriod);
			sys_timeout(2000, timer_nd6_tmr ,udpDb);
		}
		else
		{
			sys_untimeout(timer_nd6_tmr, udpDb);
		}
	}
	else
	{
		if (pbuf_copy_partial(p,&curr_header, sizeof(lwip_udp_client_header), 0) != sizeof(lwip_udp_client_header))
		{
			printf("failed to copy setting buffer\r\n");
			udp_remove(pcb);
			goto out_udp_recv;
		}
		//sent_time.tv_sec = lwip_htonl(curr_header.tv_sec);
		//sent_time.tv_usec = lwip_htonl(curr_header.tv_usec);


		if(lwip_htonl(curr_header.id) & LWIPERF_FLAGS_ANSWER_TEST)
		{
			// send report now
			client_sent = (0xFFFFFFFF - lwip_htonl(curr_header.id))+1;
			report_now=1;

		}
		else
		{
			if(lwip_htonl(curr_header.id) != (udpDb->stat.packetCount +1))
			{
				udpDb->stat.cntError++;
			}
			if(lwip_htonl(curr_header.id) <= udpDb->stat.packetCount)
			{
				udpDb->stat.outOfOrder++;
			}
			udpDb->stat.packetCount = lwip_htonl(curr_header.id);
		}

		udpDb->stat.bytes_received += p->tot_len - ( sizeof(lwiperf_settings_t) + sizeof(lwip_udp_client_header) );
		udpDb->stat.datagrams++;
		udpDb->stat.lastdatagrams++;
		if(report_now == 1)
		{
			printf("traffic ended client report:%ld packet server received:%ld\r\n",client_sent,udpDb->stat.datagrams);
			// send reply
			lwiperf_udp_reply(udpDb,pcb,addr,port);
			udp_disconnect (pcb);
			udpDb->report_fn(NULL, LWIPERF_TCP_DONE_CLIENT,
				      NULL, udpDb->stat.local_port,addr, port,
					  udpDb-> stat.bytes_received, udpDb->stat.testPeriod*10, udpDb->stat.bytes_received*8/(udpDb->stat.testPeriod*10));
			memset(&udpDb->stat,0,sizeof(lwiperfStat));
		}
	}

#if 0
	printf("flags:0x%x\r\n",lwip_htonl(tSetting.flags));
	printf("num_threads:0x%x\r\n",lwip_htonl(tSetting.num_threads));
	printf("remote_port:0x%x\r\n",lwip_htonl(tSetting.remote_port));
	printf("buffer_len:0x%x\r\n",lwip_htonl(tSetting.buffer_len));
	printf("win_band:0x%x\r\n",lwip_htonl(tSetting.win_band));
	printf("amount:0x%x\r\n",lwip_htonl(tSetting.amount));

	printf("udp id:0x%x\r\n",lwip_htonl(tUdp.id));
	printf("udp tv_sec:0x%x\r\n",lwip_htonl(tUdp.tv_sec));
	printf("udp tv_usec:0x%x\r\n",lwip_htonl(tUdp.tv_usec));
#endif
out_udp_recv:
	pbuf_free(p);
}

void *lwiperf_start_udp_server(const ip_addr_t* local_addr, u16_t local_port,
  lwiperf_report_fn report_fn, void* report_arg)
{
	struct udp_pcb * pcb;
	err_t err=0;
	lwiperf_udp_db *udpDb=NULL;

	pcb = udp_new();

	if(pcb == NULL)
	{
		return NULL;
	}

	udpDb = malloc(sizeof(lwiperf_udp_db));

	if(udpDb == NULL)
	{
		return NULL;
	}


	memset(udpDb,0,sizeof(lwiperf_udp_db));
	udpDb->report_fn = report_fn;
	udpDb->stat.local_port=local_port;


	err= udp_bind(pcb, IP_ADDR_ANY, local_port);

	if(err != ERR_OK)
	{
		printf("failed to create udp socket port id:%d\r\n",local_port);
		free(udpDb);
		return NULL;
	}

	udp_recv(pcb, lwiperf_recv_udp_session,(void *)udpDb);
	udpDb->pcb = pcb;
	return (void *)udpDb;
}

void lwiperf_udp_abort(void *handle)
{
	lwiperf_udp_db *udpDb=(lwiperf_udp_db *)handle;

	udp_remove(udpDb->pcb);

	free(udpDb);
}
#endif
