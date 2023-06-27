/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

#include <stdlib.h>
#include <stdint.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/*-----------------------------------------------------------*/

/* Initialize a system lock */
void __iar_system_Mtxinit(__iar_Rmtx *mutex) {}

/* Destroy a system lock */
void __iar_system_Mtxdst(__iar_Rmtx *mutex) {}

/* Lock a system lock */
void __iar_system_Mtxlock(__iar_Rmtx *mutex) { vTaskSuspendAll(); }

/* Unlock a system lock */
void __iar_system_Mtxunlock(__iar_Rmtx *mutex) { (void)xTaskResumeAll(); }

#define WEAK __weak
WEAK void __iar_file_Mtxinit(__iar_Rmtx *mutex) {}
WEAK void __iar_file_Mtxdst(__iar_Rmtx *mutex) {}
WEAK void __iar_file_Mtxlock(__iar_Rmtx *mutex) {}
WEAK void __iar_file_Mtxunlock(__iar_Rmtx *mutex) {}

#pragma section = "__iar_tls$$DATA"
WEAK void *__aeabi_read_tp(void) {
  // TLS is not supported, using main thread memory for errno
  return __section_begin("__iar_tls$$DATA");
}

void *pvPortMalloc(size_t xWantedSize) {
  void *pvReturn = malloc(xWantedSize);
  traceMALLOC(pvReturn, xWantedSize);

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  if (pvReturn == NULL) {
    extern void vApplicationMallocFailedHook(void);
    vApplicationMallocFailedHook();
  }
#endif

  return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree(void *pv) {
  if (pv) {
    free(pv);
    traceFREE(pv, 0);
  }
}

/* ------------------------------------------------------------------------*/

/*
 * __data_GetMemChunk --
 *
 * Gives memory to the malloc and free system.
 * The first size_t of the returned memory must hold the size of the memory chunk.
 * The returned memory must be 8-bytes aligned.
 *
 * Return 0 if there is no more memory.
 *
 */

#pragma language = extended

#pragma section = "HEAP"
#pragma section = "MOREHEAP"

static intptr_t last_returned_heap[2] = {-1, -1};

void *__data_GetMemChunk(void);

void *__data_GetMemChunk(void) {
  if (last_returned_heap[0] == -1) {
    void *s = __section_begin("HEAP");
    intptr_t is = (intptr_t)s;
    is = (is + 7) & 0xFFFFFFF8;
    s = (void *)is;
    intptr_t ie = (intptr_t)__section_end("HEAP");
    size_t *ps = (size_t *)(s);
    *ps = ie - is;
    last_returned_heap[0] = is;
    return s;
  } else if (last_returned_heap[1] == -1) {
    void *s = __section_begin("MOREHEAP");
    intptr_t is = (intptr_t)s;
    is = (is + 7) & 0xFFFFFFF8;
    s = (void *)is;
    intptr_t ie = (intptr_t)__section_end("MOREHEAP");
    size_t *ps = (size_t *)(s);
    *ps = ie - is;
    last_returned_heap[1] = is;
    return s;
  } else {
    return 0;  // No more memory...
  }
}

#define MALLINFO_FIELD_TYPE size_t

#define STRUCT_MALLINFO_DECLARED 1
struct mallinfo {
  MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
  MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
  MALLINFO_FIELD_TYPE smblks;   /* always 0 */
  MALLINFO_FIELD_TYPE hblks;    /* always 0 */
  MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
  MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
  MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
  MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
  MALLINFO_FIELD_TYPE fordblks; /* total free space */
  MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
};

struct mallinfo __iar_dlmallinfo(void);

size_t xPortGetFreeHeapSize(void) PRIVILEGED_FUNCTION {
  size_t chunksize = 0;

  if (last_returned_heap[0] == -1) {
    void *s = __section_begin("HEAP");
    intptr_t is = (intptr_t)s;
    is = (is + 7) & 0xFFFFFFF8;
    s = (void *)is;
    intptr_t ie = (intptr_t)__section_end("HEAP");
    chunksize += ie - is;
  }

  if (last_returned_heap[1] == -1) {
    void *s = __section_begin("MOREHEAP");
    intptr_t is = (intptr_t)s;
    is = (is + 7) & 0xFFFFFFF8;
    s = (void *)is;
    intptr_t ie = (intptr_t)__section_end("MOREHEAP");
    chunksize += ie - is;
  }

  struct mallinfo mi = __iar_dlmallinfo();
  return mi.fordblks + chunksize;
}
