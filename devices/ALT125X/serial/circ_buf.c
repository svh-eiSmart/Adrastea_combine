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
#include <stddef.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include "portmacro.h"
#include "125X_mcu.h"
#include "stdatomic.h"

typedef struct circ_buf_control {
  unsigned int head;
  unsigned int tail;
  // volatile int cnt;
  atomic_uint_fast32_t cnt;
  unsigned int buf_size;
} circ_buf_control_t;

typedef struct circ_buf {
  circ_buf_control_t control;
  unsigned char* buf;
} circ_buf_t;

#define FREE(x) vPortFree(x)
#define MALLOC(x) pvPortMalloc(x)
#define CIRC_BUF_OBJ_SIZE (sizeof(circ_buf_t) + buf_size)

#define uart_circ_empty(circ) ((circ)->control.cnt == 0)  //((circ)->head == (circ)->tail)
#define uart_circ_clear(circ, bufSize)                                                            \
  {                                                                                               \
    (circ)->control.head = (circ)->control.tail = (circ)->control.cnt = ATOMIC_VAR_INIT(0) /*0*/; \
    (circ)->control.buf_size = bufSize;                                                           \
  }
#define uart_circ_chars_free(circ) \
  ((unsigned int)(circ)->control.cnt != ((circ)->control.buf_size - 1))

int serialAllocCircBufUtil(void** xmitBuffer, unsigned int buf_size) {
  *xmitBuffer = NULL;
  *xmitBuffer = MALLOC(CIRC_BUF_OBJ_SIZE);
  if (*xmitBuffer != NULL) {
    circ_buf_t* circBufPtr = (circ_buf_t*)*xmitBuffer;
    uart_circ_clear(circBufPtr, buf_size);
    circBufPtr->buf =
        (unsigned char*)((unsigned int)circBufPtr + sizeof(circ_buf_control_t) + sizeof(void*));
    return 1;
  } else
    return 0;
}
void serialFreeCircBufUtil(void* Buffer) { FREE(Buffer); }

int serialCircBufIsEmpty(void* xmitBuffer) { return uart_circ_empty((circ_buf_t*)xmitBuffer); }

int serialCircBufFree(void* xmitBuffer) { return uart_circ_chars_free((circ_buf_t*)xmitBuffer); }

void serialCircBufInsert(void* xmit, char c) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  circBufPtr->buf[circBufPtr->control.head] = c;
  circBufPtr->control.head = (circBufPtr->control.head + 1) & (circBufPtr->control.buf_size - 1);
  atomic_fetch_add(&circBufPtr->control.cnt, 1);
}

void serialCircBufGetChar(void* xmit, uint8_t* dataChar) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  *dataChar = circBufPtr->buf[circBufPtr->control.tail];
  circBufPtr->control.tail = (circBufPtr->control.tail + 1) & (circBufPtr->control.buf_size - 1);

  atomic_fetch_sub(&circBufPtr->control.cnt, 1);
}

void serialCircGetCharAndCopy(void* xmit, volatile void* uartDataReg) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  // char retChar = circBufPtr->buf[circBufPtr->control.tail];
  REGISTER(uartDataReg) = circBufPtr->buf[circBufPtr->control.tail];
  circBufPtr->control.tail = (circBufPtr->control.tail + 1) & (circBufPtr->control.buf_size - 1);

  atomic_fetch_sub(&circBufPtr->control.cnt, 1);
}
int serialCircBufIsdata(void* xmit) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;

  return (circBufPtr->control.tail == circBufPtr->control.head);
}

int serialCircBufGetUsage(void* xmitBufPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmitBufPtr;

  if (circBufPtr == NULL) return 0;

  return circBufPtr->control.cnt;
  /*
  if(circBufPtr->control.head >= circBufPtr->control.tail)
          return (circBufPtr->control.head - circBufPtr->control.tail);

   return (circBufPtr->control.buf_size - circBufPtr->control.tail + circBufPtr->control.head);
  */
}

int serialCircBufGetSize(void* xmitBufPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmitBufPtr;

  if (circBufPtr == NULL) return 0;
  return circBufPtr->control.buf_size;
}

int serialCircBufGetRoom(void* xmitBufPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmitBufPtr;

  return circBufPtr->control.buf_size - circBufPtr->control.cnt;
}
