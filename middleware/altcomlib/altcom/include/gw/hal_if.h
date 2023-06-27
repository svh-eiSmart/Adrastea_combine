/****************************************************************************
 * modules/lte/altcom/gw/hal_if.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __MODULES_LTE_ALTCOM_GW_HAL_IF_H
#define __MODULES_LTE_ALTCOM_GW_HAL_IF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include "altcom_osal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define HAL_LOCK(handle)            \
  do {                              \
    altcom_sys_lock_mutex(&handle); \
  } while (0)

#define HAL_UNLOCK(handle)            \
  do {                                \
    altcom_sys_unlock_mutex(&handle); \
  } while (0)

#define HAL_NULL_POINTER_CHECK(param)      \
  do {                                     \
    if (!param) {                          \
      DBGIF_LOG_ERROR("NULL parameter\n"); \
      return -EINVAL;                      \
    }                                      \
  } while (0)

#define HAL_RECV_ABORT_CHECK()                    \
  do {                                            \
    HAL_LOCK(obj->objintmtx);                     \
    if (g_abort_type == HAL_ABORT_TERMINATE) {    \
      HAL_UNLOCK(obj->objintmtx);                 \
      return -ECONNABORTED;                       \
    } else if (g_abort_type == HAL_ABORT_AGAIN) { \
      g_abort_type = HAL_ABORT_NONE;              \
      HAL_UNLOCK(obj->objintmtx);                 \
      return -EAGAIN;                             \
    }                                             \
    HAL_UNLOCK(obj->objintmtx);                   \
  } while (0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum { HAL_ABORT_NONE, HAL_ABORT_TERMINATE, HAL_ABORT_AGAIN } hal_if_abort_type_t;

struct hal_if_s {
  CODE int32_t (*send)(FAR struct hal_if_s *thiz, FAR const uint8_t *data, uint32_t len);
  CODE int32_t (*recv)(FAR struct hal_if_s *thiz, FAR uint8_t *buffer, uint32_t len);
  CODE int32_t (*abortrecv)(FAR struct hal_if_s *thiz, hal_if_abort_type_t abort_type);
  CODE int32_t (*lock)(FAR struct hal_if_s *thiz);
  CODE int32_t (*unlock)(FAR struct hal_if_s *thiz);
  CODE void *(*allocbuff)(FAR struct hal_if_s *thiz, uint32_t len);
  CODE int32_t (*freebuff)(FAR struct hal_if_s *thiz, FAR void *buff);
};

#endif /* __MODULES_LTE_ALTCOM_GW_HAL_IF_H */
