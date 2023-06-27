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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <getopt.h>
#include <time.h>

#include "lwip/opt.h"
//#include "lwip/alt_opt.h"

#include "lwip/init.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
/* #include "lwip/timers.h" was replaced by "lwip/timeouts.h"*/
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"

#include "lwip/stats.h"

#include "lwip/tcp.h"
#include "lwip/inet_chksum.h"

#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/ppp/pppapi.h"
//#include "shell/shell.h"
//#include "netif/tapif.h"
//#include "netif/tunif.h"

//#include "netif/unixif.h"
//#include "netif/dropif.h"
//#include "netif/pcapif.h"

//#include "netif/tcpdump.h"
//#include "osal/osal.h"
//#include <pthread.h>

#ifdef TARGET_PORT
/* for baremettal */
/* Standard include. */
#include <stdio.h>
#define malloc(size) pvPortMalloc(size)
#define free(ptr) vPortFree(ptr)

#endif
#if LWIP_HAVE_SLIPIF
#include "netif/slipif.h"
#define SLIP_PTY_TEST 1
#endif

#if PPP_SUPPORT
#include "netif/ppp/pppos.h"
#include "pppos_netif.h"
#include "lwip/sio.h"
/* #define PPP_PTY_TEST 1 put under comment think it is not needed
#include <termios.h> */
#define PPPOS_RX_THREAD_PRIO 3
#endif

#include "lwip/ip_addr.h"
#include "arch/perf.h"
#include "netif/ppp/pppe2e.h"

#include "netif/ppp/lcp.h"

#ifdef APPS
#include "lwip/apps/httpd.h"
#include "apps/udpecho/udpecho.h"
#include "apps/tcpecho/tcpecho.h"
#include "apps/shell/shell.h"
#include "apps/chargen/chargen.h"
#include "apps/netio/netio.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_mib2.h"
#include "apps/snmp_private_mib/private_mib.h"
#endif

#if LWIP_RAW
#include "lwip/icmp.h"
#include "lwip/raw.h"
#endif

#include "netif/ppp/pppdebug.h"
//#include "CLI/FreeRTOS_CLI.h"

static int globPppInterfaceUp=pdFALSE;
//static int gPppLastlinkState=pdFALSE;
static int tcpip_packet_received=0;
void set_print_ppp_message_flag(int ppp_flag);

//void set_callback_received_buffer(ppp_is_received_buffer cb);

void ppp_start(ppp_pcb *pcb);
void link_established(ppp_pcb *pcb);
//void set_print_ppp_state(int print_state);
void set_ppp_new_phase(int enable);
void set_atdt_flag(int enable);
void set_pppEnd2Endtimeout(int flags);
void ppp_app_main(void *arg);
void ppp_startup(void);
void ppp_release_sem(int sleep);
void set_ppp_thread_status(int status);
int is_ppp_thread_up(void);
int pppe2e_during_activation(void);
void stop_timer_ppp_ened2end(void);



int initPppApp(pppos_arg_t *pppos_arg_ptr)
{
	pppos_arg_t *pppos_arg_copy = (pppos_arg_t *) malloc(sizeof(pppos_arg_t));
	if (!pppos_arg_ptr){
           printf("initPppApp- malloc failed for size %u\n",sizeof (pppos_arg_t));
	   return 0;
	}
	memcpy(pppos_arg_copy, pppos_arg_ptr, sizeof(pppos_arg_t));
	if (!sys_thread_new("ppp_thread", ppp_app_main, pppos_arg_copy, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO)){
           printf("initPppApp- sys_thread_new failed !!\n");
	   return 0;
	}
	//shell_init();

	return 1;
}

#if 0
static const struct snmp_mib *mibs[] = {
  &mib2,
  &mib_private
};
#endif /* LWIP_SNMP */

#if LWIP_IPV4
#if defined(LWIP_UNIX_LINUX) /* tapif is needed ONLY in Linux//unix env*/
/* (manual) host IP configuration */
static ip_addr_t ipaddr, netmask, gw;
#endif
#endif /* LWIP_IPV4 */

struct netif netif;


/* ping out destination cmd option */
#if 0 /* Not needed now */
static unsigned char ping_flag;
static ip_addr_t ping_addr;
#endif
//#ifndef TARGET_PORT
#if 0
/** @todo add options for selecting netif, starting DHCP client etc */
static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug", no_argument,        NULL, 'd'},
  /* help */
  {"help", no_argument, NULL, 'h'},
#if LWIP_IPV4
  /* gateway address */
  {"gateway", required_argument, NULL, 'g'},
  /* ip address */
  {"ipaddr", required_argument, NULL, 'i'},
  /* netmask */
  {"netmask", required_argument, NULL, 'm'},
  /* ping destination */
  {"ping",   required_argument, NULL, 'p'},
#endif /* LWIP_IPV4 */
  /* new command line options go here! */
  {NULL,   0,                 NULL,  0}
};
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)

static void init_netifs(void);

static void usage(void)
{
  unsigned char i;

  printf("options:\n");
  for (i = 0; i < NUM_OPTS; i++) {
    printf("-%c --%s\n",longopts[i].val, longopts[i].name);
  }
}
#endif
#if 0
static void
tcp_debug_timeout(void *data)
{
  LWIP_UNUSED_ARG(data);
#if TCP_DEBUG
  tcp_debug_print_pcbs();
#endif /* TCP_DEBUG */
  sys_timeout(5000, tcp_debug_timeout, NULL);
}
#endif

#if 0
void
sntp_set_system_time(u32_t sec)
{
  char buf[32];
  struct tm current_time_val;
  time_t current_time = (time_t)sec;

  localtime_r(&current_time, &current_time_val);
  
  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &current_time_val);
  printf("SNTP time: %s\n", buf);
}
#endif
#if 0
static void
tcpip_init_done(void *arg)
{
  sys_sem_t *sem;
  sem = (sys_sem_t *)arg;

  init_netifs();
#ifdef APPS
#if LWIP_IPV4
  netbiosns_set_name("simhost");
  netbiosns_init();
#endif /* LWIP_IPV4 */

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
#if LWIP_DHCP
  sntp_servermode_dhcp(1); /* get SNTP server via DHCP */
#else /* LWIP_DHCP */
#if LWIP_IPV4
  sntp_setserver(0, netif_ip_gw4(&netif));
#endif /* LWIP_IPV4 */
#endif /* LWIP_DHCP */
  sntp_init();

#if LWIP_SNMP
  lwip_privmib_init();
#if SNMP_LWIP_MIB2
#if SNMP_USE_NETCONN
  snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
#endif /* SNMP_USE_NETCONN */
  snmp_mib2_set_syscontact_readonly((const u8_t*)"root", NULL);
  snmp_mib2_set_syslocation_readonly((const u8_t*)"lwIP development PC", NULL);
  snmp_mib2_set_sysdescr((const u8_t*)"simhost", NULL);
#endif /* SNMP_LWIP_MIB2 */

  snmp_set_mibs(mibs, LWIP_ARRAYSIZE(mibs));
  snmp_init();
#endif /* LWIP_SNMP */
#endif // apps
  sys_sem_signal(sem);
}
#endif

/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_SOCKET

/* Ping using the socket api */
#if 0 /* Not needed now */
static int seq_num;

static void
ping_send(int s, const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  struct sockaddr_storage to;

  if (!(iecho = (struct icmp_echo_hdr *)malloc(sizeof(struct icmp_echo_hdr))))
    return;

  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->seqno  = htons(seq_num);
  iecho->chksum = inet_chksum(iecho, sizeof(*iecho));

#if LWIP_IPV4
  if(!IP_IS_V6(addr)) {
    struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
    to4->sin_len    = sizeof(to);
    to4->sin_family = AF_INET;
    inet_addr_from_ipaddr(&to4->sin_addr, ip_2_ip4(addr));
  }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  if(IP_IS_V6(addr)) {
    struct sockaddr_in6 *to6 = (struct sockaddr_in6*)&to;
    to6->sin6_len    = sizeof(to);
    to6->sin6_family = AF_INET6;
    inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
  }
#endif /* LWIP_IPV6 */

  lwip_sendto(s, iecho, sizeof(*iecho), 0, (struct sockaddr*)&to, sizeof(to));

  free(iecho);
  seq_num++;
}

static void
ping_recv(int s, const ip_addr_t *addr)
{
  char buf[200];
  socklen_t fromlen;
  int len;
  struct sockaddr_storage from;
  ip_addr_t ip_from;
  LWIP_UNUSED_ARG(addr);

  len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);

#if LWIP_IPV4
  if(!IP_IS_V6(addr)) {
    struct sockaddr_in *from4 = (struct sockaddr_in*)&from;
    inet_addr_to_ipaddr(ip_2_ip4(&ip_from), &from4->sin_addr);
  }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  if(IP_IS_V6(addr)) {
    struct sockaddr_in6 *from6 = (struct sockaddr_in6*)&from;
    inet6_addr_to_ip6addr(ip_2_ip6(&ip_from), &from6->sin6_addr);
  }
#endif /* LWIP_IPV6 */

  printf("Received %d bytes from %s\n", len, ipaddr_ntoa(&ip_from));
}

static void
ping_thread(void *arg)
{
  int s;
  LWIP_UNUSED_ARG(arg);

  if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  while (1) {
    printf("sending ping\n");
    ping_send(s,&ping_addr);
    ping_recv(s,&ping_addr);
    sleep(1);
  }
}
#endif
#endif /* LWIP_SOCKET */

#if LWIP_HAVE_SLIPIF
/* (manual) host IP configuration */
#if LWIP_IPV4
static ip_addr_t ipaddr_slip, netmask_slip, gw_slip;
#endif /* LWIP_IPV4 */
struct netif slipif;
#endif /* LWIP_HAVE_SLIPIF */

#if PPP_SUPPORT
sio_fd_t ppp_sio=NULL;
ppp_pcb *ppp;
struct netif pppos_netif;
extern void hw_uart_buffered_input_interrupt_handler1();
extern void hw_uart_buffered_input_interrupt_handler0();
extern int uartCharRxCnt[2];
extern int uartCharRxframeErrCnt[2];
extern int uartCharRxParityErrCnt[2];
extern int uartCharRxBrkErrCnt[2];
extern int uartCharRxOverrunErrCnt[2];
extern int uartIsrRxIndCnt[2];
extern int uartIsrTxIndCnt[2];
extern int ppp_dropCnt;
extern void bindRxUartToPPP(sendToPPP_fn send_to_PPP_fn);

struct netif *get_ppp_netif(void)
{
        if(ppp != NULL)
        {
                return ppp->netif;
        }
        return NULL;
}
char *getPppState(void)
{
	if(ppp == NULL)
	{
		return "ppp NULL pointer";
	}
	switch(ppp->ipcp_fsm.state)
	{
	case PPP_FSM_INITIAL:	/* Down, hasn't been opened */
		return "ppp state:PPP_FSM_INITIAL";
	case PPP_FSM_STARTING:	/* Down, been opened */
		return"ppp state:PPP_FSM_STARTING";
	case PPP_FSM_CLOSED:	/* Up, hasn't been opened */
		return "ppp state:PPP_FSM_CLOSED";
	case PPP_FSM_STOPPED:	/* Open, waiting for down event */
		return "ppp state:PPP_FSM_STOPPED";
	case PPP_FSM_CLOSING:	/* Terminating the connection, not open */
		return "ppp state:PPP_FSM_CLOSING";
	case PPP_FSM_STOPPING:	/* Terminating, but open */
		return "ppp state:PPP_FSM_STOPPING";
	case PPP_FSM_REQSENT:	/* We've sent a Config Request */
		return "ppp state:PPP_FSM_REQSENT";
	case PPP_FSM_ACKRCVD:	/* We've received a Config Ack */
		return "ppp state:PPP_FSM_ACKRCVD";
	case PPP_FSM_ACKSENT:	/* We've sent a Config Ack */
		return "ppp state:PPP_FSM_ACKSENT";
	case PPP_FSM_OPENED:	/* Connection available */
		return "ppp state:PPP_FSM_OPENED";
	default:
		return "ppp state:unknown";
	}
}

char *getLcpState(void)
{
	if(ppp == NULL)
	{
		return "ppp NULL pointer";
	}
	switch(ppp->lcp_fsm.state)
	{
	case PPP_FSM_INITIAL:	/* Down, hasn't been opened */
		return "lcp state:PPP_FSM_INITIAL";
	case PPP_FSM_STARTING:	/* Down, been opened */
		return"lcp state:PPP_FSM_STARTING";
	case PPP_FSM_CLOSED:	/* Up, hasn't been opened */
		return "lcp state:PPP_FSM_CLOSED";
	case PPP_FSM_STOPPED:	/* Open, waiting for down event */
		return "lcp state:PPP_FSM_STOPPED";
	case PPP_FSM_CLOSING:	/* Terminating the connection, not open */
		return "lcp state:PPP_FSM_CLOSING";
	case PPP_FSM_STOPPING:	/* Terminating, but open */
		return "lcp state:PPP_FSM_STOPPING";
	case PPP_FSM_REQSENT:	/* We've sent a Config Request */
		return "lcp state:PPP_FSM_REQSENT";
	case PPP_FSM_ACKRCVD:	/* We've received a Config Ack */
		return "lcp state:PPP_FSM_ACKRCVD";
	case PPP_FSM_ACKSENT:	/* We've sent a Config Ack */
		return "lcp state:PPP_FSM_ACKSENT";
	case PPP_FSM_OPENED:	/* Connection available */
		return "lcp state:PPP_FSM_OPENED";
	default:
		return "lcp state:unknown";
	}
}

void printPPPState(void)
{
  if(ppp != NULL)
  {
//  printf("timer executed:%d\r\n",ppp->timer_execute);
    printf("protocol type:%x\r\n",ppp->ipcp_fsm.protocol);
    printf("flags :%x\r\n",ppp->ipcp_fsm.flags);
    printf("retransmits:%d\r\n",ppp->ipcp_fsm.retransmits);
    printf("nakloops:%d\r\n",ppp->ipcp_fsm.nakloops);
    printf("rnakloops:%d\r\n",ppp->ipcp_fsm.rnakloops);
    printf("maxnakloops:%d\r\n",ppp->ipcp_fsm.maxnakloops);

    if( (ppp->ipcp_fsm.term_reason_len != 0) && (ppp->ipcp_fsm.term_reason != NULL) )
    {
      printf("termination last reason:%s\r\n",ppp->ipcp_fsm.term_reason);
    }
    printf("chap_max_transmits:%d\r\n",ppp->settings.chap_max_transmits);
     printf("chap_timeout_time:%d\r\n",ppp->settings.chap_timeout_time);
    printf("fsm_max_conf_req_transmits:%d\r\n",ppp->settings.fsm_max_conf_req_transmits);
    printf("fsm_max_nak_loops:%d\r\n",ppp->settings.fsm_max_nak_loops);
    printf("fsm_timeout_time:%d\r\n",ppp->settings.fsm_timeout_time);
    printf("lax_recv:%d\r\n",ppp->settings.lax_recv);
    printf("lcp_echo_fails:%d\r\n",ppp->settings.lcp_echo_fails);
    printf("lcp_echo_interval:%d\r\n",ppp->settings.lcp_echo_interval);
    printf("lcp_loopbackfail:%d\r\n",ppp->settings.lcp_loopbackfail);
    printf("listen_time:%d\r\n",ppp->settings.listen_time);
    printf("lcp_echos_pending:%d\r\n",ppp->lcp_echos_pending);
    printf("tcpip_packet_received:%d\r\n",tcpip_packet_received);
  }
  printf("%s\r\n",getPppState());
  printf("%s\r\n",getLcpState());
}

BaseType_t pppend2end_cmd( int8_t *pcWriteBuffer,
	  	  size_t xWriteBufferLen,
		  const int8_t *pcCommandString ,
		  void *flush_func,
		  void *flush_arg,
		  int argc,void *argv[])
{
	int flags=0;
	int cli_flag=0;
	int i=0;

    for (i=1; i<argc; i++)
    {
        if (argv[i] == NULL)
            continue;

        if( (strcmp(argv[i],"-atdt") == 0) && (i+1 < argc) )
        {
        	flags = atoi(argv[i+1]);
        	set_atdt_flag(flags);
			i++;
			cli_flag++;
        }
        if( (strcmp(argv[i],"-d") == 0) && (i+1 < argc) )
        {
        	flags = atoi(argv[i+1]);
        	set_ppp_new_phase(flags);
        	//set_print_ppp_state(flags);
			i++;
			cli_flag++;
        }
        if( (strcmp(argv[i],"-t") == 0) && (i+1 < argc) )
        {
        	flags = atoi(argv[i+1]);
        	set_pppEnd2Endtimeout(flags);
        	cli_flag++;
        }

    }

#if 0
    if(cli_flag == 0)
    {
    	CLI_PF ("usage: pppe2e -atdt <0/1> -d <0/1> -t <timeout> [options]\r\n");
    }
#endif

	return pdFALSE;
}

//void upper_layers_send_link_termination(ppp_pcb *pcb,const char *reason);
void fsm_sdata_lcp_terminate(fsm *f, u_char code, u_char id, const u_char *data, int datalen);

int is_ppp_in_close_session(void)
{
	int close_session=0;
	if(ppp != NULL)
	{
		switch(ppp->ipcp_fsm.state)
		{
			case PPP_FSM_CLOSED:
			case PPP_FSM_STOPPED:
			case PPP_FSM_CLOSING:
				close_session=1;
				break;
			default:
				close_session=0;
				break;
		}
	}
	return close_session;
}

void ppp_restart_session(void)
{
	if(ppp != NULL)
	{
//		printf("ppp state:%s\r\n",print_ppp_stat(ppp->phase));
		if(ppp->phase == PPP_PHASE_AUTHENTICATE)
		{
			fsm *f = &ppp->ipcp_fsm;
			if(f->callbacks->resetci)
			{
				f->callbacks->resetci(f);
			}
			link_established(ppp);
		}
	}
}

int ppp_up_session(void)
{
	if(ppp != NULL)
	{
		switch(ppp->ipcp_fsm.state)
		{
			case PPP_FSM_OPENED:
				return 1;
			default:
				return 0;
		}
	}
	return 0;
}


void close_ppp_session(void)
{
	fsm *f=NULL;
	if(ppp != NULL)
	{
		switch(ppp->ipcp_fsm.state)
		{
			case PPP_FSM_REQSENT:
			case PPP_FSM_ACKRCVD:
			case PPP_FSM_ACKSENT:
			case PPP_FSM_OPENED:
				f = &ppp->ccp_fsm;
				f->term_reason=NULL;
				f->term_reason_len=0;
				fsm_sdata_lcp_terminate(f, TERMREQ, f->reqid = ++f->id,
					  (const u_char *) f->term_reason, f->term_reason_len);
				break;
			default:
				break;
		}
	}
}

BaseType_t pppStatus_cmd( int8_t *pcWriteBuffer,
  	  	  size_t xWriteBufferLen,
		  const int8_t *pcCommandString ,
		  void *flush_func,
		  void *flush_arg,
		  int argc,void *argv[])
{
	int pppPrintFlag=0;
//	const ip_addr_t *ns;
	if(ppp != NULL)
	{
//		printPPPState(pcWriteBuffer,xWriteBufferLen,flush_func,flush_arg);
//		CLI_PF("%s\r\n",getPppState());
//        ns = (const ip_addr_t *)dns_getserver(0);
//        CLI_PF("dns1 = %s\n\r", ipaddr_ntoa(ns));
//        ns = dns_getserver(1);
//        CLI_PF("dns2 = %s\n\r", ipaddr_ntoa(ns));
	}
	if(argc > 1)
	{
		pppPrintFlag = atoi(argv[1]);

		if(pppPrintFlag == 100)
		{
//			CLI_PF("close the ppp session\n\r");
			close_ppp_session();
		}
		if(pppPrintFlag == 200)
		{
//			CLI_PF("open the ppp session\n\r");
			ppp_startup();
		}
		set_print_ppp_message_flag(pppPrintFlag);
	}
	return pdFALSE;
}

int get_tcpip_packet_received(void)
{
	return tcpip_packet_received;
}

void sendToPPP(u8_t *s, int l)
{
	tcpip_packet_received++;

	if(ppp != NULL)
	{
		pppos_input_tcpip(ppp, s, l);
	}
}
#if 0/* Debug thread run for printing serial uart stats errors */
static void uart_pollRx_thread(void *arg)
{
	int cnt=0;
	LWIP_UNUSED_ARG(arg);
	vTaskDelay(2000);/* to make sure that sio is setup already */
	printf("uart_pollRx_thread Task started\n");
	while (1)
	{
		printf("Task start\n");
//		hw_uart_buffered_input_interrupt_handler1();
		vTaskDelay(8000);/*Every 200mSec*/
		printf("Task end\n");
		if ((++cnt % 4)==0)
		   printf("UART1 -loop cnt=%d IsrRxIndCnt=%d IsrTxIndCnt=%d RxCnt=%d frameErr=%d parityErr=%d BrkErr=%d overrunErr=%d TxOverDrop=%d\n",
				   cnt,
				   uartIsrRxIndCnt[1],
				   uartIsrTxIndCnt[1],
				   uartCharRxCnt[1],
				   uartCharRxframeErrCnt[1],
				   uartCharRxParityErrCnt[1],
				   uartCharRxBrkErrCnt[1],
				   uartCharRxOverrunErrCnt[1],
				   ppp_dropCnt);
		if ((cnt % 5)==0)
		   printf("UART0 -loop cnt=%d IsrRxIndCnt=%d IsrTxIndCnt=%d RxCnt=%d frameErr=%d parityErr=%d BrkErr=%d overrunErr=%d\n",
				   cnt,
				   uartIsrRxIndCnt[0],
				   uartIsrTxIndCnt[0],
				   uartCharRxCnt[0],
				   uartCharRxframeErrCnt[0],
				   uartCharRxParityErrCnt[0],
				   uartCharRxBrkErrCnt[0],
				   uartCharRxOverrunErrCnt[0]);

	}

}
#endif

void ppp_print_ip_detail(struct netif *pppif)
{
#if LWIP_DNS
        const ip_addr_t *ns;
#endif /* LWIP_DNS */

        if(pppif == NULL)
        {
        	return;
        }

#if LWIP_IPV4
        if (pppif->ip_addr.u_addr.ip4.addr != 0)
        {
			printf( "our_ip4addr = %s\n\r", ip4addr_ntoa(netif_ip4_addr(pppif)));
			printf("his_ipaddr  = %s\n\r", ip4addr_ntoa(netif_ip4_gw(pppif)));
			printf("netmask     = %s\n\r", ip4addr_ntoa(netif_ip4_netmask(pppif)));
        }
#endif /* LWIP_IPV4 */

#if LWIP_DNS
        ns = dns_getserver(0);
        if( (ns != NULL) && (ns->u_addr.ip4.addr != 0 ) )
        {
        	printf("dns1        = %s\n\r", ipaddr_ntoa(ns));
        }
        ns = dns_getserver(1);
        if( (ns != NULL) && (ns->u_addr.ip4.addr != 0 ) )
        {
        	printf("dns2        = %s\n\r", ipaddr_ntoa(ns));
        }
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
        printf("our6_ipaddr = %s\n\r", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */


//        printf("IPV4: Host at %s ", ip4addr_ntoa(netif_ip4_addr(nif)));
//         printf("mask %s ", ip4addr_ntoa(netif_ip4_netmask(nif)));
//         printf("gateway %s\n", ip4addr_ntoa(netif_ip4_gw(nif)));

}

static void
ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    //struct netif *pppif = ppp_netif(pcb);
	static int cnt=1;
	static int cntLost=1;

    LWIP_UNUSED_ARG(ctx);
    globPppInterfaceUp=pdFALSE;
    switch(err_code) {
    case PPPERR_NONE:               /* No error. */
        {
        	cnt=1;
        	globPppInterfaceUp=pdTRUE;

        }
        break;

    case PPPERR_PARAM:             /* Invalid parameter. */
        printf("ppp_link_status_cb: PPPERR_PARAM\n");
        break;

    case PPPERR_OPEN:              /* Unable to open PPP session. */
        printf("ppp_link_status_cb: PPPERR_OPEN\n");
        break;

    case PPPERR_DEVICE:            /* Invalid I/O device for PPP. */
        printf("ppp_link_status_cb: PPPERR_DEVICE\n");
        break;

    case PPPERR_ALLOC:             /* Unable to allocate resources. */
        printf("ppp_link_status_cb: PPPERR_ALLOC\n");
        break;

    case PPPERR_USER:              /* User interrupt. */
        printf("ppp_link_status_cb: PPPERR_USER\n");
        break;

    case PPPERR_CONNECT:           /* Connection lost. */
        printf("ppp_link_status_cb: PPPERR_CONNECT -  Connection lost.cnt=%d\n",cntLost);
        if ((cntLost%5) == 0)
    	{
//          if (ppp_sio && sioToggle(ppp_sio)){
          if (ppp_sio){
//			ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
			ppp_release_sem(0);
          }
        }
        cntLost++;
        break;

    case PPPERR_AUTHFAIL:          /* Failed authentication challenge. */
        printf("ppp_link_status_cb: PPPERR_AUTHFAIL\n");
        break;

    case PPPERR_PROTOCOL:          /* Failed to meet protocol. */
        printf("ppp_link_status_cb: PPPERR_PROTOCOL\n");
        break;

    case PPPERR_PEERDEAD:          /* Connection timeout. */
       {printf("ppp_link_status_cb: PPPERR_PEERDEAD -Connection timeout.cnt=%d\n",cnt);
	   if ((cnt%3) == 0){
//     	   if (ppp_sio && sioToggle(ppp_sio)){
     	   if (ppp_sio){
//			 ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
			 ppp_release_sem(0);
			 cnt=0;
       	   }
     	   else
     	    cnt=0;
	   }
		cntLost=0;
        cnt++;
       }
        break;

    case PPPERR_IDLETIMEOUT:       /* Idle Timeout. */
        printf("ppp_link_status_cb: PPPERR_IDLETIMEOUT\n");
        break;

    case PPPERR_CONNECTTIME:       /* PPPERR_CONNECTTIME. */
        printf("ppp_link_status_cb: PPPERR_CONNECTTIME\n");
        break;

    case PPPERR_LOOPBACK:          /* Connection timeout. */
        printf("ppp_link_status_cb: PPPERR_LOOPBACK\n");
        break;

    default:
        printf("ppp_link_status_cb: unknown errCode %d\n", err_code);
        break;
    }
    if (globPppInterfaceUp==pdFALSE)
    {
    	LWIP_DEBUGF(LOG_NOTICE, ("calling ppp_connect() \n"));
    	ppp_connect(ppp, 0);
    }
}

static u32_t
ppp_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(ctx);
  return sio_write(ppp_sio, data, len);

}
#endif

#if LWIP_NETIF_STATUS_CALLBACK
static void
netif_status_callback(struct netif *nif)
{
	const ip6_addr_t *ipv6;
	const ip4_addr_t *ipv4;

	int ppp_going_down=0;

  printf("NETIF: %c%c%d is %s\n", nif->name[0], nif->name[1], nif->num,
         netif_is_up(nif) ? "UP" : "DOWN");


#if LWIP_NETIF_HOSTNAME
  printf("FQDN: %s\n", netif_get_hostname(nif));
#endif /* LWIP_NETIF_HOSTNAME */
  printf("ppp state:%d\r\n",ppp->phase);

  ipv6 = netif_ip6_addr(nif, 0) ;
  ipv4 = netif_ip4_addr(nif);

  printf("ppp state:%d ipv4:%d.%d.%d.%d\r\n",
		  ppp->phase,
		  (ipv4->addr & 0xFF000000) >> 24,
		  (ipv4->addr & 0x00FF0000) >> 16,
		  (ipv4->addr & 0x0000FF00) >> 8,
		  (ipv4->addr & 0x000000FF));

  if( (ipv4->addr == 0) && (ipv6->addr[0] == 0) && (pppe2e_during_activation() == 0) )
  {
	  printf("ppp going down!!!\r\n");
	  ppp_going_down=1;
//      if (ppp_sio && sioToggle(ppp_sio)){
      if (ppp_sio){
//		   ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
		   ppp_release_sem(0);
      }
  }
  else if( (ipv4->addr != 0) && (netif_is_up(nif) == 1) )
  {
	  stop_timer_ppp_ened2end();
  }

  if(gPppLastlinkState != netif_is_up(nif))
  {
	  if(netif_is_up(nif) == 0)
	  {
		  // down
		  if(ppp != NULL)
		  {
			  printf("PPP link down\r\n");
			  ppp_going_down=1;
//		      if (ppp_sio && sioToggle(ppp_sio))
		      if (ppp_sio)
		      {
//				   ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
				   ppp_release_sem(0);
		      }
		  }
	  }
/*
	  else
	  {
		  // up
		  if(ppp != NULL)
		  {
			  ppp_start(ppp);
			  printf("PPP start\r\n");
		  }
	  }
*/
  }
  gPppLastlinkState = netif_is_up(nif);
  ppp_print_ip_detail(nif);

  if(ppp_going_down == 1)
  {
	  ppp_release_sem(0);
  }

}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

void ppp_close_session(void)
{
	if(ppp)
	{
		ppp_close(ppp,0);
	}
}

void ppp_startup(void)
{
//	fsm *f=NULL;
	if(ppp){//if ppp was NOt sytarted ignore and do nothing
//		f = &ppp->ccp_fsm;
//		f->term_reason=NULL;
//		f->term_reason_len=0;
//		fsm_sdata_lcp_terminate(f, TERMREQ, f->reqid = ++f->id,
//			  (const u_char *) f->term_reason, f->term_reason_len);
	   ppp_start(ppp);
	}
}

void ppp_e2e_activation(void)
{
	//ppp_pcb *pcb = (ppp_pcb*)ppp;
	//fsm *f = &pcb->ipcp_fsm;
	//f->callbacks->resetci(f);
	//link_established(pcb);
	//ppp_startup();
	ppp_restart_session();
}

static void
init_netifs(pppos_arg_t *pppos_arg_ptr)
{
#if LWIP_HAVE_SLIPIF
#if SLIP_PTY_TEST
  u8_t siodev_slip = 3;
#else
  u8_t siodev_slip = 0;
#endif

#if LWIP_IPV4
  netif_add(&slipif, ip_2_ip4(&ipaddr_slip), ip_2_ip4(&netmask_slip), ip_2_ip4(&gw_slip),
            (void*)&siodev_slip, slipif_init, tcpip_input);
#else /* LWIP_IPV4 */
  netif_add(&slipif, (void*)&siodev_slip, slipif_init, tcpip_input);
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&slipif, 1);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&slipif, netif_status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
  netif_set_link_up(&slipif);
  netif_set_up(&slipif);
#endif /* LWIP_HAVE_SLIPIF */

#if PPP_SUPPORT
#if PPP_PTY_TEST
  ppp_sio = sio_open(2);
#else
  ppp_sio = sio_open((void *)pppos_arg_ptr);
#endif
  free(pppos_arg_ptr);
  if(!ppp_sio)
  {
//      perror("Error opening device: ");
      return;
  }

  bindRxUartToPPP(sendToPPP);

  ppp = pppos_create(&pppos_netif, ppp_output_cb, ppp_link_status_cb, NULL);
  if (!ppp)
  {
      printf("Could not create PPP control interface");
      exit(1);
  }
  //print_boot_msg("init_netifs() pppos_create was return with %p\n",ppp);
#ifdef LWIP_PPP_CHAP_TEST
  ppp_set_auth(ppp, PPPAUTHTYPE_CHAP, "lwip", "mysecret");
#endif
  ppp_set_auth(ppp, PPPAUTHTYPE_NONE, NULL, NULL);

  
  ppp->ppp_connect_e2e_cb=NULL;
  ppp->ppp_timer_e2e_cb=NULL;
/* Even that we act as server side we need to initiate connect and make this calling to ppp_connect */
  ppp_connect(ppp, 0);
//  ppp->ppp_connect_e2e_cb = serialSwitchSetE2efunc();
//  ppp->ppp_timer_e2e_cb = serialSwitchSetE2eTimer();
//  cb_function_ppp_termination(close_ppp_session);

//  set_callback_received_buffer(get_tcpip_packet_received);

#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&pppos_netif, netif_status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#endif /* PPP_SUPPORT */
  
#if defined(LWIP_UNIX_LINUX) /* tapif is needed ONLY in Linux//unix env*/
#if LWIP_IPV4
#if LWIP_DHCP
  IP_ADDR4(&gw,      0,0,0,0);
  IP_ADDR4(&ipaddr,  0,0,0,0);
  IP_ADDR4(&netmask, 0,0,0,0);
#endif /* LWIP_DHCP */
  netif_add(&netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw), NULL, tapif_init, tcpip_input);
#else /* LWIP_IPV4 */
  netif_add(&netif, NULL, tapif_init, tcpip_input);
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&netif, 1);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&netif, netif_status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
  netif_set_default(&netif);
  netif_set_up(&netif);

#if LWIP_DHCP
  dhcp_start(&netif);
#else
#if LWIP_AUTOIP
  autoip_start();
#endif
#endif /* LWIP_DHCP */
#endif /* LWIP_UNIX_LINUX */
#if 0
  /* Only used for testing purposes: */
  netif_add(&ipaddr, &netmask, &gw, NULL, pcapif_init, tcpip_input);
#endif


#if LWIP_IPV6_AUTOCONFIG /* comment it was #if  */
  /* IPv6 address autoconfiguration not enabled by default */
  netif.ip6_autoconfig_enabled = 1;
#endif /* LWIP_IPV6_AUTOCONFIG */
#ifdef LWIP_ALT_SUPPORT_RA
  ppp->netif->ipv6_router_adver_mode=1; // we are in router mode
#endif
#ifdef APPS
#if LWIP_TCP
  netio_init();
#endif
#if LWIP_TCP && LWIP_NETCONN
  tcpecho_init();
  shell_init();
  httpd_init();
#endif
#if LWIP_UDP && LWIP_NETCONN  
  udpecho_init();
#endif  
#if LWIP_SOCKET
  chargen_init();
#endif
#endif /* APPS */
//  shell_init(); /* telnet server setup */
  /*  httpd_init();  http server setup */
  /*  sys_timeout(5000, tcp_debug_timeout, NULL);*/
  pppapi_set_default(ppp);
}

#ifdef TARGET_PORT
int ppp_app_entry(void *arg)
{
/* PPP thread is being initiated by serial app mngr */
	return 0;
}
#endif

static sys_sem_t pppsem;
static int ppp_sleep_before_down=0;

void ppp_release_sem(int sleep)
{
	printf("release sem\n");
	if(ppp != NULL)
	{
		ppp_sleep_before_down = sleep;
		sys_mutex_unlock(&pppsem);
	}
}

void free_alloc_pbuf(void)
{
	struct memp *bufArry[50];
	int max_buffers=50;
	struct pbuf *pbuff;
	int i=0;

	get_memp_buffers(MEMP_PBUF_POOL,&bufArry[0],&max_buffers);

	for(i=0;i<max_buffers;i++)
	{
		pbuff = (struct pbuf *)bufArry[i];

		if( (pbuff->ref > 0) && (pbuff->tot_len < 32) )
		{
			pbuf_free(pbuff);
		}
	}
}



/*-----------------------------------------------------------------------------------*/
#ifdef TARGET_PORT
void ppp_app_main(void *arg)
#else
//static void  main_thread(void *arg)
#endif
{
  //sys_sem_t sem;
#ifndef TARGET_PORT
  LWIP_UNUSED_ARG(arg);
#else
  pppos_arg_t *pppos_arg_ptr = (pppos_arg_t *) arg;
#endif
  set_ppp_thread_status(1);
//  debug_flags |= (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
  taskENTER_CRITICAL();
  //print_boot_msg("Application compile time %s %s %d\n",__DATE__,__TIME__,__LINE__);
  taskEXIT_CRITICAL();

  if(sys_sem_new(&pppsem, 0) != ERR_OK) {
    LWIP_ASSERT("Failed to create ppp application semaphore", 0);
  }
//  init_pppEnd2EndApp(((serial_appp_mngr_msg_t *)arg)->logical_serial_id);
  init_netifs(pppos_arg_ptr);
  printf("Applications started.\r\n");

  /* Wait forever  */
   sys_sem_wait(&pppsem);
   printf("sem released wait few seconds:%d before ppp going down\n",ppp_sleep_before_down);
   set_ppp_thread_status(0);
   if(ppp != NULL)
   {
	   printf("sizeof ppp:%d\r\n",sizeof(struct ppp_pcb_s));
	   if(ppp_sleep_before_down != 0)
	   {
		   //sleep(ppp_sleep_before_down); // wait few seconds
		   vTaskDelay(1000*ppp_sleep_before_down); // wait few seconds
	   }
	   close_ppp_session();
	   vTaskDelay(1000*1);
//	   ppp->lcp_fsm.state = PPP_FSM_CLOSING;
//	   lcp_lowerdown(ppp);
//	   ppp->lcp_fsm.state = PPP_FSM_CLOSING;
//	   fsm_lowerdown(ppp->lcp_fsm);
	   ppp_close(ppp,0);
	   ppp->phase = PPP_PHASE_DEAD;
	   ppp_free(ppp);
	   ppp=NULL;

	   //free_alloc_pbuf();
	   // need to deallocate buffers
	   printf("ppp ended\n");
   }
	sys_sem_free(&pppsem);
	//serialMngrUnbind2SerialPortApi(LOGICAL_PORT_TO_PPP_APP_SWITCH_PORT1);
	free_alloc_pbuf();

   vTaskDelete( NULL );
   //return NULL;
}

#ifndef TARGET_PORT
/*-----------------------------------------------------------------------------------*/
int
ppp_app_entry(int argc, char **argv)
{
  int ch;
  char ip_str[16] = {0};



  printf("\n%s %s %d\n\n",__FILE__,__FUNCTION__,__LINE__);

  /* startup defaults (may be overridden by one or more opts) */
#if LWIP_IPV4
  IP_ADDR4(&gw,      10,0,  0,1);
  IP_ADDR4(&netmask, 255,255,255,0);
  IP_ADDR4(&ipaddr,  10,0,  0,88);
#if LWIP_HAVE_SLIPIF
  IP_ADDR4(&gw_slip,      192,168,  2,  1);
  IP_ADDR4(&netmask_slip, 255,255,255,255);
  IP_ADDR4(&ipaddr_slip,  192,168,  2,  2);
#endif
#endif /* LWIP_IPV4 */
  
  ping_flag = 0;
  /* use debug flags defined by debug.h */
  debug_flags = LWIP_DBG_OFF;
  
  while ((ch = getopt_long(argc, argv, "dhg:i:m:p:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_flags |= (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
        break;
      case 'h':
        usage();
        exit(0);
        break;
#if LWIP_IPV4
      case 'g':
        ipaddr_aton(optarg, &gw);
        break;
      case 'i':
        ipaddr_aton(optarg, &ipaddr);
        break;
      case 'm':
        ipaddr_aton(optarg, &netmask);
        break;
#endif /* LWIP_IPV4 */
      case 'p':
        ping_flag = !0;
        ipaddr_aton(optarg, &ping_addr);
        strncpy(ip_str,ipaddr_ntoa(&ping_addr),sizeof(ip_str));
        ip_str[sizeof(ip_str)-1] = 0; /* ensure \0 termination */
        printf("Using %s to ping\n", ip_str);
        break;
      default:
        usage();
        break;
    }
  }
  argc -= optind;
  argv += optind;
  
#ifdef PERF
  perf_init("/tmp/simhost.perf");
#endif /* PERF */

  printf("System initialized.\n");

  sys_thread_new("main_thread", main_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);

  /* Start the tasks running. */
  vTaskStartScheduler();

 /* If all is well we will never reach here as the scheduler will now be
    running.  If we do reach here then it is likely that there was insufficient
    heap available for the idle task to be created. */
 configASSERT( 0 ); /* was pause() */
 for( ;; );

  return 0;
}
#endif
/*-----------------------------------------------------------------------------------*/
