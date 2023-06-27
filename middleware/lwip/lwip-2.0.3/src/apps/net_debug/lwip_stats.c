/*  ---------------------------------------------------------------------------

	(c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include <task.h>
#include "queue.h"
#include "semphr.h"
#include <time.h>
#include <timers.h>
//#include <pthread.h>
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/init.h"
#include "lwip/inet_chksum.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/dhcp6.h"
#include "lwip/nd6.h"
#include "lwip/priv/nd6_priv.h"
#include "lwip/prot/nd6.h"
#include "lwip/netbuf.h"


#define malloc(size) pvPortMalloc(size)
#define free(ptr) vPortFree(ptr)

//extern int convert_param_to_argv (const int8_t *pcCommandString, char **argv, int *argc);
int print_lwip_information(void);
int print_lwip_table();
int nd6_path_mtu_discovery_print(char *buffer,int max_buffer_size);

//int send_tcpip_reschedule(void);
int get_queue_message_rcv_counter(void);

int nd6_print_all_destination_cache_entry(char *entry,int max_entries);
int nd6_create_destination_cache_entry(struct nd6_destination_cache_entry *entry);
int nd6_delete_destination_cache_entry(struct nd6_destination_cache_entry *entry);

#if DEBUG_PBUF_ASSIGNED
void print_pbuf_params(void);
#endif

int nd6_route_print(char *buffer,int max_buffer_size);

#if USE_MEMP_DYNAMIC_ALLOCATION
unsigned int get_number_of_memp_used(void);
#endif


#define	  D_LWIP_SHOW_ALL_COUNTERS				0x00000001
#define	  D_LWIP_SHOW_LINK_COUNTERS				0x00000002
#define	  D_LWIP_SHOW_ETHARP_COUNTERS			0x00000004
#define	  D_LWIP_SHOW_FRAGMENT_COUNTERS			0x00000008
#define	  D_LWIP_SHOW_IPV4_COUNTERS				0x00000010
#define	  D_LWIP_SHOW_IPV6_COUNTERS				0x00000020
#define	  D_LWIP_SHOW_ICMPV4_COUNTERS			0x00000040
#define	  D_LWIP_SHOW_ICMPV6_COUNTERS			0x00000080
#define	  D_LWIP_SHOW_TCPV4_COUNTERS			0x00000100
#define	  D_LWIP_SHOW_UDPV4_COUNTERS			0x00000200
#define	  D_LWIP_SHOW_ND6_COUNTERS				0x00000400
#define	  D_LWIP_SHOW_SYS_COUNTERS				0x00000800
#define	  D_LWIP_SHOW_PPP_COUNTERS				0x00001000
#define	  D_LWIP_SHOW_DHCP_COUNTERS				0x00002000
#define	  D_LWIP_SHOW_DHCP6_COUNTERS			0x00004000


#define	  D_LWIP_SHOW_IGMP_COUNTERS				0x00000001
#define	  D_LWIP_SHOW_HEAP_COUNTERS				0x00000002
#define	  D_LWIP_SHOW_MEM_COUNTERS				0x00000004
#define	  D_LWIP_SHOW_MLD6_COUNTERS				0x00000008
#define	  D_LWIP_CLEAN_COUNTERS					0x00000010
#define   D_LWIP_SHOW_TIMER_COUNTERS			0x00000020

#define	  D_LWIP_INTERFACE_NAME_SIZE			10

#define   TMP_BUF_LEN							100
#define   IFCONFIG_BUF_LEN						500

struct lwipStat
{
	unsigned int options[3];

};

struct lwipIfconfig
{
	int b_interface_name;
	char name[D_LWIP_INTERFACE_NAME_SIZE];
	int b_clean_counter;

};

struct clatconfig
{
	int b_show_global_params;
};

struct pathmtuconfig
{
	int b_show_params;
	int b_create_ipv6_params;
	int b_delete_ipv6_params;
	struct nd6_destination_cache_entry entry;
};

typedef int (*counter_callback)(void *p,char *name);

struct lwipopt
{
	char					protocol_name[8];
	unsigned int 			bitmak;
	char					abbr[8];
	counter_callback		cb_counter;
	void					*counter;
};

int show_specific_counter(void *p,char *name);
int show_all_counter(void *p,char *name);
int show_igmp_counter(void *p,char *name);
int show_heap_counter(void *p,char *name);
int show_mem_counter(void *p,char *name);
int clean_lwip_counter(void *p,char *name);
int show_timer_counter(void *p,char *name);
int show_sys_counter(void *p,char *name);
int show_ppp_counter(void *p,char *name);
#if  LWIP_DHCP
int show_dhcp_counter(void *p,char *name);
#endif
#if LWIP_IPV6_DHCP6
int show_dhcp6_counter(void *p,char *name);
#endif
void print_free_ram_mem(void);
void set_lwip_level_log(int level);
void clear_lwip_level_log(int level);

unsigned long get_mbox_tcpip_msg_waiting(void);
u32_t get_mbox_tcpip_msg_value(void);
int islinkListEmpty(void);
int nunOfEntriesInlinkList(void);
void lwiperf_udp_abort(void *handle);

static struct lwipopt lwip_state[] = {

		{"all",D_LWIP_SHOW_ALL_COUNTERS,"-a",show_all_counter,NULL},
#if LINK_STATS
		{"link",D_LWIP_SHOW_LINK_COUNTERS,"-l",show_specific_counter,(void *)&lwip_stats.link},
#endif
#if ETHARP_STATS
		{"arp",D_LWIP_SHOW_ETHARP_COUNTERS,"-e",show_specific_counter,(void *)&lwip_stats.etharp},
#endif
#if IPFRAG_STATS
		{"frag",D_LWIP_SHOW_FRAGMENT_COUNTERS,"-f",show_specific_counter,(void *)&lwip_stats.ip_frag},
#endif
#if IP_STATS
		{"ip4",D_LWIP_SHOW_IPV4_COUNTERS,"-ip4",show_specific_counter,(void *)&lwip_stats.ip},
#endif
#if ICMP_STATS
		{"icmp4",D_LWIP_SHOW_ICMPV4_COUNTERS,"-icmp4",show_specific_counter,(void *)&lwip_stats.icmp},
#endif
#if UDP_STATS
		{"udp",D_LWIP_SHOW_UDPV4_COUNTERS,"-udp",show_specific_counter,(void *)&lwip_stats.udp},
#endif
#if TCP_STATS
		{"tcp",D_LWIP_SHOW_TCPV4_COUNTERS,"-tcp",show_specific_counter,(void *)&lwip_stats.tcp},
#endif
#if IP6_STATS
		{"ip6",D_LWIP_SHOW_IPV6_COUNTERS,"-ip6",show_specific_counter,(void *)&lwip_stats.ip6},
#endif
#if ICMP6_STATS
		{"icmp6",D_LWIP_SHOW_ICMPV6_COUNTERS,"-icmp6",show_specific_counter,(void *)&lwip_stats.icmp6},
#endif
#if ND6_STATS
		{"nd6",D_LWIP_SHOW_ND6_COUNTERS,"-nd6",show_specific_counter,(void *)&lwip_stats.nd6},
#endif


};

static struct lwipopt lwip_special_state[] = {
#if IGMP_STATS
		{"igmp",D_LWIP_SHOW_IGMP_COUNTERS,"-igmp",show_igmp_counter,(void *)&lwip_stats.igmp},
#endif
#if MEM_STATS
		{"heap",D_LWIP_SHOW_HEAP_COUNTERS,"-heap",show_heap_counter,(void *)&lwip_stats.mem},
#endif
#if MEM_STATS
		{"mem",D_LWIP_SHOW_MEM_COUNTERS,"-mem",show_mem_counter,(void *)lwip_stats.memp},
#endif
#if MLD6_STATS
		{"mld",D_LWIP_SHOW_MLD6_COUNTERS,"-mld",show_igmp_counter,(void *)&lwip_stats.mld6},
#endif
		{"cln",D_LWIP_CLEAN_COUNTERS,"-c",clean_lwip_counter,NULL},

		{"timer",D_LWIP_SHOW_TIMER_COUNTERS,"-timer",show_timer_counter,NULL},

		{"sys",D_LWIP_SHOW_SYS_COUNTERS,"-sys",show_sys_counter,(void *)&lwip_stats.sys},

		{"ppp",D_LWIP_SHOW_PPP_COUNTERS,"-ppp",show_ppp_counter,NULL},
#if  LWIP_DHCP
		{"dhcp",D_LWIP_SHOW_DHCP_COUNTERS,"-dhcp",show_dhcp_counter,NULL},
#endif
#if LWIP_IPV6_DHCP6
		{"dhcp6",D_LWIP_SHOW_DHCP6_COUNTERS,"-dhcp6",show_dhcp6_counter,NULL},
#endif

};
int print_lwip_igmp_information()
{
	int i=0;
	char stat_msgs_proto[] ="Name |Tx   |Rx   |drop |cs er|Len  |Mem  |Proto|v1   |rx gr|rx qy|rx rp|tx jn|tx lv|tx rp|";
	char stat_msgs_space[] ="=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|";
	char *stat_msgs_explain[14] = {
	  "Tx-transmitted",
	  "Rx-received",
	  "drop-dropped",
	  "cs er-checksum errors",
	  "Len-length errors",
	  "mem -memory errors",
	  "Proto-protocol errors",
	  "vi -Received v1 frames",
	  "rx gr - Received group-specific queries",
	  "rx qy - Received general queries.",
	  "rx rp - Received reports.",
	  "tx jn - Sent joins.",
	  "tx lv - Sent leaves.",
	  "tx rp - Sent reports.",
	};

	for(i=0;i<14;i++)
	{
		printf("%s\r\n",stat_msgs_explain[i]);
	}


	printf("%s\r\n",stat_msgs_proto);
	printf("%s\r\n",stat_msgs_space);
	return 0;
}

int show_igmp_counter(void *p,char *name)
{
	int i=0;
	int len=0;
	char buff[TMP_BUF_LEN];
	struct stats_igmp *pProto=(struct stats_igmp *)p;

	memset(&buff[0],0,sizeof(buff));

	snprintf(&buff[len],(TMP_BUF_LEN-len),"%s",name);
	len+=5;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->xmit);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->recv);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->drop);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->chkerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->lenerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->memerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->proterr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->rx_v1);			/* Received v1 frames. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->rx_group);			/* Received group-specific queries. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->rx_general);	   /* Received general queries. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->rx_report);		/* Received reports. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->tx_join);		  /* Sent joins. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->tx_leave);		 /* Sent leaves. */
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->tx_report);		/* Sent reports. */
	len+=6;

	for(i=0;i<len;i++)
	{
		if(buff[i] == 0)
		{
			buff[i]=' ';
		}
	}

	print_lwip_igmp_information();

	printf("%s\r\n",buff);
	return 1;

}

int print_lwip_mem_information()
{
	char stat_msgs_proto[] ="Name		   |err  |avail|used |max  |illeg|total size|";
	char stat_msgs_space[] ="===============|=====|=====|=====|=====|=====|==========|";


	printf("%s\r\n",stat_msgs_proto);
	printf("%s\r\n",stat_msgs_space);
	return 0;
}


int show_heap_counter(void *p, char* name)
{
	int len=0;
	char buff[TMP_BUF_LEN];
	int i=0;
	struct stats_mem *pProto=(struct stats_mem *)p;

	memset(&buff[0],0,sizeof(buff));

	snprintf(&buff[len],(TMP_BUF_LEN-len),"%s",name);
	len+=15;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->err);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->avail);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->used);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->max);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->illegal);

	for(i=0;i<len;i++)
	{
		if(buff[i] == 0)
		{
			buff[i]=' ';
		}
	}

	print_lwip_mem_information();

	printf("%s\r\n",buff);
	return 1;

}

int show_mem_counter(void *p, char* name)
{
	int len=0;
	char buff[TMP_BUF_LEN];
	int i=0;
	int j=0;
	struct memp *bufArry[50];
	int max_buffers=50;
	struct pbuf *pbuff;
	struct stats_mem **pProto=(struct stats_mem **)p;

	print_lwip_mem_information();

	for(j=0;j<MEMP_MAX;j++)
	{
		if(pProto[j] == NULL)
		{
			continue;
		}
		memset(&buff[0],0,sizeof(buff));
		len=0;

		snprintf(&buff[len],(TMP_BUF_LEN-len),"%s",pProto[j]->name);
		len+=15;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto[j]->err);
		len+=6;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto[j]->avail);
		len+=6;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto[j]->used);
		len+=6;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto[j]->max);
		len+=6;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto[j]->illegal);
		len+=6;
		snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",get_memp_buffersize((char *)pProto[j]->name)*pProto[j]->avail);

		for(i=0;i<len;i++)
		{
			if(buff[i] == 0)
			{
				buff[i]=' ';
			}
		}

		printf("%s\r\n",buff);
		vTaskDelay(10);
	}

	get_memp_buffers(MEMP_PBUF_POOL,&bufArry[0],&max_buffers);

	for(i=0;i<max_buffers;i++)
	{
		pbuff = (struct pbuf *)bufArry[i];
#if DEBUG_PBUF_ALLOCATION
		if(pbuff->func == NULL)
		{
			printf("ref count :%d buffer len:%d total len:%d flow:%x\r\n",pbuff->ref,pbuff->len,pbuff->tot_len,pbuff->flow);
		}
		else
		{
			printf("ref count :%d buffer len:%d total len:%d owner:%s flow:%x\r\n",pbuff->ref,pbuff->len,pbuff->tot_len,pbuff->func,pbuff->flow);
		}
#else
		printf("pointer:%p ref count :%d buffer len:%d total len:%d\r\n",pbuff,pbuff->ref,pbuff->len,pbuff->tot_len);
#endif

		vTaskDelay(10);
	}
#if USE_MEMP_DYNAMIC_ALLOCATION
	printf("number of buffer used:%d\r\n",get_number_of_memp_used());
#endif
	print_free_ram_mem();

#if DEBUG_PBUF_ASSIGNED
		print_pbuf_params();
#endif
	return 1;

}

int show_sys_counter(void *p,char *name)
{

	struct stats_sys *psys=(struct stats_sys *)p;

	printf("mbox.err:%d\r\n",psys->mbox.err);
	printf("mbox.max:%d\r\n",psys->mbox.max);
	printf("mbox.used:%d\r\n",psys->mbox.used);

	printf("mutex.err:%d\r\n",psys->mutex.err);
	printf("mutex.max:%d\r\n",psys->mutex.max);
	printf("mutex.used:%d\r\n",psys->mutex.used);

	printf("sem.err:%d\r\n",psys->sem.err);
	printf("sem.max:%d\r\n",psys->sem.max);
	printf("sem.used:%d\r\n",psys->sem.used);


	return 1;

}

int ppp_get_packet_statistics(char *buf);

int show_ppp_counter(void *p,char *name)
{
	char *buff=NULL;

	buff = (char *)malloc(1000);
	if(buff == NULL)
	{
		printf("failed to allocate buffer\r\n");
		return 1;
	}
	if(ppp_get_packet_statistics(buff) == 0)
	{
		printf("cant get ppp statistics\r\n");
		free(buff);
		return 1;
	}
	printf("%s\r\n",buff);
	free(buff);
	return 1;
}

#if LWIP_IPV6_DHCP6
char *dhcp6_state(int state)
{
	switch(state)
	{
	case DHCP6_STATE_OFF: return "OFF";
	case DHCP6_STATE_STATELESS_IDLE: return "IDLE";
	case DHCP6_STATE_REQUESTING_CONFIG: return "CONFIG";
	default:return "unknown state";
	}
}


int show_dhcp6_counter(void *p,char *name)
{
	struct netif *netif = netif_list;
	struct dhcp6 *dhcp6=NULL;
	while (netif != NULL)
	{
		dhcp6 = netif_dhcp6_data(netif);
		if (dhcp6 != NULL)
		{
			printf("dhcp6 xid:%d\r\n",dhcp6->xid);
			printf("dhcp6 pcb_allocated:%d\r\n",dhcp6->pcb_allocated);
			printf("dhcp6 state:%s\r\n",dhcp6_state(dhcp6->state));
			printf("dhcp6 tries:%d\r\n",dhcp6->tries);
			printf("dhcp6 request_config_pending:%d\r\n",dhcp6->request_config_pending);
			printf("dhcp6 request_timeout:%d\r\n",dhcp6->request_timeout);
			if(dhcp6->pClient_id != NULL)
			{
				printf("dhcp6 client id identifier:%d\r\n",dhcp6->pClient_id->header.indentifier);
				printf("dhcp6 client id msg len:%d\r\n",dhcp6->pClient_id->header.msg_len);
				switch(dhcp6->pClient_id->header.duid_type)
				{
				case LINK_LAYER_ADDRESS_PLUS_TIME:
					printf("dhcp6 client id type:%s\r\n","LINK_LAYER_ADDRESS_PLUS_TIME");
					printf("dhcp6 client id hardware type:%d\r\n",dhcp6->pClient_id->identifier.link_plus_time.duid_hardware_type);
					printf("dhcp6 client id time:%d\r\n",dhcp6->pClient_id->identifier.link_plus_time.duid_time);
					printf("dhcp6 client link-layer:%d.%d.%d.%d.%d.%d\r\n",
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[0],
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[1],
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[2],
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[3],
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[4],
							dhcp6->pClient_id->identifier.link_plus_time.link_layer_address[5]);

					break;
				case VENDOR_BASED_ON_ENTERPRISE_NUMBER:
					printf("dhcp6 client id type:%s\r\n","VENDOR_BASED_ON_ENTERPRISE_NUMBER");
					printf("dhcp6 client id enterprise number:%d\r\n",dhcp6->pClient_id->identifier.vendor_based_enterpise.enterprize_number);
					printf("dhcp6 client link-layer:%s\r\n",dhcp6->pClient_id->identifier.vendor_based_enterpise.identifier);
					break;
				case LINK_LAYER_ADDRESS_ONLY:
					printf("dhcp6 client id type:%s\r\n","LINK_LAYER_ADDRESS_ONLY");
					printf("dhcp6 client id hardware type:%d\r\n",dhcp6->pClient_id->identifier.link_addr_only.duid_hardware_type);
					printf("dhcp6 client link-layer:%d.%d.%d.%d.%d.%d\r\n",
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[0],
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[1],
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[2],
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[3],
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[4],
							dhcp6->pClient_id->identifier.link_addr_only.link_layer_address[5]);
					break;
				default:
					break;
				}
			}
			if(dhcp6->pServer_id != NULL)
			{
				printf("dhcp6 server id identifier:%d\r\n",dhcp6->pServer_id->header.indentifier);
				printf("dhcp6 server id msg len:%d\r\n",dhcp6->pServer_id->header.msg_len);
				switch(dhcp6->pServer_id->header.duid_type)
				{
				case LINK_LAYER_ADDRESS_PLUS_TIME:
					printf("dhcp6 server id type:%s\r\n","LINK_LAYER_ADDRESS_PLUS_TIME");
					printf("dhcp6 server id hardware type:%d\r\n",dhcp6->pServer_id->identifier.link_plus_time.duid_hardware_type);
					printf("dhcp6 server id time:%d\r\n",dhcp6->pServer_id->identifier.link_plus_time.duid_time);
					printf("dhcp6 server link-layer:%d.%d.%d.%d.%d.%d\r\n",
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[0],
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[1],
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[2],
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[3],
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[4],
							dhcp6->pServer_id->identifier.link_plus_time.link_layer_address[5]);

					break;
				case VENDOR_BASED_ON_ENTERPRISE_NUMBER:
					printf("dhcp6 server id type:%s\r\n","VENDOR_BASED_ON_ENTERPRISE_NUMBER");
					printf("dhcp6 server id enterprise number:%d\r\n",dhcp6->pServer_id->identifier.vendor_based_enterpise.enterprize_number);
					printf("dhcp6 server link-layer:%s\r\n",dhcp6->pServer_id->identifier.vendor_based_enterpise.identifier);
					break;
				case LINK_LAYER_ADDRESS_ONLY:
					printf("dhcp6 server id type:%s\r\n","LINK_LAYER_ADDRESS_ONLY");
					printf("dhcp6 server id hardware type:%d\r\n",dhcp6->pServer_id->identifier.link_addr_only.duid_hardware_type);
					printf("dhcp6 server link-layer:%d.%d.%d.%d.%d.%d\r\n",
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[0],
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[1],
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[2],
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[3],
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[4],
							dhcp6->pServer_id->identifier.link_addr_only.link_layer_address[5]);
					break;
				default:
					break;
				}
			}
#if LWIP_DHCP6_MAX_DNS_SERVERS
		  for(int i=0;i<LWIP_DHCP6_MAX_DNS_SERVERS;i++)
		  {
			  if(dhcp6->dns_addr_arr[i] != NULL)
			  {
				  if(dhcp6->dns_addr_arr[i]->type == IPADDR_TYPE_V6)
				  {
					printf("dns ip address:%s\r\n",ip6addr_ntoa((const ip6_addr_t *)&dhcp6->dns_addr_arr[i]->u_addr.ip6));
				  }
			  }
		  }
#endif
		}
		netif = netif->next;
	}
	return 1;
}
#endif

#if  LWIP_DHCP

char *dhcp_state(int state)
{
	switch(state)
	{
	case DHCP_STATE_OFF: return "OFF";
	case DHCP_STATE_REQUESTING: return "REQUESTING";
	case DHCP_STATE_INIT: return "INIT";
	case DHCP_STATE_REBOOTING: return "REBOOTING";
	case DHCP_STATE_REBINDING: return "REBINDING";
	case DHCP_STATE_RENEWING: return "RENEWING";
	case DHCP_STATE_SELECTING: return "SELECTING";
	case DHCP_STATE_INFORMING: return "INFORMING";
	case DHCP_STATE_CHECKING: return "CHECKING";
	case DHCP_STATE_PERMANENT: return "PERMANENT";
	case DHCP_STATE_BOUND:return "BOUND";
	case DHCP_STATE_RELEASING:return "RELEASING";
	case DHCP_STATE_BACKING_OFF:return "BACKING_OFF";
	default:return "unknown state";
	}
}
int show_dhcp_counter(void *p,char *name)
{
	struct netif *netif = netif_list;
	struct dhcp *dhcp;
	while (netif != NULL)
	{
		dhcp = netif_dhcp_data(netif);
		if (dhcp != NULL)
		{
			printf("dhcp stste:%s\r\n",dhcp_state(dhcp->state));
			printf("dhcp tries:%d\r\n",dhcp->tries);
			printf("subnet mask given:%d\r\n",dhcp->subnet_mask_given);
			printf("outgoing msg options length:%d\r\n",dhcp->options_out_len);
			printf("ticks with period %d milisecond for request timeout:%d\r\n",DHCP_FINE_TIMER_MSECS,dhcp->request_timeout);

			printf("ticks with period %d seconds for renewal time:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->t1_timeout);
			printf("ticks with period %d seconds for rebind time:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->t2_timeout);
			printf("ticks with period %d seconds until next renew try:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->t1_renew_time);
			printf("ticks with period %d seconds until next rebind try:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->t2_rebind_time);

			printf("ticks with period %d seconds since last received DHCP ack:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->lease_used);
			printf("ticks with period %d seconds for lease time:%d\r\n",DHCP_COARSE_TIMER_SECS,dhcp->t0_timeout);

			printf("server ip address :%lu.%lu.%lu.%lu\r\n",
					(dhcp->server_ip_addr.u_addr.ip4.addr & 0xFF000000) >> 24,
					(dhcp->server_ip_addr.u_addr.ip4.addr & 0x00FF0000) >> 16,
					(dhcp->server_ip_addr.u_addr.ip4.addr & 0x0000FF00) >> 8,
					(dhcp->server_ip_addr.u_addr.ip4.addr & 0x000000FF));

			printf("offer ip address :%lu.%lu.%lu.%lu\r\n",
					(dhcp->offered_ip_addr.addr & 0xFF000000) >> 24,
					(dhcp->offered_ip_addr.addr & 0x00FF0000) >> 16,
					(dhcp->offered_ip_addr.addr & 0x0000FF00) >> 8,
					(dhcp->offered_ip_addr.addr & 0x000000FF));

			printf("offer subnet address :%lu.%lu.%lu.%lu\r\n",
				(dhcp->offered_sn_mask.addr & 0xFF000000) >> 24,
				(dhcp->offered_sn_mask.addr & 0x00FF0000) >> 16,
				(dhcp->offered_sn_mask.addr & 0x0000FF00) >> 8,
				(dhcp->offered_sn_mask.addr & 0x000000FF));

			printf("offer gateway address :%lu.%lu.%lu.%lu\r\n",
				(dhcp->offered_gw_addr.addr & 0xFF000000) >> 24,
				(dhcp->offered_gw_addr.addr & 0x00FF0000) >> 16,
				(dhcp->offered_gw_addr.addr & 0x0000FF00) >> 8,
				(dhcp->offered_gw_addr.addr & 0x000000FF));

			printf("lease period (in seconds):%lu\r\n",dhcp->offered_t0_lease);
			printf("recommended renew time (usually 50 percents of lease period):%lu\r\n",dhcp->offered_t1_renew);
			printf("recommended rebind time (usually 87.5 of lease period):%lu\r\n",dhcp->offered_t2_rebind);

		}
		netif = netif->next;
	}
	return 1;
}
#endif
int show_specific_counter(void *p,char *name)
{
	int i=0;
	int len=0;
	char buff[TMP_BUF_LEN];
	struct stats_proto *pProto=(struct stats_proto *)p;

	memset(&buff[0],0,sizeof(buff));

	snprintf(&buff[len],(TMP_BUF_LEN-len),"%s",name);
	len+=5;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->xmit);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->recv);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->fw);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->drop);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->chkerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->lenerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->memerr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->rterr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->proterr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->opterr);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->err);
	len+=6;
	snprintf(&buff[len],(TMP_BUF_LEN-len),"|%d",pProto->cachehit);
	len+=6;

	for(i=0;i<len;i++)
	{
		if(buff[i] == 0)
		{
			buff[i]=' ';
		}
	}

	printf("%s\r\n",buff);
	return 1;

}
int show_all_counter(void *p,char *name)
{
	unsigned int j=0;

	for(j=1;j<sizeof(lwip_state)/sizeof(lwip_state[0]);j++)
	{
		lwip_state[j].cb_counter(lwip_state[j].counter,lwip_state[j].protocol_name);
	}

	return 1;
}

void print_timers_informarion(char *buff,uint32_t buffer_size);

int show_timer_counter(void *p,char *name)
{
	char buff[1000];
	print_timers_informarion(buff,1000);
	printf("%s\r\n",buff);

	return 1;
}

int clean_lwip_counter(void *p,char *name)
{
	unsigned int j=0;

	print_lwip_information();

	for(j=1;j<sizeof(lwip_state)/sizeof(lwip_state[0]);j++)
	{
		lwip_state[j].cb_counter(lwip_state[j].counter,lwip_state[j].protocol_name);
		memset(lwip_state[j].counter,0,sizeof(struct stats_proto));
	}
	return 0;
}

#define NETIF_FLAG_UP		   0x01U
/** If set, the netif has broadcast capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_BROADCAST	0x02U
/** If set, the interface has an active link
 *  (set by the network interface driver).
 * Either set by the netif driver in its init function (if the link
 * is up at that time) or at a later point once the link comes up
 * (if link detection is supported by the hardware). */
#define NETIF_FLAG_LINK_UP	  0x04U
/** If set, the netif is an ethernet device using ARP.
 * Set by the netif driver in its init function.
 * Used to check input packet types and use of DHCP. */
#define NETIF_FLAG_ETHARP	   0x08U
/** If set, the netif is an ethernet device. It might not use
 * ARP or TCP/IP if it is used for PPPoE only.
 */
#define NETIF_FLAG_ETHERNET	 0x10U
/** If set, the netif has IGMP capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_IGMP		 0x20U
/** If set, the netif has MLD6 capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_MLD6		 0x40U

int get_netflag_options(uint8_t flags,char  *buff)
{
	int len=0;

	if(flags & NETIF_FLAG_UP)
	{
		len += snprintf(&buff[len],(TMP_BUF_LEN-len),"UP,");
	}
	if(flags & NETIF_FLAG_BROADCAST)
	{
		len += snprintf(&buff[len],(TMP_BUF_LEN-len),"BROADCAST,");
	}
	if(flags & NETIF_FLAG_LINK_UP)
	{
		len += snprintf(&buff[len],(TMP_BUF_LEN-len),"RUNNING,");
	}
	if(flags & NETIF_FLAG_IGMP)
	{
		len += snprintf(&buff[len],(TMP_BUF_LEN-len),"MULTICAST,");
	}
	if(len)
	{
		buff[len-1]=0;
	}
	return len;
}
#if LWIP_CUSTOMER_SIDE_TRANS
int config_clatstruct clatconfig *pOpt)
{
	struct clat_config_params *clatParams=NULL;
	struct clat_ip_table *entry;
	if(pOpt->b_show_global_params == 1)
	{
		clatParams = get_clat_global_params();

		if(clatParams != NULL)
		{
			printf("clat enable:%s\r\n",clatParams->enable==1?"Enable":"Disable");
			if(clatParams->enable == 1)
			{
				printf("clat ipv6 interface:%s\r\n",clatParams->ipv6_interface);
				printf("clat external map:%x\r\n",clatParams->u32_ext_ip_map);
				printf("clat internal map:%x\r\n",clatParams->u32_clat_ip_map);
				printf("clat suffix:%x\r\n",clatParams->u32_clat_suffix);
				printf("clat ipv4 address:%x\r\n",clatParams->u32_ipv4_address);
				printf("plat address:%s\r\n",clatParams->plat_address);
				printf("plat address:%x:%x:%x:%x\r\n",
						clatParams->platv6_server.un.u32_addr[0],
						clatParams->platv6_server.un.u32_addr[1],
						clatParams->platv6_server.un.u32_addr[2],
						clatParams->platv6_server.un.u32_addr[3]);


				entry = get_first_clat_table();
				while(entry)
				{
					printf("interface name:%c%c\r\n",entry->interface_name[0],entry->interface_name[1]);
					printf("ipv4 map:%x\r\n",(unsigned int)entry->ipv4Map);
					printf("ipv4 source ip:%x\r\n",(unsigned int)entry->srcv4);
					printf("ipv6 source ip:%x:%x:%x:%x\r\n",(unsigned int)entry->srcv6.addr[0],
															(unsigned int)entry->srcv6.addr[1],
															(unsigned int)entry->srcv6.addr[2],
															(unsigned int)entry->srcv6.addr[3]);
					entry = get_next_clat_table(entry);
				}
			}
		}
	}
	return 1;
}

#endif
int show_lwip_ifconfig(struct lwipIfconfig *pOpt)
{
	struct netif *netif;
	char  *buff=NULL;
	char  flag_buff[TMP_BUF_LEN];
	int	  len=0;
	int i=0;

	netif = netif_list;

	if(netif == NULL)
	{
		printf("net link list is empty\r\n");
		return 0;
	}

	buff = malloc(IFCONFIG_BUF_LEN);

	if(buff == NULL)
	{
		printf("failed to allocate memory\r\n");
		return 0;
	}

	while(netif != NULL)
	{
		//printf("check next net\r\n");
		if(pOpt->b_interface_name == 1)
		{
			//printf("check specific interface\r\n");
			if( (pOpt->name[0] != netif->name[0]) || (pOpt->name[1] != netif->name[1]) )
			{
				netif = netif->next;
				continue;
			}
		}
		len=0;
		if (netif_is_up(netif))
		{
			get_netflag_options(netif->flags,flag_buff);
			if(1)
			{
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"%c%c:  flags=%d<%s> mtu:%d ",netif->name[0], netif->name[1],netif->flags,flag_buff,netif->mtu);
#ifdef SUPPORT_IPV6_MTU
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"ipv6 mtu:%d",netif->ipv6_mtu);
#endif
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"\r\n");
			}
			len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		inet: %d.%d.%d.%d netmask %d.%d.%d.%d GW %d.%d.%d.%d\r\n",
						   ip4_addr1(netif_ip4_addr(netif)),
						   ip4_addr2(netif_ip4_addr(netif)),
						   ip4_addr3(netif_ip4_addr(netif)),
						   ip4_addr4(netif_ip4_addr(netif)),
						   ip4_addr1(netif_ip4_netmask(netif)),
						   ip4_addr2(netif_ip4_netmask(netif)),
						   ip4_addr3(netif_ip4_netmask(netif)),
						   ip4_addr4(netif_ip4_netmask(netif)),
						   ip4_addr1(netif_ip4_gw(netif)),
						   ip4_addr2(netif_ip4_gw(netif)),
						   ip4_addr3(netif_ip4_gw(netif)),
						   ip4_addr4(netif_ip4_gw(netif)) );
#if LWIP_IPV6
			i=0;
			if( (i < LWIP_IPV6_NUM_ADDRESSES) && (!ip6_addr_isinvalid(netif_ip6_addr_state(netif, i))) )
			{
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		inet6 %s <local-link>\r\n",ip6addr_ntoa(netif_ip6_addr(netif, i)));
			}
			i++;
			if( (i < LWIP_IPV6_NUM_ADDRESSES) && (!ip6_addr_isinvalid(netif_ip6_addr_state(netif, i))) )
			{
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		inet6 %s <global-ip>\r\n",ip6addr_ntoa(netif_ip6_addr(netif, i)));
			}
			i++;
			if( (i < LWIP_IPV6_NUM_ADDRESSES) && (!ip6_addr_isinvalid(netif_ip6_addr_state(netif, i))) )
			{
				len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		inet6 %s <global-ip>\r\n",ip6addr_ntoa(netif_ip6_addr(netif, i)));
			}
#endif
			if(netif->flags & NETIF_FLAG_ETHARP)
			{
						len += sprintf(&buff[len],"		ether %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x   (Ethernet)\r\n",
												netif->hwaddr[0],netif->hwaddr[1],netif->hwaddr[2],
												netif->hwaddr[3],netif->hwaddr[4],netif->hwaddr[5]);
			}
#if LWIP_SNMP
			len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		RX ucastPcts %ld  uncastPcts %ld bytes %ld\r\n",netif->mib2_counters.ifinucastpkts,netif->mib2_counters.ifinnucastpkts,netif->mib2_counters.ifinoctets);
			len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		RX errors %ld  dropped %ld unkProro %ld\r\n",netif->mib2_counters.ifinerrors,netif->mib2_counters.ifindiscards,netif->mib2_counters.ifinunknownprotos);
			len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		TX ucastPcts %ld  uncastPcts %ld bytes %ld\r\n",netif->mib2_counters.ifoutucastpkts,netif->mib2_counters.ifoutnucastpkts,netif->mib2_counters.ifoutoctets);
			len += snprintf(&buff[len],(IFCONFIG_BUF_LEN-len),"		TX errors %ld  dropped %ld\r\n",netif->mib2_counters.ifouterrors,netif->mib2_counters.ifoutdiscards);
#endif
		}
		if(pOpt->b_clean_counter == 1)
		{
#if LWIP_SNMP
			memset(&netif->mib2_counters,0,sizeof(struct stats_mib2_netif_ctrs));
#endif
		}
		if(len) {
			printf("%s\r\n",buff);
		}

		netif = netif->next;
	}
	free(buff);
	return 1;
}

int get_lwipifconfig_args(int argc, char **argv, struct lwipIfconfig *pOpt)
{
	int i=0;

	memset(pOpt,0,sizeof(struct lwipIfconfig));

	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;

		if( (strcmp(argv[i],"-i") == 0) && (i+1 < argc) )
		{
			strncpy(pOpt->name,argv[i+1],D_LWIP_INTERFACE_NAME_SIZE);
			pOpt->name[D_LWIP_INTERFACE_NAME_SIZE-1] = 0;
			pOpt->b_interface_name=1;
			i++;
		}
		if( (strcmp(argv[i],"-c") == 0))
		{
			pOpt->b_clean_counter=1;
		}
#if 0
		if( (strcmp(argv[i],"-p") == 0) && (i+1 < argc) )
		{
			ip_addr_t ip_addr;
			ip_addr_t netmask;
			int ret = get_pdn_ipv4_address(argv[++i],&ip_addr,&netmask);
			if(ret == ERR_OK)
			{
				printf("ip address:%x netmask:%x\r\n",ip_addr.u_addr.ip4.addr,netmask.u_addr.ip4.addr);
			}
			else if(ret == ERR_INPROGRESS)
			{
				printf("interface name :%s in progress\r\n",argv[i]);
			}
			else
			{
				printf("failed to get interface name :%s\r\n",argv[i]);
			}
		}
#endif
	}
	return 1;
}

int get_lwipstat_args(int argc, char **argv, struct lwipStat *pOpt)
{
	int i=0;
	unsigned int j=0;

	memset(pOpt,0,sizeof(struct lwipStat));



	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;

		for(j=0;j<sizeof(lwip_state)/sizeof(lwip_state[0]);j++)
		{
			if(strcmp(lwip_state[j].abbr,argv[i]) == 0)
			{
				pOpt->options[0] |= lwip_state[j].bitmak;
				break;
			}
		}
		for(j=0;j<sizeof(lwip_special_state)/sizeof(lwip_special_state[0]);j++)
		{
			if(strcmp(lwip_special_state[j].abbr,argv[i]) == 0)
			{
				pOpt->options[1] |= lwip_special_state[j].bitmak;
				break;
			}
		}
	}

	if( (pOpt->options[0] == 0) && (pOpt->options[1] == 0) )
		return 0;

	return 1;
}
int lwipifconfig_usage()
{
	printf ("usage: ifconfig < -i interface>\r\n");
	return 0;
}

int lwipstat_usage()
{
	printf ("usage: lwip-stt <-a|-l|-e|-f|-ip4|-ip6|-icmp4|-icmp6|-udp4|-tcp4|-nd6|-igmp|-mem|-heap|-mld|-c|> [options]\r\n");
	printf ("\r\n");
	printf ("Options:\r\n");
	printf ("	-a				 - show all counter information\r\n");
#if LINK_STATS
	printf ("	-l				 - show all link counters\r\n");
#endif
#if ETHARP_STATS
	printf ("	-e				 - show all Ethernet arp\r\n");
#endif
#if IPFRAG_STATS
	printf ("	-f				 - show all fragments\r\n");
#endif
#if IP_STATS
	printf ("	-ip4			   - show all ip version 4 counters\r\n");
#endif
#if IP6_STATS
	printf ("	-ip6			   - show all ip version 6 counters\r\n");
#endif
#if ICMP_STATS
	printf ("	-icmp4			 - show all icmp version 4\r\n");
#endif
#if ICMP6_STATS
	printf ("	-icmp6			 - show all icmp version 6\r\n");
#endif
#if TCP_STATS
	printf ("	-tcp			 - show all tcp version 4\r\n");
#endif
#if UDP_STATS
	printf ("	-udp			 - show all udp version 4\r\n");
#endif
#if ND6_STATS
	printf ("	-nd6			 - show all neighbor discovery\r\n");
#endif
#if IGMP_STATS
	printf ("	-igmp			 - show all igmp counters\r\n");
#endif
#if MEM_STATS
	printf ("	-mem			 - show lwip memory usage\r\n");
#endif
#if MEM_STATS
	printf ("	-heap			 - show lwip heap usage\r\n");
#endif
#if MLD6_STATS
	printf ("	-mld			 - show lwip multicast listener usage\r\n");
#endif
	printf ("	-timer		   - show timer information\r\n");
	printf ("	-c			 - clean counter\r\n");
	printf ("	-sys			 - show queue system\r\n");
	printf ("	-ppp			 - show ppp packet statistics\r\n");
#if  LWIP_DHCP
	printf ("	-dhcp			- show dhcp parameters\r\n");
#endif
#if  LWIP_IPV6_DHCP6
	printf ("	-dhcp6			- show dhcp version 6 parameters\r\n");
#endif

	return 0;
}

int print_lwip_table()
{
	char stat_msgs_proto[] ="Name |Tx   |Rx   |Fwd  |drop |Cksum|Len  |Mem  |Route|Proto|Opt  |Misc |cache|";
	char stat_msgs_space[] ="=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|=====|";

	printf("%s\r\n",stat_msgs_proto);
	printf("%s\r\n",stat_msgs_space);

	return 0;
}

int print_lwip_information(void)
{
	int i=0;
	char *stat_msgs_explain[12] = {
	  "Tx-transmitted",
	  "Rx-received",
	  "Fwd-forwarded",
	  "drop-dropped",
	  "Cksum-checksum errors",
	  "Len-length errors",
	  "Mem-memory errors",
	  "Route-routing errors",
	  "Proto-protocol errors",
	  "Opt-option errors",
	  "Misc-misc errors",
	  "cache-cache hits"
	};

	for(i=0;i<12;i++)
	{
		printf("%s\r\n",stat_msgs_explain[i]);
	}
	return 0;
}

int show_lwip_special_counters(struct lwipStat *opt)
{
	unsigned int j=0;

	for(j=0;j<sizeof(lwip_special_state)/sizeof(lwip_special_state[0]);j++)
	{
		if(opt->options[1] & lwip_special_state[j].bitmak)
		{
			lwip_special_state[j].cb_counter(lwip_special_state[j].counter,lwip_special_state[j].protocol_name);
		}
	}
	return 1;
}

int show_lwip_counters(struct lwipStat *opt)
{
	unsigned int j=0;
	int found=0;

	print_lwip_table();

	for(j=0;j<sizeof(lwip_state)/sizeof(lwip_state[0]);j++)
	{
		if(opt->options[0] & lwip_state[j].bitmak)
		{
			lwip_state[j].cb_counter(lwip_state[j].counter,lwip_state[j].protocol_name);
			found++;
		}
	}
	if(found > 0)
		print_lwip_information();
	return 1;
}
#if LWIP_CUSTOMER_SIDE_TRANS
int clatconfig_parser(int argc, char **argv)
{
	struct clatconfig opt;

	if(argc >= 1)
	{
		if (get_clat_args (argc, argv, &opt) == 0)
			return clat_usage ();

		config_clat(&opt);
	}
	return 0;
}
#endif

int config_pathmtu(struct pathmtuconfig *pOpt)
{
	int num=0;
	if(pOpt->b_show_params == 1)
	{
		char *entry=(char *)malloc(1000);
		if(entry == NULL)
		{
			printf ("failed to allocate memory\r\n");
			return 0;
		}
		num = nd6_print_all_destination_cache_entry(entry,1000);
		if(num)
		{
			printf("%s\r\n",entry);

		}
		free(entry);
	}
	else if(pOpt->b_create_ipv6_params == 1)
	{
		if(nd6_create_destination_cache_entry(&pOpt->entry) == 1)
		{
			printf("new entry created\r\n");
		}
		else
		{
			printf("entry already exist\r\n");
		}
	}
	else if(pOpt->b_delete_ipv6_params == 1)
	{
		if(nd6_delete_destination_cache_entry(&pOpt->entry) == 1)
		{
			printf("entry deleted\r\n");
		}
		else
		{
			printf("entry not found\r\n");
		}
	}
	return 0;
}

int pathmtu_usage()
{
	printf ("usage: pathmtu -show -show valid path mtu\r\n");
	printf ("usage: pathmtu -ipv6 <crt/dlt> #ipv6 [#mtu] -create/delete entry path mtu ipv6\r\n");
	return 0;
}

int get_pathmtu_args(int argc, char **argv, struct pathmtuconfig *pOpt)
{
	int i=0;
	int found=0;

	memset(pOpt,0,sizeof(struct pathmtuconfig));

	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;

		if(strcmp(argv[i],"-show") == 0)
		{
			pOpt->b_show_params=1;
			found=1;
		}
		if( (strcmp(argv[i],"-ipv6") == 0) && (i+3 < argc) )
		{
			i++;
			if(strncmp(argv[i],"crt",3) == 0)
			{
				pOpt->b_create_ipv6_params=1;
			}
			else if(strncmp(argv[i],"dlt",3) == 0)
			{
				pOpt->b_delete_ipv6_params=1;
			}
			else
			{
				printf ("first parameter must crt/dlt\r\n");
				return 0;
			}

			if(inet_pton(AF_INET6, argv[++i], &pOpt->entry.destination_addr) != 1)
			{
				printf ("is not ipv6 address\r\n");
				return 0;
			}
			if(pOpt->b_create_ipv6_params == 1)
			{
				pOpt->entry.pmtu=atoi(argv[++i]);
				if(pOpt->entry.pmtu < 1200)
				{
					printf ("max ipv6 MTU size cannot be less than 1200 bytes:%d\r\n",pOpt->entry.pmtu);
					return 0;
				}
			}
			else
			{
				pOpt->entry.pmtu=1200;
			}
			printf ("ipv6:%s mtu:%d\r\n",ip6addr_ntoa(&pOpt->entry.destination_addr),pOpt->entry.pmtu);
			found=1;
		}
	}
	return found;
}
int pathmtuconfig_parser(int argc, char **argv)
{
	struct pathmtuconfig opt;

	memset(&opt,0,sizeof(struct pathmtuconfig));
	if(argc >= 1)
	{
		if (get_pathmtu_args (argc, argv, &opt) == 0)
			return pathmtu_usage ();

		config_pathmtu(&opt);
	}
	return 0;
}

int lwipifconfig_parser(int argc, char **argv)
{
	struct lwipIfconfig opt;

	if(argc >= 1)
	{
		if (get_lwipifconfig_args (argc, argv, &opt) == 0)
			return lwipifconfig_usage ();

		show_lwip_ifconfig(&opt);
	}
	return 0;
}

int show_ifconfig()
{
	struct lwipIfconfig opt;

	memset(&opt,0,sizeof(struct lwipIfconfig));

	show_lwip_ifconfig(&opt);

	return 0;
}

int lwipstat_parser (int argc, char **argv)
{
	struct lwipStat opt;

	if (argc <= 1 || get_lwipstat_args (argc, argv, &opt) == 0)
		return lwipstat_usage ();


	if(opt.options[0]  != 0)
	{
		show_lwip_counters(&opt);
	}
	if(opt.options[1]  != 0)
	{
		show_lwip_special_counters(&opt);
	}
	return 0;
}

int lwipstat_show_all ()
{
	struct lwipStat opt;
	unsigned int j=0;

	memset(&opt,0,sizeof(struct lwipStat));

	for(j=0;j<sizeof(lwip_state)/sizeof(lwip_state[0]);j++)
	{
		opt.options[0] |= lwip_state[j].bitmak;
	}
	for(j=0;j<sizeof(lwip_special_state)/sizeof(lwip_special_state[0]);j++)
	{
		opt.options[1] |= lwip_special_state[j].bitmak;
	}

	show_lwip_counters(&opt);
	show_lwip_special_counters(&opt);
	return  0;
}

static void dummy_tmr_func(void *arg)
{
	TickType_t call_tick=(TickType_t)arg;

	printf("callback func call after timetick :%lu\r\n",xTaskGetTickCount( )-call_tick);

}

int lwiptimer_cmd( int argc,void *argv[] )
{
	int i=0;
	int number_of_timer;
	unsigned int timeout;
	int j=0;
	TickType_t tick=0;

	if(argc > 1)
	{
		for (i=1; i<argc; i++)
		{
			if (argv[i] == NULL)
				continue;
#if 0
			if(strcmp(argv[i],"-r") == 0)
			{
				send_tcpip_reschedule();
				printf ("Reschedule LWIP Done!!!\r\n");
			}
#endif
			if(strcmp(argv[i],"-a") == 0)
			{
				printf ("number of tcpip_message waiting:%lu\r\n",get_mbox_tcpip_msg_waiting());
				printf ("message message received:%d\r\n",get_queue_message_rcv_counter());
				if(islinkListEmpty() == 1)
				{
					printf ("linklist is empty\r\n");
				}
				else
				{
					printf ("number of linklist waiting:%d\r\n",nunOfEntriesInlinkList());
				}
			}
			if(strcmp(argv[i],"-b") == 0)
			{
				get_mbox_tcpip_msg_value();
				printf ("done\r\n");
			}
			if( (strcmp(argv[i],"-t") == 0)&& (i+2 < argc) )
			{
				number_of_timer=atoi(argv[++i]);
				//timeout=atoi(argv[++i]);
				sscanf(argv[++i], "%u", &timeout);  // Parse the String into the Number
				//sys_untimeout(dummy_tmr_func,NULL);
				tick = xTaskGetTickCount( );
				for(j=0;j<number_of_timer;j++)
				{
					sys_timeout(timeout,dummy_tmr_func,(void *)tick);
				}

			}
		}
	}
  return 0;
}

int lwiplogger_cmd(int argc,void *argv[] )
{
	int i=0;
	int feature=0;
#if 0
	printf ("lwiplog -d			 -> display lwip modules\r\n");
	printf ("lwiplog -s #moduleid   -> set lwiplog module id\r\n");
	printf ("lwiplog -c #moduleid   -> clear lwiplog module id\r\n");
	printf ("lwiplog -a			 -> clear all lwiplog module id\r\n");
	printf ("lwiplog -b			 -> set all lwiplog module id\r\n");
#endif
	printf ("lwiplog -set-lvl <critical|error|warning|info> -> set log level\r\n");
	printf ("lwiplog -clr-lvl	   -> clear log level\r\n");

	if(argc > 1)
	{
		for (i=1; i<argc; i++)
		{
			if (argv[i] == NULL)
				continue;
#if 0
			if(strcmp(argv[i],"-d") == 0) // display all log features
			{
				sys_debug_diplay_all_log();
			}
			if( (strcmp(argv[i],"-s") == 0) && (i+1 < argc) )// set log features
			{
				i++;
				feature = atoi(argv[i]);
				set_lwip_debug_log(feature);
			}
#endif
			if( (strcmp(argv[i],"-set-lvl") == 0) && (i+1 < argc) )// set log features
			{
				i++;
				if(strcmp(argv[i],"critical") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					set_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"error") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					set_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"warning") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					set_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"info") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					set_lwip_level_log(feature);
				}
				else
				{
					printf ("unknown log level:%s\r\n",(char *)argv[i]);
				}

			}
			if( (strcmp(argv[i],"-clr-lvl") == 0) && (i+1 < argc) )// set log features
			{
				i++;
				if(strcmp(argv[i],"critical") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					clear_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"error") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					clear_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"warning") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					clear_lwip_level_log(feature);
				}
				else if(strcmp(argv[i],"info") == 0)
				{
					feature = LWIP_DBG_LEVEL_SEVERE;
					clear_lwip_level_log(feature);
				}
				else
				{
					printf ("unknown log level:%s\r\n",(char *)argv[i]);
				}
			}
#if 0
			if( (strcmp(argv[i],"-c") == 0) && (i+1 < argc) )// clear log features
			{
				i++;
				feature = atoi(argv[i]);
				clear_lwip_debug_log(feature);
			}
			if(strcmp(argv[i],"-a") == 0) // clean log features
			{
				clear_all_lwip_debug_log();
			}
			if(strcmp(argv[i],"-b") == 0) // set all log features
			{
				set_all_lwip_debug_log();
			}
#endif
		}
	}
  return 0;
}

#define		NETBUF_BUFFER_SIZE_PRINT		100

int lwipnetbuf_cmd(int argc,void *argv[])
{
	int i=0;
	int j=0;
	struct memp *bufArry[10];
	int max_buffers=10;
	struct netbuf *buf = NULL;
	int found=0;

	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;


		if(strcmp(argv[i],"-show") == 0)
		{
			found=1;
			get_memp_buffers(MEMP_NETBUF,&bufArry[0],&max_buffers);
			for(j=0;j<max_buffers;j++)
			{
				buf = (struct netbuf *)bufArry[j];
				if(buf->addr.type == IPADDR_TYPE_V4)
				{
					printf("%lu.%lu.%lu.%lu:%d",
								   ( (buf->addr.u_addr.ip4.addr & 0xFF000000) >> 24),
								   ( (buf->addr.u_addr.ip4.addr & 0x00FF0000) >> 16),
								   ( (buf->addr.u_addr.ip4.addr & 0x0000FF00) >> 8),
								   ( (buf->addr.u_addr.ip4.addr & 0x000000FF) ),
								   buf->port);
				}
				else if(buf->addr.type == IPADDR_TYPE_V6)
				{
					printf("%s:%d",ip6addr_ntoa(&buf->addr.u_addr.ip6),buf->port);
				}
				else
				{
					continue;
				}
			}
		}
	}
	if(found == 0)
	{
		printf ("usage: lwipnetbuf -show\r\n");
	}
	return 0;
}

#define		IPV6_BUFFER_SIZE_PRINT		1000

int lwipnd6_cmd(int argc,void *argv[])
{
	int cli_flag=0;
	int i=0;
	char *buffer=NULL;
	int len=0;

	buffer = malloc(IPV6_BUFFER_SIZE_PRINT);
	if(buffer == NULL)
	{
		printf ("failed to allocate buffer\r\n");
		return pdFALSE;
	}

	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;

		if(strcmp(argv[i],"-stt") == 0)
		{
			len = nd6_route_print(buffer,IPV6_BUFFER_SIZE_PRINT);
			if(len != 0)
			{
				printf ("%s\r\n",buffer);
			}
			cli_flag++;
		}
		if(strcmp(argv[i],"-pmu") == 0)
		{
			len = nd6_path_mtu_discovery_print(buffer,IPV6_BUFFER_SIZE_PRINT);
			if(len != 0)
			{
				printf ("%s\r\n",buffer);
			}
			cli_flag++;
		}

	}

	if(cli_flag == 0)
	{
		printf ("usage: lwipnd6 -stt\r\n");
		printf ("usage: lwipnd6 -pmu\r\n");
	}
	free(buffer);
	return 0;
}

void* lwiperf_server_g =NULL;
void* lwiperf_udp_server_g =NULL;

void lwiperf_report(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(local_addr);
  LWIP_UNUSED_ARG(local_port);

  printf("IPERF report: type=%d, remote: %s:%d, total bytes: %lu, duration in ms: %lu, kbits/s: %lu\r\n",
	(int)report_type, ipaddr_ntoa(remote_addr), (int)remote_port, bytes_transferred, ms_duration, bandwidth_kbitpsec);
}


int lwiperf_cmd (int argc, char *argv[]){
	int found=0;
	int port=LWIPERF_TCP_PORT_DEFAULT;
	int i=0;

	for (i=1; i<argc; i++)
	{
		if (argv[i] == NULL)
			continue;

		if( (strcmp(argv[i], "-p")==0) && (i+1 < argc) )
		{
			port = atoi(argv[i+1]);
			i++;
		}
		if(strcmp(argv[i], "-s")==0)
		{
			 if (lwiperf_server_g!=NULL)
			 {
				printf("lwiperf is already running , ignored\r\n");
			 }
			 else
			 {
				 printf("staring server , listening to port %d\r\n",port);
				 lwiperf_server_g=lwiperf_start_tcp_server(IP_ADDR_ANY, port,
						 lwiperf_report, NULL);
				 if(lwiperf_server_g == NULL)
				 {
					 printf("failed to start tcp server\r\n");
				 }
			 }
			 found=1;
		}
		if(strcmp(argv[i], "-u")==0)
		{
			 if (lwiperf_udp_server_g!=NULL)
			 {
				printf("lwiperf is already running , ignored\r\n");
			 }
			 else
			 {
				 printf("staring udp server , listning to port %d\r\n",port);
				 lwiperf_udp_server_g=lwiperf_start_udp_server(IP_ADDR_ANY, port,
						 lwiperf_report, NULL);
			 }
			 found=1;
		}
		if(strcmp(argv[i], "-k")==0)
		{
			 if (lwiperf_server_g != NULL)
			 {
				 printf("killing lwiperf tcp server\r\n");
				 lwiperf_abort(lwiperf_server_g);
				 lwiperf_server_g = NULL;
			 }
			 if(lwiperf_udp_server_g != NULL)
			 {
				 printf("killing lwiperf udp server\r\n");
				 lwiperf_udp_abort(lwiperf_udp_server_g);
				 lwiperf_udp_server_g = NULL;
			 }
			 found=1;
		}
	}
	if(found == 0)
	{
		printf("lwiperf  -s (tcp iperf) or -u (udp iperf) -p (port id) starts iperf server  -k  kill lwiperf server\r\n");
		printf("example: lwiperf -p 5002 -s --> create lwip tcp server on port 5002\r\n");
	}

	return 0;
}


