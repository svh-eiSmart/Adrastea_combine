/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 */
#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_NETCONN 0
#define LWIP_SOCKET 1

#define LWIP_IPV4 1
#define LWIP_IPV6 1

/* Enable DHCP to test it, disable UDP checksum to easier inject packets */
#define LWIP_DHCP 1

/* Minimal changes to opt.h required for tcp unit tests: */
#define MEM_SIZE 16000
#define TCP_SND_QUEUELEN 40
#define MEMP_NUM_TCP_SEG TCP_SND_QUEUELEN
#define TCP_SND_BUF (12 * TCP_MSS)
#define TCP_WND (10 * TCP_MSS)
#define LWIP_WND_SCALE 1
#define TCP_RCV_SCALE 0
#define PBUF_POOL_SIZE 30 /* pbuf tests need ~200KByte */
#define MEMP_NUM_TCPIP_MSG_INPKT 20
#define TCPIP_MBOX_SIZE 50
#define TCPIP_THREAD_PRIO (configMAX_PRIORITIES - 1)

/* MEMP_NUM_RAW_PCB: the number of UDP protocol control blocks. One
   per active RAW "connection". */
#define MEMP_NUM_RAW_PCB 3

/* Enable IGMP and MDNS for MDNS tests */
#define LWIP_IGMP 1
#define LWIP_MDNS_RESPONDER 1
#define LWIP_NUM_NETIF_CLIENT_DATA (LWIP_MDNS_RESPONDER)

/* Minimal changes to opt.h required for etharp unit tests: */
#define ETHARP_SUPPORT_STATIC_ENTRIES 1

/* MIB2 stats are required to check IPv4 reassembly results */
#define MIB2_STATS 1

/**
 * LWIP_HAVE_LOOPIF==1: Support loop interface (127.0.0.1).
 * This is only needed when no real netifs are available. If at least one other
 * netif is available, loopback traffic uses this netif.
 */
#define LWIP_NETIF_LOOPBACK 1
#define LWIP_LOOPBACK_MAX_PBUFS 16

#define DEFAULT_THREAD_STACKSIZE 1024
#define DEFAULT_MBOX_SIZE 16

#define LWIP_RAW 1

/* ---------- Statistics options ---------- */
/* individual STATS options can be turned off by defining them to 0
 * (e.g #define TCP_STATS 0). All of them are turned off if LWIP_STATS
 * is 0
 * */

#define LWIP_STATS 1
#define LWIP_STATS_DISPLAY 1

#define PING_USE_SOCKETS 1
#define LWIP_SO_RCVTIMEO 1

/* ---------- DNS options ---------- */
#define LWIP_DNS 1

/* ---------- PPP options ---------- */

#define PPP_SUPPORT 1 /* Set > 0 for PPP */
#define MPPE_SUPPORT PPP_SUPPORT
#define PPPOE_SUPPORT 0  // PPP_SUPPORT
#define PPPOL2TP_SUPPORT PPP_SUPPORT
#define PPPOS_SUPPORT PPP_SUPPORT

#if PPP_SUPPORT > 0

#define NUM_PPP 1 /* Max PPP sessions. */

/* Select modules to enable.  Ideally these would be set in the makefile but
 * we're limited by the command line length so you need to modify the settings
 * in this file.
 */
#define PAP_SUPPORT 0    /* Set > 0 for PAP. */
#define CHAP_SUPPORT 0   /* Set > 0 for CHAP. */
#define MSCHAP_SUPPORT 0 /* Set > 0 for MSCHAP (NOT FUNCTIONAL!) */
#define CBCP_SUPPORT 0   /* Set > 0 for CBCP (NOT FUNCTIONAL!) */
#define CCP_SUPPORT 0    /* Set > 0 for CCP (NOT FUNCTIONAL!) */
#define VJ_SUPPORT 0     /* Set > 0 for VJ header compression. */
#define MD5_SUPPORT 1    /* Set > 0 for MD5 (see also CHAP) */

/*
 * Timeouts.
 */
#define FSM_DEFTIMEOUT 3      /* Timeout time in seconds */
#define FSM_DEFMAXTERMREQS 3  /* Maximum Terminate-Request transmissions */
#define FSM_DEFMAXCONFREQS 30 /* Maximum Configure-Request transmissions */
#define FSM_DEFMAXNAKLOOPS 5  /* Maximum number of nak loops */

#define UPAP_DEFTIMEOUT 6  /* Timeout (seconds) for retransmitting req */
#define UPAP_DEFREQTIME 30 /* Time to wait for auth-req from peer */

#define CHAP_DEFTIMEOUT 6    /* Timeout time in seconds */
#define CHAP_DEFTRANSMITS 10 /* max # times to send challenge */

/* Interval in seconds between keepalive echo requests, 0 to disable. */
#if 1
#define LCP_ECHOINTERVAL 0
#else
#define LCP_ECHOINTERVAL 10
#endif

/* Number of unanswered echo requests before failure. */
#define LCP_MAXECHOFAILS 3

/* Max Xmit idle time (in jiffies) before resend flag char. */
#define PPP_MAXIDLEFLAG 100

#define LWIP_NETIF_LOOPBACK 1
#define LWIP_LOOPBACK_MAX_PBUFS 16

/*
 * Packet sizes
 *
 * Note - lcp shouldn't be allowed to negotiate stuff outside these
 *    limits.  See lcp.h in the pppd directory.
 * (XXX - these constants should simply be shared by lcp.c instead
 *    of living in lcp.h)
 */
#define PPP_MTU 1500 /* Default MTU (size of Info field) */
#if 0
#define PPP_MAXMTU 65535 - (PPP_HDRLEN + PPP_FCSLEN)
#else
#define PPP_MAXMTU 1500 /* Largest MTU we allow */
#endif
#define PPP_MINMTU 64
#define PPP_MRU 1500    /* default MRU = max length of info field */
#define PPP_MAXMRU 1500 /* Largest MRU we allow */
//#define PPP_DEFMRU	(1024+40)		/* Try for this */
#define PPP_DEFMRU 1500 /* Try for this */
#define PPP_MINMRU 128  /* No MRUs below this */

#define MAXNAMELEN 256   /* max length of hostname or name for auth */
#define MAXSECRETLEN 256 /* max length of password or secret */

#endif /* PPP_SUPPORT > 0 */

#define TARGET_PORT 1

/* ---------- SNMP options ---------- */
#define LWIP_SNMP 1
#if !defined(MIB2_STATS)
#define MIB2_STATS LWIP_SNMP
#endif
#define SNMP_USE_NETCONN LWIP_NETCONN
#define SNMP_USE_RAW (!LWIP_NETCONN)

#endif /* LWIP_HDR_LWIPOPTS_H */
