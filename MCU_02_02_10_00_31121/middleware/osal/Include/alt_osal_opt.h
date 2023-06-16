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

#ifndef MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_OPT_H
#define MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_OPT_H

#include <stdint.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <event_groups.h>
#include <timers.h>
#include <errno.h>

#ifndef ALT_OSAL_MALLOC
#define ALT_OSAL_MALLOC(sz) malloc(sz)
#endif

#ifndef ALT_OSAL_FREE
#define ALT_OSAL_FREE(ptr) free(ptr)
#endif

#ifndef FAR
#define FAR
#endif

#ifndef CODE
#define CODE
#endif

#ifndef begin_packed_struct
#define begin_packed_struct
#endif

#ifndef end_packed_struct
#define end_packed_struct __attribute__((__packed__))
#endif

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

typedef StackType_t alt_osal_stack_type;

#ifndef STACK_WORD_SIZE
#define STACK_WORD_SIZE (sizeof(alt_osal_stack_type))
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
#ifndef EALRDYDONE
#define EALRDYDONE 125
#endif
#ifndef EWONTDO
#define EWONTDO 126
#endif

#define MIN_STATIC_TASK_CBSIZE (sizeof(StaticTask_t))
#define MIN_STATIC_SEM_CBSIZE (sizeof(StaticSemaphore_t))
#define MIN_STATIC_MUTEX_CBSIZE (sizeof(StaticSemaphore_t))
#define MIN_STATIC_QUEUE_CBSIZE (sizeof(StaticQueue_t))
#define MIN_STATIC_EVTFLAG_CBSIZE (sizeof(StaticEventGroup_t))
#define MIN_STATIC_TIMER_CBSIZE (sizeof(StaticTimer_t))
#define MAX_BITS_TASK_NOTIFY 31U

#define TASK_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))

#endif /* MIDDLEWARE_OSAL_INCLUDE_ALT_OSAL_OPT_H */
