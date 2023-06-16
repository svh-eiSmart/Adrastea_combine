/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

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
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "alt_osal.h"
#include "hal_uart_alt125x.h"
#include "dbg_if.h"
#include "buffpoolwrapper.h"
#include "serial.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifdef CONFIG_UART_MAX_PAYLOAD_SIZE
#define HAL_UART_ALT125X_MAXPACKETSIZE (CONFIG_UART_MAX_PAYLOAD_SIZE)
#else
#define HAL_UART_ALT125X_MAXPACKETSIZE (5000)
#endif

#define HAL_UART_ALT125X_SERIAL_TYPE_ID SERIAL_TYPE_UART_ID
#define HAL_UART_ALT125X_LOGICAL_SERIAL_ID LS_REPRESENTATION_ID_UART_0

#define HAL_UART_BUFF_END_ADDR(buff) ((buff) + HAL_UART_ALT125X_MAXPACKETSIZE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hal_uart_alt125x_obj_s {
  struct hal_if_s hal_if;
  alt_osal_mutex_handle objextmtx;
  alt_osal_mutex_handle objintmtx;
  void *serialhdl;
  size_t (*txfunc)(serial_handle handle, void *buf, size_t len);
  size_t (*rxfunc)(serial_handle handle, void *buf, size_t len);
  int32_t (*abortfunc)(serial_handle handle);
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int32_t hal_uart_alt125x_send(struct hal_if_s *thiz, const uint8_t *data, uint32_t len);
static int32_t hal_uart_alt125x_recv(struct hal_if_s *thiz, uint8_t *buffer, uint32_t len);
static int32_t hal_uart_alt125x_abortrecv(struct hal_if_s *thiz, hal_if_abort_type_t abort_type);
static int32_t hal_uart_alt125x_lock(struct hal_if_s *thiz);
static int32_t hal_uart_alt125x_unlock(struct hal_if_s *thiz);
static void *hal_uart_alt125x_allocbuff(struct hal_if_s *thiz, uint32_t len);
static int32_t hal_uart_alt125x_freebuff(struct hal_if_s *thiz, void *buff);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static void *g_serialhdl = NULL;
static hal_if_abort_type_t g_abort_type = HAL_ABORT_NONE;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hal_uart_alt125x_send
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

static int32_t hal_uart_alt125x_send(struct hal_if_s *thiz, const uint8_t *data, uint32_t len) {
  size_t ret;
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);
  if (NULL == data || 0 == len || HAL_UART_ALT125X_MAXPACKETSIZE < len) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return -EINVAL;
  }

  obj = (struct hal_uart_alt125x_obj_s *)thiz;
  ret = (int32_t)obj->txfunc(obj->serialhdl, (void *)data, (size_t)len);

  return (ret == (size_t)len) ? (int32_t)ret : -EIO;
}

/****************************************************************************
 * Name: hal_uart_alt125x_recv
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

static int32_t hal_uart_alt125x_recv(struct hal_if_s *thiz, uint8_t *buffer, uint32_t len) {
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);
  if (NULL == buffer || 0 == len || HAL_UART_ALT125X_MAXPACKETSIZE < len) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return -EINVAL;
  }

  obj = (struct hal_uart_alt125x_obj_s *)thiz;

  /* Check abort flag. */
  HAL_RECV_ABORT_CHECK();

  /* Read from media */
  return (int32_t)obj->rxfunc(obj->serialhdl, (void *)buffer, (size_t)len);
}

/****************************************************************************
 * Name: hal_uart_alt125x_abortrecv
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

static int32_t hal_uart_alt125x_abortrecv(struct hal_if_s *thiz, hal_if_abort_type_t abort_type) {
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);
  obj = (struct hal_uart_alt125x_obj_s *)thiz;
  HAL_LOCK(obj->objintmtx);
  g_abort_type = abort_type;
  HAL_UNLOCK(obj->objintmtx);
  obj->abortfunc(obj->serialhdl);
  return 0;
}

/****************************************************************************
 * Name: hal_uart_alt125x_lock
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

static int32_t hal_uart_alt125x_lock(struct hal_if_s *thiz) {
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_alt125x_obj_s *)thiz;
  HAL_LOCK(obj->objextmtx);

  return 0;
}

/****************************************************************************
 * Name: hal_uart_alt125x_unlock
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

static int32_t hal_uart_alt125x_unlock(struct hal_if_s *thiz) {
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_alt125x_obj_s *)thiz;
  HAL_UNLOCK(obj->objextmtx);

  return 0;
}

/****************************************************************************
 * Name: hal_uart_alt125x_allocbuff
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

static void *hal_uart_alt125x_allocbuff(struct hal_if_s *thiz, uint32_t len) {
  return BUFFPOOL_ALLOC(len);
}

/****************************************************************************
 * Name: hal_uart_alt125x_freebuff
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

static int32_t hal_uart_alt125x_freebuff(struct hal_if_s *thiz, void *buff) {
  return BUFFPOOL_FREE(buff);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hal_uart_alt125x_create
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

struct hal_if_s *hal_uart_alt125x_create(void) {
  struct hal_uart_alt125x_obj_s *obj = NULL;
  alt_osal_mutex_attribute mutex_param = {0};
  int32_t ret;

  /* Create data object */

  obj = (struct hal_uart_alt125x_obj_s *)BUFFPOOL_ALLOC(sizeof(struct hal_uart_alt125x_obj_s));
  DBGIF_ASSERT(obj, "Data obj allocate failed.\n");

  if (!obj) {
    goto objerr;
  }

  memset(obj, 0, sizeof(struct hal_uart_alt125x_obj_s));

  /* Set interface */

  obj->hal_if.send = hal_uart_alt125x_send;
  obj->hal_if.recv = hal_uart_alt125x_recv;
  obj->hal_if.abortrecv = hal_uart_alt125x_abortrecv;
  obj->hal_if.lock = hal_uart_alt125x_lock;
  obj->hal_if.unlock = hal_uart_alt125x_unlock;
  obj->hal_if.allocbuff = hal_uart_alt125x_allocbuff;
  obj->hal_if.freebuff = hal_uart_alt125x_freebuff;

  /* Create mutex. */
  ret = alt_osal_create_mutex(&obj->objextmtx, &mutex_param);
  DBGIF_ASSERT(0 == ret, "objextmtx create failed.\n");

  if (ret) {
    goto objextmtxerr;
  }

  ret = alt_osal_create_mutex(&obj->objintmtx, &mutex_param);
  DBGIF_ASSERT(0 == ret, "objintmtx create failed.\n");

  if (ret) {
    goto objintmtxerr;
  }

  /* Initialize SerialMngr. */

  if (!g_serialhdl) {
    DBGIF_LOG_INFO("First init, create serialhdl.\n");

    g_serialhdl = serial_open(ACTIVE_UARTI0);
    DBGIF_ASSERT(g_serialhdl, "Serial handle allocate failed.\n");

    if (!g_serialhdl) {
      goto serialhdlerr;
    }
  }

  obj->serialhdl = g_serialhdl;
  obj->txfunc = serial_write;
  obj->rxfunc = serial_read;
  obj->abortfunc = abort_read;
  return (struct hal_if_s *)obj;

serialhdlerr:
  alt_osal_delete_mutex(&obj->objintmtx);

objintmtxerr:
  alt_osal_delete_mutex(&obj->objextmtx);

objextmtxerr:
  BUFFPOOL_FREE(obj);

objerr:
  return NULL;
}

/****************************************************************************
 * Name: hal_uart_alt125x_delete
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

int32_t hal_uart_alt125x_delete(struct hal_if_s *thiz) {
  struct hal_uart_alt125x_obj_s *obj = NULL;

  HAL_NULL_POINTER_CHECK(thiz);

  obj = (struct hal_uart_alt125x_obj_s *)thiz;

  serial_close(obj->serialhdl);
  g_serialhdl = NULL;

  alt_osal_delete_mutex(&obj->objintmtx);
  alt_osal_delete_mutex(&obj->objextmtx);
  BUFFPOOL_FREE(obj);
  g_abort_type = HAL_ABORT_NONE;

  return 0;
}
