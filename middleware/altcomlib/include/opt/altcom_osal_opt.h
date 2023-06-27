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

#ifndef __ALTCOM_OSAL_OPT_H__
#define __ALTCOM_OSAL_OPT_H__

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <event_groups.h>
#include <timers.h>
#include <stdbool.h>
#include <errno.h>

#define ALTCOM_SYS_MALLOC(sz) pvPortMalloc(sz)
#define ALTCOM_SYS_FREE(ptr) vPortFree(ptr)

#if defined(__GNUC__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htons(x) (x)
#define ntohs(x) (x)
#define htonl(x) (x)
#define ntohl(x) (x)
#define ntohd(x) (x)
#define htond(x) (x)
#else /* __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__ */
#ifndef htons
#define htons(x) ((((x)&0x00ffUL) << 8) | (((x)&0xff00UL) >> 8))
#endif

#ifndef ntohs
#define ntohs(x) htons(x)
#endif

#ifndef htonl
#define htonl(x)                                                                        \
  ((((x)&0x000000ffUL) << 24) | (((x)&0x0000ff00UL) << 8) | (((x)&0x00ff0000UL) >> 8) | \
   (((x)&0xff000000UL) >> 24))
#endif

#ifndef ntohl
#define ntohl(x) htonl(x)
#endif

#define htonll(x)                                                              \
  ((((x)&0x00000000000000ffULL) << 56) | (((x)&0x000000000000ff00ULL) << 40) | \
   (((x)&0x0000000000ff0000ULL) << 24) | (((x)&0x00000000ff000000ULL) << 8) |  \
   (((x)&0x000000ff00000000ULL) >> 8) | (((x)&0x0000ff0000000000ULL) >> 24) |  \
   (((x)&0x00ff000000000000ULL) >> 40) | (((x)&0xff00000000000000ULL) >> 56))
#define ntohll(x) htonll(x)
#define ntohd(x) conv(x)
#define htond(x) conv(x)

static inline double conv(double in) {
  double result;
  unsigned int i;
  char *dest = (char *)&result;
  char *src = (char *)&in;
  for (i = 0; i < sizeof(double); i++) {
    dest[i] = src[sizeof(double) - i - 1];
  }
  return result;
}

#endif

#elif defined(__ICCARM__)
#if __ARM_BIG_ENDIAN
#define htons(x) (x)
#define ntohs(x) (x)
#define htonl(x) (x)
#define ntohl(x) (x)
#else
#ifndef htons
#define htons(x) ((((x)&0x00ffUL) << 8) | (((x)&0xff00UL) >> 8))
#endif

#ifndef ntohs
#define ntohs(x) htons(x)
#endif

#ifndef htonl
#define htonl(x)                                                                        \
  ((((x)&0x000000ffUL) << 24) | (((x)&0x0000ff00UL) << 8) | (((x)&0x00ff0000UL) >> 8) | \
   (((x)&0xff000000UL) >> 24))
#endif

#ifndef ntohl
#define ntohl(x) htonl(x)
#endif

#define htonll(x)                                                              \
  ((((x)&0x00000000000000ffULL) << 56) | (((x)&0x000000000000ff00ULL) << 40) | \
   (((x)&0x0000000000ff0000ULL) << 24) | (((x)&0x00000000ff000000ULL) << 8) |  \
   (((x)&0x000000ff00000000ULL) >> 8) | (((x)&0x0000ff0000000000ULL) >> 24) |  \
   (((x)&0x00ff000000000000ULL) >> 40) | (((x)&0xff00000000000000ULL) >> 56))
#define ntohll(x) htonll(x)
#define ntohd(x) conv(x)
#define htond(x) conv(x)

static inline double conv(double in) {
  double result;
  unsigned int i;
  char *dest = (char *)&result;
  char *src = (char *)&in;
  for (i = 0; i < sizeof(double); i++) {
    dest[i] = src[sizeof(double) - i - 1];
  }
  return result;
}

#endif /*__ARM_BIG_ENDIAN*/

#else /*!__GNUC__ && !__ICCARM__*/
#error Please define endian detection macro for this toolchain
#endif /*__GNUC__*/

#ifndef STACK_WORD_SIZE
#define STACK_WORD_SIZE 4
#endif

#ifndef EPERM
#define EPERM 1
#endif
#ifndef EIO
#define EIO 5
#endif
#ifndef EAGAIN
#define EAGAIN 11
#endif
#ifndef ENOMEM
#define ENOMEM 12
#endif
#ifndef EBUSY
#define EBUSY 16
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef ENOSPC
#define ENOSPC 28
#endif
#ifndef ETIME
#define ETIME 62
#endif
#ifndef ECONNABORTED
#define ECONNABORTED 113
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 116
#endif

typedef TaskHandle_t altcom_sys_task_t;
typedef SemaphoreHandle_t altcom_sys_sem_t;
typedef QueueHandle_t altcom_sys_mq_t;
typedef SemaphoreHandle_t altcom_sys_mutex_t;
typedef EventGroupHandle_t altcom_sys_evflag_t;
typedef EventBits_t altcom_sys_evflag_ptn_t;
typedef BaseType_t altcom_sys_evflag_mode_t;
typedef TimerHandle_t altcom_sys_timer_t;

#define ALTCOM_MAX_TASKS_WAITING_ON_COND_VAR 10
typedef struct {
  SemaphoreHandle_t cond_mutex;
  int num_waiting;
  QueueHandle_t evtQue;
} altcom_thread_cond_variable_t;

typedef int32_t altcom_sys_thread_cond_t;
typedef int32_t altcom_sys_thread_condattr_t;

#endif /* __ALTCOM_OSAL_OPT_H__ */
