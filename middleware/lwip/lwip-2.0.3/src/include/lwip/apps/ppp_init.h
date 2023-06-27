#ifndef LWIP_PPP_INIT_H
#define LWIP_PPP_INIT_H
#include "lwip/sio.h"
#include "pppos_netif.h"

//int initPppAppWithLogicalSerialPort(int app_instance_num,logical_serial_id_t logical_serial_id);
int initPppApp(pppos_arg_t *pppos_arg_ptr);
void printPPPState(void);
struct netif *get_ppp_netif(void);

#endif /* LWIP_PPP_INIT_H */
