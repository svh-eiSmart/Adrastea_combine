/**
 * @file
 * Ping sender module
 *
 */

/*
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

/**
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "lwip/apps/ping.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip6.h"

//#if PING_USE_SOCKETS
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <stdio.h>
//#endif /* PING_USE_SOCKETS */

struct ping_option
{
  ip_addr_t	ipaddress;
  unsigned int	count;
  unsigned int	ttl;
  unsigned int	timeout;
  unsigned int	packetSize;
} ping_option_t;


/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping target - should be an "ip4_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   (netif_default ? *netif_ip4_gw(netif_default) : (*IP4_ADDR_ANY))
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

#define D_MAX_ICMP_BUFFER_SIZE 		1500

/* ping variables */
static struct ping_option currPing;
static u16_t ping_seq_num;
#ifdef LWIP_DEBUG
static u32_t ping_time;
#endif
static int ping_reply=0;
#if !PING_USE_SOCKETS
static struct raw_pcb *ping_pcb;
#endif /* PING_USE_SOCKETS */

static unsigned int	kill_process=0;
static TickType_t timeTickSend=0;


/** Prepare a echo ICMP request */
static void
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  iecho->seqno  = lwip_htons(++ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}

#if PING_USE_SOCKETS

/* Ping using the socket ip */
static err_t
ping_send(int s, struct ping_option *pPingOp)
{
//  char buff[100];
  int err;
  struct icmp_echo_hdr *iecho;
  struct sockaddr_storage to;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + pPingOp->packetSize;

  if(ping_size > D_MAX_ICMP_BUFFER_SIZE)
  {
    printf("cann't send more than %d packet size\r\n",D_MAX_ICMP_BUFFER_SIZE);
    return ERR_VAL;
  }

  iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
  if (!iecho) {
    printf("failed to allocate memory\r\n");
    return ERR_MEM;
  }

  struct sockaddr_storage {
  u8_t        s2_len;
  sa_family_t ss_family;
  char        s2_data1[2];
  u32_t       s2_data2[3];
#if LWIP_IPV6
  u32_t       s2_data3[3];
#endif /* LWIP_IPV6 */
};


  ping_prepare_echo(iecho, (u16_t)ping_size);

  {
    struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
    to4->sin_len    = sizeof(to4);
    to4->sin_family = AF_INET;
    inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(&pPingOp->ipaddress));
  }

  err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

  mem_free(iecho);

  return (err ? ERR_OK : ERR_VAL);
}

static int
ping_recv(int s)
{
  char buf[1500];
//  char buff[100];
  int len;
  struct sockaddr_storage from;
  int fromlen = sizeof(from);

  while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
    if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
      ip_addr_t fromaddr;
      memset(&fromaddr, 0, sizeof(fromaddr));

#if LWIP_IPV4
      if(from.ss_family == AF_INET) {
        struct sockaddr_in *from4 = (struct sockaddr_in*)&from;
        inet_addr_to_ip4addr(ip_2_ip4(&fromaddr), &from4->sin_addr);
        IP_SET_TYPE(&fromaddr, IPADDR_TYPE_V4);
      }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
      if(from.ss_family == AF_INET6) {
        struct sockaddr_in6 *from6 = (struct sockaddr_in6*)&from;
        inet6_addr_to_ip6addr(ip_2_ip6(&fromaddr), &from6->sin6_addr);
        IP_SET_TYPE(&fromaddr, IPADDR_TYPE_V6);
      }
#endif /* LWIP_IPV6 */

      LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
      ip_addr_debug_print(PING_DEBUG, &fromaddr);
      LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", (sys_now() - ping_time)));

      /* todo: support ICMP6 echo */
#if LWIP_IPV4
      if (IP_IS_V4_VAL(fromaddr)) {
        struct ip_hdr *iphdr;
        struct icmp_echo_hdr *iecho;

        iphdr = (struct ip_hdr *)buf;
        iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
        if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {

          /* do some ping result processing */
          printf("%d length from %lu.%lu.%lu.%lu: icmp_seq=%d ttl=%d milliseconds:%lu\r\n",
                          (int)(len-sizeof(struct ip_hdr)),
                          ((iphdr->src.addr & 0x000000FF)),
                          ((iphdr->src.addr & 0x0000FF00) >> 8),
                          ((iphdr->src.addr & 0x00FF0000) >> 16),
                          ((iphdr->src.addr & 0xFF000000) >> 24),
                          (int)iecho->seqno,
                          (int)iphdr->_ttl,
                          (sys_now() - timeTickSend));
          return 1;
        } else {
          LWIP_DEBUGF( PING_DEBUG, ("ping: drop\n"));
        }
      }
#endif /* LWIP_IPV4 */
    }
    fromlen = sizeof(from);
  }

  if (len == 0) {
    LWIP_DEBUGF( PING_DEBUG, ("ping: recv - %"U32_F" ms - timeout\n", (sys_now()-ping_time)));
  }

  return 0;
}

static void
ping_thread(void *arg)
{
  int s;
  int ret;
//  char buff[100];
  unsigned int i=0;
  ping_reply=0;
  

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int timeout = PING_RCV_TIMEO;
#else
  struct timeval timeout;
  timeout.tv_sec = PING_RCV_TIMEO/1000;
  timeout.tv_usec = (PING_RCV_TIMEO%1000)*1000;
#endif

  struct ping_option *pPingOp=NULL;

  pPingOp = (struct ping_option *)arg;
#if 0
  char esc = 27;
  for (i=0; i<3; i++)
  { // Backspace 3 times
    console_write(&esc,1);
    console_write("[D",strlen("[D"));
    // serial_putc_f(27);
    // serial_putc_f('[');
    // serial_putc_f('D');
  }
#endif
  printf("ping thread started socket creating...\r\n");

#if LWIP_IPV6
  if(IP_IS_V4(&pPingOp->ipaddress) || ip6_addr_isipv4mappedipv6(ip_2_ip6(&pPingOp->ipaddress))) {
//    s = lwip_socket(AF_INET6, SOCK_RAW, IP_PROTO_ICMP);
    s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
  } else {
    s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
  }
#else
  s = lwip_socket(AF_INET,  SOCK_RAW, IP_PROTO_ICMP);
#endif
  if (s < 0) {
    return;
  }

  ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  LWIP_ASSERT("setting receive timeout failed", ret == 0);
  LWIP_UNUSED_ARG(ret);
  // need to received number of ping request
  for(i=0;i<pPingOp->count;i++)
  {
    if (ping_send(s, pPingOp) == ERR_OK) {
#ifdef LWIP_DEBUG
      ping_time = sys_now();
#endif
      timeTickSend = sys_now();
      ping_reply += ping_recv(s);
    } else {
      printf("failed to set ping send\r\n");
    }
	if(kill_process == 1)
		break;
    sys_msleep(pPingOp->timeout);
  }

  printf("request:%d received:%d\r\n>> ",pPingOp->count,ping_reply);
  //((pdFLUSH_CALLBACK)pPingOp->flush_func)(buff,pPingOp->flush_arg);

  lwip_close(s);
  vTaskDelete( NULL );
}

#else /* PING_USE_SOCKETS */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
      pbuf_header(p, -PBUF_IP_HLEN) == 0) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
      LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
      ip_addr_debug_print(PING_DEBUG, addr);
      LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", (sys_now()-ping_time)));

      /* do some ping result processing */
      PING_RESULT(1);
      pbuf_free(p);
      return 1; /* eat the packet */
    }
    /* not eaten, restore original packet */
    pbuf_header(p, PBUF_IP_HLEN);
  }

  return 0; /* don't eat the packet */
}

static void
ping_send(struct raw_pcb *raw, struct ping_option *pPingOp)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + pPingOp->packetSize;

  if(ping_size > D_MAX_ICMP_BUFFER_SIZE)
  {
	  printf("cann't send more than %d packet size\r\n",D_MAX_ICMP_BUFFER_SIZE);
	  return ERR_VAL;
  }

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return ERR_MEM;
  }
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ping_prepare_echo(iecho, (u16_t)ping_size);

    raw_sendto(raw, p, addr);
    ping_time = sys_now();
  }
  pbuf_free(p);
}

static void
ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;
  ip_addr_t ping_target;

  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  ip_addr_copy_from_ip4(ping_target, PING_TARGET);
  ping_send(pcb, &ping_target);

  sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static void
ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, ping_recv, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);
  sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

void
ping_send_now(void)
{
  ip_addr_t ping_target;
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);
  ip_addr_copy_from_ip4(ping_target, PING_TARGET);
  ping_send(ping_pcb, &ping_target);
}

#endif /* PING_USE_SOCKETS */

void ping_init(struct ping_option *p_pingOp)
{

#if PING_USE_SOCKETS
  memcpy(&currPing,p_pingOp,sizeof(struct ping_option));
  sys_thread_new("ping_thread", ping_thread, (void *)&currPing, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#else /* PING_USE_SOCKETS */
  ping_raw_init();
#endif /* PING_USE_SOCKETS */
}

int ping_usage(int8_t *pcWriteBuffer,size_t xWriteBufferLen,void *flush_func,void *flush_arg)
{
        printf ("usage: ping <ip> [-c count] [-s packetsize] [-T timeout]\r\n");
        printf ("\r\n");
        return 0;
}

int get_ping_args(int argc, char **argv, struct ping_option *pPingOp)
{
	int c_ip[4];
	int isIpAddr = 0;
	int i;
	if(sscanf(argv[1], "%d.%d.%d.%d", &c_ip[3], &c_ip[2], &c_ip[1], &c_ip[0]) == 4)
	{
	    inet_pton(AF_INET, (const char *) argv[1], &pPingOp->ipaddress);
	    isIpAddr = 1;
	}
	else if (!(strncmp(argv[1], "-k", 2)))
	{
	    kill_process = 1;
	    return 0;
	}

	for(i = 2; i < argc; i++)
	{
	    if (argv[i] == NULL)
	        continue;

	    if( (isIpAddr) && (strcmp(argv[i], "-c")==0) && (i + 1 < argc) )
	    {
	            pPingOp->count = atoi(argv[i + 1]);
	            i++;
	    }
	    else if( (isIpAddr) && (strcmp(argv[i], "-s")==0) && (i + 1 < argc) )
	    {
	            pPingOp->packetSize = atoi(argv[i+1]);
	            if(pPingOp->packetSize < 4)
	            {
	                    pPingOp->packetSize = 4;
	            }
	            i++;
	    }
	    else if( (isIpAddr) && (strcmp(argv[i], "-t") == 0) && (i + 1 < argc) )
	    {

	            pPingOp->ttl = atoi(argv[i + 1]);
	            i++;
	    }
	    else if( (isIpAddr) && (strcmp(argv[i], "-T")==0) && (i + 1 < argc) )
	    {
	            pPingOp->timeout = atoi(argv[i + 1]);
	            i++;
	    }
		else if(strcmp(argv[i], "-k")==0)
		{
			kill_process = 1;
		}
	}
	return 0;
}

int ping_parser_multi_arg(int argc, char **argv)
{
	struct ping_option 	pingOp;
	// default parameters
	pingOp.count=3;
	pingOp.timeout=PING_DELAY;
	pingOp.ttl=64;
	pingOp.packetSize=100;
	pingOp.ipaddress.type=IPADDR_TYPE_V4;
	kill_process=0;
	inet_pton(AF_INET, "127.0.0.1", &pingOp.ipaddress);
	get_ping_args(argc, argv, &pingOp);

	if(kill_process == 0) {
		ping_init(&pingOp);
	}
	return 0;
}

int ping_parser (char *s)
{
    struct ping_option 	pingOp;
    int c_ip[4];

    // default parameters
    pingOp.count=3;
    pingOp.timeout=PING_DELAY;
    pingOp.ttl=64;
    pingOp.packetSize=100;
    pingOp.ipaddress.type=IPADDR_TYPE_V4;
    kill_process=0;
    inet_pton(AF_INET, "127.0.0.1", &pingOp.ipaddress);

    if(sscanf(s, "%d.%d.%d.%d", &c_ip[3], &c_ip[2], &c_ip[1], &c_ip[0]) == 4)
    {
      inet_pton(AF_INET, s, &pingOp.ipaddress);
    }

    if(kill_process == 0) {
    	ping_init(&pingOp);
    }

    return 0;
}

#endif /* LWIP_RAW */
