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
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sio.h"
#include "pppos_netif.h"

//#include "netif/ppp/pppe2e.h"
//#include "newlibPort.h"
//#include "osal/osal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "emux.h"

static char pppFlag=0x7e;
int ppp_dropCnt=0;

#define DEF_ESCAPE_STR "+++"
#define DEF_TIME_BEFORE_ESCAPE 1000
#define DEF_TIME_AFTER_ESCAPE 1000

//static char escapeStr[5];

static u16_t hdlcBuffLen=0;
static uint8_t hdlcBuff[2048];

typedef enum
{
    E_ESC_WAIT_FOR_CAHR,
    E_ESC_SEQ_RECEIVED
}eEscState;

typedef struct
{
    char         escapeStr[10];
    int            escape_len;
    int            current_place;
    unsigned int            timeBeforeEsc;
    unsigned int            timeAfterEsc;
    TickType_t    lastTimeTick;
    eEscState    state;
    TimerHandle_t xTimer;
}tEscSeqDb;

static tEscSeqDb *pScapeSeq;


/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

#define MAX_DEVICE 4 /*currently [0] in enough ??? */

#include "lwip/sio.h"



static sendToPPP_fn sendToPPP_CB= NULL;



static int print_ppp_message_flag=0; // Change to 0 - 1 is only for debug

void initEscSeq(void);
int timer_escape_squence(TickType_t time);
void ppp_release_sem(int sleep);

void bindRxUartToPPP(sendToPPP_fn send_to_PPP_fn)
{
    sendToPPP_CB = send_to_PPP_fn;
}

void set_print_ppp_message_flag(int ppp_flag)
{
    print_ppp_message_flag = ppp_flag;
}
void print_ppp_message(uint8_t *buff,int len,const char *funcName)
{
    int i=0;

    if(print_ppp_message_flag == 0)
    {
        return;
    }

    printf("function name:%s\r\n",funcName);
    for(i=0;i<len;i++)
    {
        if( (i%16) == 0)
        {
            printf("\r\n");
        }
        printf("0x%2.2x ",buff[i]);
    }
    printf("\r\n");

}

static void sendToPpp(uint8_t recvChar,void *appCookie)
{
    TickType_t now;

    if(pScapeSeq == NULL)
    {
        printf("escape sequence is NULL\r\n");
        return;
    }


    if( (pScapeSeq->escape_len > pScapeSeq->current_place) && (pScapeSeq->escapeStr[pScapeSeq->current_place] == recvChar) )
    {
//        printf("got:%c\r\n",recvChar); // Debug print - remove later
        pScapeSeq->current_place++;
        if(pScapeSeq->current_place >= pScapeSeq->escape_len)
        {
            printf("got all chars\r\n");
            now = xTaskGetTickCount();
            if( (now - pScapeSeq->lastTimeTick) > pScapeSeq->timeBeforeEsc)
            {
                // found string,  wait no char during timeout
                pScapeSeq->state = E_ESC_SEQ_RECEIVED;
                printf("call to timer:%d\r\n",pScapeSeq->timeAfterEsc);
                timer_escape_squence(pScapeSeq->timeAfterEsc);
            }
            else
            {
                initEscSeq();
            }
        }
    }
    else
    {
        initEscSeq();
    }


    if (hdlcBuffLen < 2048)
    {
        hdlcBuff[hdlcBuffLen]=recvChar;
    }
    else
    {
        printf("serial Hdcl bufffer fully occupy!!!\n");
        hdlcBuffLen=0;
        return;
    }

#if 0
    /*Search for switch to AT cmd channel back */
    if ((hdlcBuffLen >= pScapeSeq->escape_len) &&
      (!memcmp(&hdlcBuff[hdlcBuffLen-pScapeSeq->escape_len],pScapeSeq->escapeStr,pScapeSeq->escape_len))){
          printf("Found %s that trigger moving back from PPP to AT client \n",pScapeSeq->escapeStr);
          hdlcBuffLen=0;//For next iteration
          serialMngrToggleApi(serialChNum);
                  ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
          return;
    }
#endif
    if (recvChar == pppFlag) { /* Got 0x7e flag */
        if (hdlcBuffLen>1){
           if (sendToPPP_CB != NULL){
               //printf("hdlcBuffLen: %d\r\n", hdlcBuffLen);
               print_ppp_message(hdlcBuff,hdlcBuffLen,"sendToPpp");
              sendToPPP_CB(hdlcBuff, hdlcBuffLen+1);
           }
        }
        hdlcBuffLen=0;
    }
    else
    {
        if (hdlcBuffLen < 2048)
           hdlcBuffLen++;
    }
}

static void escTimerExpiredCallback( TimerHandle_t pxTimer )
{
    if(pScapeSeq == NULL)
    {
        printf("escape sequence is NULL\r\n");
        return;
    }
    printf("timer called:%d\n",pScapeSeq->state);
    if(pScapeSeq->state == E_ESC_SEQ_RECEIVED)
    {
          printf("Found %s that trigger moving back from PPP to AT client \n",pScapeSeq->escapeStr);
          hdlcBuffLen=0;//For next iteration
//          serialMngrToggleApi(serialChNum);
          //serialSwitchIsPortToggledApi(0,LOGICAL_PORT_TO_PPP_APP_SWITCH_PORT1);
//          ppp_end2end_set_state(PPP_AT_SWITCH_SESSION,0,0);
          ppp_release_sem(0);
          initEscSeq();
    }
}

int timer_escape_squence(TickType_t time)
{
    if(pScapeSeq == NULL)
    {
        printf("escape sequence is NULL\r\n");
        return 1;
    }
    if(pScapeSeq->xTimer == NULL)
    {

        pScapeSeq->xTimer = xTimerCreate(     "esc_Timer",
                                    time,
                                    pdFALSE,
                                    NULL,
                                    escTimerExpiredCallback
                                );

        if( pScapeSeq->xTimer == NULL )
        {
            printf("failed to create timer\r\n");
            return 0;
        }
    }
    if( xTimerStart( pScapeSeq->xTimer, 0 ) != pdPASS )
    {
        printf("failed to start time\r\n");
        return 0;
    }
    printf("timer stated\r\n");
    return 1;
}

void initEscSeq(void)
{
    if(pScapeSeq == NULL)
    {
        printf("escape sequence is NULL\r\n");
        return;
    }
    pScapeSeq->current_place=0;
    pScapeSeq->lastTimeTick = xTaskGetTickCount();
    pScapeSeq->state = E_ESC_WAIT_FOR_CAHR;

    if(pScapeSeq->xTimer != NULL)
    {
        if (xTimerIsTimerActive(pScapeSeq->xTimer) != pdFALSE)
        {
            /* xTimer is active, do something. */
            if (xTimerStop(pScapeSeq->xTimer, 0) != pdPASS)
            {
                printf("failed to stop timer\r\n");
            }
        }
        configASSERT(xTimerDelete(pScapeSeq->xTimer, 0) == pdPASS);
        pScapeSeq->xTimer = NULL;
    }
//    printf("init timer\r\n");
}

void setPPPEscapeString(char *str,int len,int timeBeforeEscSeq,int timeAfterEscSeq)
{
    if(pScapeSeq == NULL)
    {
        printf("escape sequence is NULL\r\n");
        return;
    }
    memcpy(pScapeSeq->escapeStr,str,len);
    pScapeSeq->escape_len=len;
}

static void setPPPEscapeString_sio(char *str,int len,int timeBeforeEscSeq,int timeAfterEscSeq)
{
    if(pScapeSeq == NULL)
    {
        pScapeSeq = malloc(sizeof(tEscSeqDb));
        if(pScapeSeq == NULL)
        {
            printf("failed to allocate memory for escape sequence\r\n");
            return;
        }
        memset(pScapeSeq,0,sizeof(tEscSeqDb));
        memcpy(pScapeSeq->escapeStr,str,len);
        pScapeSeq->escape_len=len;
    }
    pScapeSeq->current_place=0;
    pScapeSeq->timeBeforeEsc = timeBeforeEscSeq;
    pScapeSeq->timeAfterEsc = timeAfterEscSeq;
    pScapeSeq->state = E_ESC_WAIT_FOR_CAHR;
    pScapeSeq->xTimer = NULL;
    initEscSeq();
}

static emux_handle_t sio_init(int mux_id, int virtual_port_id)
{
        emux_handle_t mh;

    setPPPEscapeString_sio(DEF_ESCAPE_STR,3,DEF_TIME_BEFORE_ESCAPE,DEF_TIME_AFTER_ESCAPE);

    mh = MUX_Init(mux_id, virtual_port_id, NULL, 0);
    MUX_StartRxDataEventCallback(mh, sendToPpp, NULL);

        return mh;
}

sio_fd_t sio_open(void *sio_arg)
{
    pppos_arg_t *pppos_arg = (pppos_arg_t *) sio_arg;
    emux_handle_t mh;
    int mux_id = pppos_arg->mux_id;
    int virtual_port_id = pppos_arg->virtual_port_id;

    LWIP_ASSERT("devnum < MAX_DEVICE", (virtual_port_id < MAX_DEVICE));
    /* would be nice with dynamic memory alloc */
/*     siostruct_t * tmp; */



    //print_boot_msg("sio_open: for muxID %d virtualPortID=%d\n", mux_id, virtual_port_id);

        if(( mh = (emux_handle_t*) sio_init(mux_id, virtual_port_id)) == NULL)
    {
        LWIP_DEBUGF(SIO_DEBUG, ("sio_open: ERROR opening serial device logical_serial_id=%d\n", logical_serial_id));
        //abort( );
        return NULL;
    }
    LWIP_DEBUGF(SIO_DEBUG, ("sio_open[%p]:  open.\n", mh));
    //save the logical port this instance uses

    return (sio_fd_t) mh;
}


#if (PPP_SUPPORT || LWIP_HAVE_SLIPIF)
#if 0
int sioToggle(sio_fd_t sioFd)
{
    sio_status_t *fd = (sio_status_t *)sioFd ;
    SWITCH_ID switchId = serialSwitchGetPortSwitchApi(fd->logical_serial_id);
    if (serialSwitchIsPortToggledApi(switchId, fd->logical_serial_id)){
       serialMngrToggleApi(serialChNum);
       return 1;
    }
       return 0;
}
#endif

u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len)
{
    int ret;
    if (fd== NULL) {
       return 0;/*No data transmitted */
    }


    LWIP_DEBUGF(SIO_DEBUG,("\nsio_write to UART1 len=%d\n",len));

    print_ppp_message(data,len,"sio_write");
    ret = MUX_Send((emux_handle_t)fd, data, len);
    if (ret == emux_Transfer_Success)
        return len;
    else
        return 0;

}
/*void sio_send( u8_t c, sio_status_t * siostat )*/
void sio_send(u8_t c, sio_fd_t fd)
{
    if (fd== NULL) {
        return ;/*No data transmitted */
    }
    /*    sio_status_t * siostat = ((siostruct_t*)netif->state)->sio; */
/*
    if ( write( siostat->fd, &c, 1 ) <= 0 )
    {
        LWIP_DEBUGF(SIO_DEBUG, ("sio_send[%d]: write refused\n", siostat->fd));
    }

  const char * ptr_char=(const char *)ptr;
*/
    print_ppp_message(&c,1,"sio_send");
    MUX_Send((emux_handle_t)fd, &c, 1);
}
#if 0
static void debugPrint(u8_t indication,u8_t dataRecvWaitFlag)
{
    printf ("Debug got indication to xQueue indication=%d *dataRecvWaitFlag=%d\n",indication,dataRecvWaitFlag);
}
#endif
#if 0
/*u32_t sio_read(sio_status_t * siostat, u8_t *buf, u32_t size) */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
    /*ssize_t rsz = read( siostat->fd, buf, size );
    return rsz < 0 ? 0 : rsz;
    */
    //int index=0;
    //unsigned long ulReceivedValue;
        char *ptr_char=(char*)data;
//        for (index=0;index<len;index+=sizeof(unsigned long))
        {
            /* Wait until something arrives in the queue - this task will block
            indefinitely provided INCLUDE_vTaskSuspend is set to 1 in FreeRTOSConfig.h. */
            xQueueReceive( xQueue_input, (ptr_char+len), portMAX_DELAY );
            //*(unsigned long*)(ptr_char+index) = ulReceivedValue;
        }
        if (*(ptr_char+len) == 0x7e)
          return 1;
        else
          return 0;
}
#endif
/*void sio_read_abort(sio_status_t * siostat)*/
void sio_read_abort(sio_fd_t fd)
{
    LWIP_UNUSED_ARG(fd);
    printf("sio_read_abort[%d]: not yet implemented for freeRTOS\n",(int) fd);
}
#endif /* (PPP_SUPPORT || LWIP_HAVE_SLIPIF) */

