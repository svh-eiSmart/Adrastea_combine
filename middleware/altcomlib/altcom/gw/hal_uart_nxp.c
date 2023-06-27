/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "hal_uart_nxp.h"
#include "dbg_if.h"
#include "buffpoolwrapper.h"
#include "serial_container.h"
#include "hifc_api.h"
#include "app_config.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifdef CONFIG_UART_MAX_PAYLOAD_SIZE
#define HAL_UART_NXP_MAXPACKETSIZE (CONFIG_UART_MAX_PAYLOAD_SIZE)
#else
#define HAL_UART_NXP_MAXPACKETSIZE (3092)
#endif

#define HAL_UART_NXP_SERIAL_TYPE_ID SERIAL_TYPE_UART_ID
#define HAL_UART_NXP_LOGICAL_SERIAL_ID LS_REPRESENTATION_ID_UART_0
#define HAL_UART_NXP_SERIAL_RX_BUFFSIZE (4096) /* Must be power of 2 */

#define HAL_UART_BUFF_END_ADDR(buff) ((buff) + HAL_UART_NXP_MAXPACKETSIZE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hal_uart_nxp_obj_s {
  struct hal_if_s hal_if;
  uint8_t *recvbuff;
  uint8_t *writeptr;
  uint8_t *readptr;
  altcom_sys_mutex_t objextmtx;
  altcom_sys_mutex_t objintmtx;
  altcom_sys_sem_t objsem;
  serial_handle serialhdl;
  int (*txfunc)(serial_handle handle, const uint8_t *buf, uint32_t len);
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

uint8_t alt1250_background_buffer[ALT1250_LPUART_BACKGROUND_BUF_LEN];
lpuart_rtos_config_t alt1250_lpuart_config = {.base = ALT1250_LPUART_BASE,
                                              .baudrate = ALT1250_LPUART_BAUDRATE,
                                              .parity = kLPUART_ParityDisabled,
                                              .stopbits = kLPUART_OneStopBit,
                                              .buffer = alt1250_background_buffer,
                                              .buffer_size = sizeof(alt1250_background_buffer),
                                              .enableRxRTS = false,
                                              .enableTxCTS = false};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int32_t hal_uart_nxp_send(struct hal_if_s *thiz, const uint8_t *data, uint32_t len);
static int32_t hal_uart_nxp_recv(struct hal_if_s *thiz, uint8_t *buffer, uint32_t len);
static int32_t hal_uart_nxp_abortrecv(struct hal_if_s *thiz, hal_if_abort_type_t abort_type);
static int32_t hal_uart_nxp_lock(struct hal_if_s *thiz);
static int32_t hal_uart_nxp_unlock(struct hal_if_s *thiz);
static void *hal_uart_nxp_allocbuff(struct hal_if_s *thiz, uint32_t len);
static int32_t hal_uart_nxp_freebuff(struct hal_if_s *thiz, void *buff);
static void hal_uart_nxp_recv_callback(const char c, void *appCookie);

/****************************************************************************
 * Private Data
 ****************************************************************************/

serial_handle g_serialhdl = NULL;
static hal_if_abort_type_t g_abort_type = HAL_ABORT_NONE;

/****************************************************************************
 * Inline functions
 ****************************************************************************/

static inline bool HAL_UART_NXP_IS_NO_READABLE_DATA(struct hal_uart_nxp_obj_s *obj) {
  bool ret = false;
  HAL_LOCK(obj->objintmtx);
  ret = (obj->writeptr == obj->readptr) ? true : false;
  HAL_UNLOCK(obj->objintmtx);
  return ret;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void reset_indication(void) {
  // printf("get reset indication\n");
  NVIC_SystemReset();
}

static int BOARD_InitHIFC() {
  int ret;
  ret = hifc_host_interface_init(ALT1250_LPUART_BASE);
  if (ret) {
    printf("HIFC init failed!.\r\n");
    return ret;
  }
  NVIC_SetPriority(PORTD_IRQn, 5);
  EnableIRQ(PORTD_IRQn);

  /* init reset indication*/
  ret = hifc_rst_indication_init(reset_indication);
  if (ret) {
    printf("init reset indication fail\n");
    return ret;
  }

  return 0;
}

/****************************************************************************
 * Name: hal_uart_nxp_send
 *
 * Description:
 *   Transfer data by internal UART.
 *
 * Input Parameters:
 *   thiz     struct hal_if_s pointer(i.e. instance of internal UART gateway).
 *   data     Buffer that stores send data.
 *   len      @data length.
 *
 * Returned Value:
 *   If the send succeed, it returned send size.
 *   Otherwise 0 is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_send(struct hal_if_s *thiz, const uint8_t *data, uint32_t len) {
  int ret;
  struct hal_uart_nxp_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);
  if (NULL == data || 0 == len || HAL_UART_NXP_MAXPACKETSIZE < len) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return -EINVAL;
  }

  obj = (struct hal_uart_nxp_obj_s *)thiz;

  HAL_LOCK(obj->objintmtx);
  ret = obj->txfunc(obj->serialhdl, data, len);
  HAL_UNLOCK(obj->objintmtx);

  return (SERIAL_CONTAINER_SUCCESS == ret) ? (int32_t)len : -EIO;
}

/****************************************************************************
 * Name: hal_uart_nxp_recv
 *
 * Description:
 *   Receive data with buffer.
 *   This function is blocking.
 *
 * Input Parameters:
 *   thiz       struct hal_if_s pointer
 *              (i.e. instance of internal UART gateway).
 *   buffer     Buffer for storing received data.
 *   len        @buffer length.
 *
 * Returned Value:
 *   If the receive succeed, it returned receive size.
 *   If receive abort packet, returned recieve size.
 *   Otherwise 0 is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_recv(struct hal_if_s *thiz, uint8_t *buffer, uint32_t len) {
  struct hal_uart_nxp_obj_s *obj = NULL;
  uint32_t i;
  int32_t sz = 0;

  HAL_NULL_POINTER_CHECK(thiz);
  if (NULL == buffer || 0 == len || HAL_UART_NXP_MAXPACKETSIZE < len) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return -EINVAL;
  }

  obj = (struct hal_uart_nxp_obj_s *)thiz;

  /* Check abort flag. */
  HAL_RECV_ABORT_CHECK();

  /* Check whether readable data exsit. */
  while (HAL_UART_NXP_IS_NO_READABLE_DATA(obj)) {
    /* Get into blocking state. */
    altcom_sys_wait_semaphore(obj->objsem, ALTCOM_SYS_TIMEO_FEVR);
    /* Check abort flag again. */
    HAL_RECV_ABORT_CHECK();
  }

  HAL_LOCK(obj->objintmtx);
  for (i = 0; i < len; i++) {
    *buffer = *obj->readptr;
    buffer++;
    obj->readptr++;
    sz++;

    if (obj->writeptr == obj->readptr) {
      break;
    }
  }
  HAL_UNLOCK(obj->objintmtx);

  return sz;
}

/****************************************************************************
 * Name: hal_uart_nxp_abortrecv
 *
 * Description:
 *   Abort receive processing.
 *   This is unsupported function.
 *
 * Input Parameters:
 *   thiz  struct hal_if_s pointer(i.e. instance of internal UART gateway).
 *   abort_type abort by terminating flow or receive again
 *
 * Returned Value:
 *   Always 0 is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_abortrecv(struct hal_if_s *thiz, hal_if_abort_type_t abort_type) {
  struct hal_uart_nxp_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);
  obj = (struct hal_uart_nxp_obj_s *)thiz;
  HAL_LOCK(obj->objintmtx);
  g_abort_type = abort_type;
  HAL_UNLOCK(obj->objintmtx);
  altcom_sys_post_semaphore(obj->objsem);
  return 0;
}

/****************************************************************************
 * Name: hal_uart_nxp_lock
 *
 * Description:
 *   Lock the internal UART gateway object.
 *
 * Input Parameters:
 *   thiz  struct hal_if_s pointer(i.e. instance of internal UART gateway).
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_lock(struct hal_if_s *thiz) {
  struct hal_uart_nxp_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_nxp_obj_s *)thiz;
  HAL_LOCK(obj->objextmtx);

  return 0;
}

/****************************************************************************
 * Name: hal_uart_nxp_unlock
 *
 * Description:
 *   Unlock the internal UART gateway object.
 *
 * Input Parameters:
 *   thiz  struct hal_if_s pointer(i.e. instance of internal UART gateway).
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_unlock(struct hal_if_s *thiz) {
  struct hal_uart_nxp_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_nxp_obj_s *)thiz;
  HAL_UNLOCK(obj->objextmtx);

  return 0;
}

/****************************************************************************
 * Name: hal_uart_nxp_allocbuff
 *
 * Description:
 *   Allocat buffer for internal UART transaction message.
 *
 * Input Parameters:
 *   len      Allocat memory size.
 *
 * Returned Value:
 *   If succeeds allocate buffer, start address of the data field
 *   is returned. Otherwise NULL is returned.
 *
 ****************************************************************************/

static void *hal_uart_nxp_allocbuff(struct hal_if_s *thiz, uint32_t len) {
  return BUFFPOOL_ALLOC(len);
}

/****************************************************************************
 * Name: hal_uart_nxp_freebuff
 *
 * Description:
 *   Free buffer for internal UART transaction message.
 *
 * Input Parameters:
 *   buff      Allocated memory pointer.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

static int32_t hal_uart_nxp_freebuff(struct hal_if_s *thiz, void *buff) {
  return BUFFPOOL_FREE(buff);
}

/****************************************************************************
 * Name: hal_uart_nxp_recv_callback
 *
 * Description:
 *   Recieve data from internal UART.
 *
 * Input Parameters:
 *   c             Recieved 1 byte data.
 *   appCookie     cookie that being passed from application
 *                 and assign to the logical port.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void hal_uart_nxp_recv_callback(const char c, void *appCookie) {
  struct hal_uart_nxp_obj_s *obj;
  int32_t spent_size = 0;

  DBGIF_ASSERT(appCookie, "appCookie is NULL.\n");

  obj = (struct hal_uart_nxp_obj_s *)appCookie;

  HAL_LOCK(obj->objintmtx);

  //  DBGIF_LOG2_DEBUG("[%s]0x%02X\n", __func__, c & 0xFF);

  /* Check whether writeptr reached end of buffer. */
  if (obj->writeptr == HAL_UART_BUFF_END_ADDR(obj->recvbuff)) {
    spent_size = obj->readptr - obj->recvbuff;
    if (spent_size == 0) {
      /* No free space, discard data. */
      DBGIF_LOG_INFO("Reach end of buff. Discard due to no space.\n");
      HAL_UNLOCK(obj->objintmtx);
      return;
    }
    DBGIF_LOG1_INFO("Reach end of buff. Shift sz:%ld:\n", spent_size);
    memmove(obj->recvbuff, obj->readptr, HAL_UART_NXP_MAXPACKETSIZE - spent_size);
    obj->readptr -= spent_size;
    obj->writeptr -= spent_size;
  }

  *obj->writeptr = c;
  obj->writeptr++;

  altcom_sys_post_semaphore(obj->objsem);
  HAL_UNLOCK(obj->objintmtx);
  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hal_uart_nxp_create
 *
 * Description:
 *   Create an object of internal UART gateway and get the instance.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   struct hal_if_s pointer(i.e. instance of internal UART gateway).
 *   If can't create instance, returned NULL.
 *
 ****************************************************************************/

struct hal_if_s *hal_uart_nxp_create(void) {
  struct hal_uart_nxp_obj_s *obj = NULL;
  altcom_sys_cremtx_s mutex_param;
  altcom_sys_cresem_s sem_param = {.initial_count = 0, .max_count = 1};
  int32_t ret;

  /* Create data object */
  obj = (struct hal_uart_nxp_obj_s *)BUFFPOOL_ALLOC(sizeof(struct hal_uart_nxp_obj_s));
  DBGIF_ASSERT(obj, "Data obj allocate failed.\n");

  if (!obj) {
    goto objerr;
  }

  memset(obj, 0, sizeof(struct hal_uart_nxp_obj_s));

  /* Set interface */
  obj->hal_if.send = hal_uart_nxp_send;
  obj->hal_if.recv = hal_uart_nxp_recv;
  obj->hal_if.abortrecv = hal_uart_nxp_abortrecv;
  obj->hal_if.lock = hal_uart_nxp_lock;
  obj->hal_if.unlock = hal_uart_nxp_unlock;
  obj->hal_if.allocbuff = hal_uart_nxp_allocbuff;
  obj->hal_if.freebuff = hal_uart_nxp_freebuff;

  /* Create recieve buffer. */
  obj->recvbuff = (uint8_t *)BUFFPOOL_ALLOC(HAL_UART_NXP_MAXPACKETSIZE);
  DBGIF_ASSERT(obj->recvbuff, "Recieve buff allocate failed.\n");

  if (!obj->recvbuff) {
    goto objrecvbufferr;
  }

  memset(obj->recvbuff, 0, HAL_UART_NXP_MAXPACKETSIZE);
  obj->writeptr = obj->recvbuff;
  obj->readptr = obj->recvbuff;

  /* Create mutex. */
  ret = altcom_sys_create_mutex(&obj->objextmtx, &mutex_param);
  DBGIF_ASSERT(0 == ret, "objextmtx create failed.\n");

  if (ret) {
    goto objextmtxerr;
  }

  ret = altcom_sys_create_mutex(&obj->objintmtx, &mutex_param);
  DBGIF_ASSERT(0 == ret, "objintmtx create failed.\n");

  if (ret) {
    goto objintmtxerr;
  }

  /* Create semaphore. */
  ret = altcom_sys_create_semaphore(&obj->objsem, &sem_param);
  DBGIF_ASSERT(0 == ret, "objsem create failed.\n");

  if (ret) {
    goto objsemerr;
  }

  /* Initialize SerialMngr. */
  if (!g_serialhdl) {
    DBGIF_LOG_INFO("First init, create serialhdl.\n");
    alt1250_lpuart_config.srcclk = ALT1250_LPUART_CLK_FREQ;
    NVIC_SetPriority(ALT1250_LPUART_IRQn, 5);
    g_serialhdl = serialDriverContainerOpen(&alt1250_lpuart_config);
    DBGIF_ASSERT(g_serialhdl, "objsem create failed.\n");

    if (!g_serialhdl) {
      goto serialhdlerr;
    }

    BOARD_InitHIFC();
  }

  obj->serialhdl = g_serialhdl;
  ret = serialDriverContainerIoctl(obj->serialhdl, IOCTL_SERIAL_RX_CB_FUNC,
                                   (void *)hal_uart_nxp_recv_callback, (void *)obj);
  DBGIF_ASSERT(ret, "Serial RX callbak registration failed.");

  if (!ret) {
    goto ioctlerr;
  }

  obj->txfunc = serialDriverContainerWrite;
  return (struct hal_if_s *)obj;

ioctlerr:
  serialDriverContainerClose(g_serialhdl);
  g_serialhdl = 0;

serialhdlerr:
  altcom_sys_delete_semaphore(&obj->objsem);

objsemerr:
  altcom_sys_delete_mutex(&obj->objintmtx);

objintmtxerr:
  altcom_sys_delete_mutex(&obj->objextmtx);

objextmtxerr:
  BUFFPOOL_FREE(obj->recvbuff);

objrecvbufferr:
  BUFFPOOL_FREE(obj);

objerr:
  return NULL;
}

/****************************************************************************
 * Name: hal_uart_nxp_delete
 *
 * Description:
 *   Delete instance of internal UART gateway.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t hal_uart_nxp_delete(struct hal_if_s *thiz) {
  struct hal_uart_nxp_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_nxp_obj_s *)thiz;
  serialDriverContainerClose(obj->serialhdl);

  g_serialhdl = NULL;
  altcom_sys_delete_semaphore(&obj->objsem);
  altcom_sys_delete_mutex(&obj->objintmtx);
  altcom_sys_delete_mutex(&obj->objextmtx);
  BUFFPOOL_FREE(obj->recvbuff);
  BUFFPOOL_FREE(obj);
  g_abort_type = HAL_ABORT_NONE;

  return 0;
}
