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
#include <stddef.h>
#include <stdlib.h>
#include "los/los_mux.h"
#include "stdatomic.h"

typedef struct circ_buf_control {
  unsigned int head;
  unsigned int tail;
  atomic_uint_fast32_t cnt;
  unsigned int buf_size;
} circ_buf_control_t;

typedef struct circ_buf {
  circ_buf_control_t control;
  unsigned char* buf;
} circ_buf_t;

#define FREE(x) losMuxFree(x)
#define MALLOC(x) losMuxMalloc(x)
#define CIRC_BUF_OBJ_SIZE (sizeof(circ_buf_t))

#define uart_circ_empty(circ) ((circ)->control.cnt == 0)  //((circ)->head == (circ)->tail)
#define uart_circ_clear(circ, bufSize)              \
  {                                                 \
    (circ)->control.head = -1;                      \
    (circ)->control.tail = (circ)->control.cnt = 0; \
    (circ)->control.buf_size = bufSize;             \
  }
#define uart_circ_chars_free(circ) ((circ)->control.cnt != ((circ)->control.buf_size - 1))

int allocCircBufUtil(void** buffer, unsigned char* buf, unsigned int buf_size) {
  *buffer = NULL;

  if (buf_size == 0 || buf == NULL) return 1;

  *buffer = MALLOC(CIRC_BUF_OBJ_SIZE);
  /*	printf("Malloc Circ: %p  size: %ld\n", *buffer, CIRC_BUF_OBJ_SIZE); */
  if (*buffer != NULL) {
    circ_buf_t* circBufPtr = (circ_buf_t*)*buffer;
    uart_circ_clear(circBufPtr, buf_size);
    circBufPtr->buf = buf;
    return 1;
  } else
    return 0;
}
void freeCircBufUtil(void** Buffer) {
  if (*Buffer) {
    /*		printf ("free: %p\n", *Buffer); */
    FREE(*Buffer);
    *Buffer = NULL;
  }
}

int circBufIsEmpty(void* buffer) { return uart_circ_empty((circ_buf_t*)buffer); }

int circBufFree(void* buffer) { return uart_circ_chars_free((circ_buf_t*)buffer); }

void circBufInsert(void* xmit, char c) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  circBufPtr->control.head = (circBufPtr->control.head + 1) & (circBufPtr->control.buf_size - 1);
  circBufPtr->buf[circBufPtr->control.head] = c;
  atomic_fetch_add(&circBufPtr->control.cnt, 1);
}

void circBufGetChar(void* xmit, uint8_t* dataChar) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  *dataChar = circBufPtr->buf[circBufPtr->control.tail];
  circBufPtr->control.tail = (circBufPtr->control.tail + 1) & (circBufPtr->control.buf_size - 1);

  atomic_fetch_sub(&circBufPtr->control.cnt, 1);
}

void* getCircBufPtr(void* xmit) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  return &circBufPtr->buf[circBufPtr->control.tail];
}

void ClearCirBuff(void* xmit, uint32_t in_bytes) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;
  uart_circ_clear(circBufPtr, in_bytes);
}

int circBufIsdata(void* xmit) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmit;

  return (circBufPtr->control.tail ==
          ((circBufPtr->control.head + 1) & (circBufPtr->control.buf_size - 1)));
}

int circBufGetUsage(void* xmitBufPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmitBufPtr;

  if (circBufPtr == NULL) return 0;

  return (circBufPtr->control.cnt);
}
void debugPrintCircBuf(void* xmitBufPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)xmitBufPtr;

  if (circBufPtr == NULL) return;
  printf("control.head=%d\ncontrol.tail=%d\ncontrol.buf_size=%d\ncnt=%d\n",
         circBufPtr->control.head, circBufPtr->control.tail, circBufPtr->control.buf_size,
         circBufPtr->control.cnt);
}
int circBufGetSize(void* BuffPtr) {
  circ_buf_t* circBufPtr = (circ_buf_t*)BuffPtr;

  if (circBufPtr == NULL) return 0;
  return circBufPtr->control.buf_size - 1;
}
