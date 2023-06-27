/**
 * \file heap_useNewlib.c
 * \brief Wrappers required to use newlib malloc-family within FreeRTOS.
 *
 * \par Overview
 * Route FreeRTOS memory management functions to newlib's malloc family.
 * Thus newlib and FreeRTOS share memory-management routines and memory pool,
 * and all newlib's internal memory-management requirements are supported.
 *
 * \author Dave Nadler
 * \date 7-August-2019
 * \version 23-Sep-2019 comments, check no malloc call inside ISR
 *
 * \see http://www.nadler.com/embedded/newlibAndFreeRTOS.html
 * \see https://sourceware.org/newlib/libc.html#Reentrancy
 * \see https://sourceware.org/newlib/libc.html#malloc
 * \see https://sourceware.org/newlib/libc.html#index-_005f_005fenv_005flock
 * \see https://sourceware.org/newlib/libc.html#index-_005f_005fmalloc_005flock
 * \see https://sourceforge.net/p/freertos/feature-requests/72/
 * \see http://www.billgatliff.com/newlib.html
 * \see http://wiki.osdev.org/Porting_Newlib
 * \see http://www.embecosm.com/appnotes/ean9/ean9-howto-newlib-1.0.html
 *
 *
 * \copyright
 * (c) Dave Nadler 2017-2019, All Rights Reserved.
 * Web:         http://www.nadler.com
 * email:       drn@nadler.com
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Use or redistributions of source code must retain the above copyright notice,
 *   this list of conditions, ALL ORIGINAL COMMENTS, and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <malloc.h> // mallinfo...
#include <stdbool.h>

#include "newlib.h"
#if (__NEWLIB__ != 3) || (__NEWLIB_MINOR__ != 0)
  #warning "This wrapper was verified for newlib version 3.0.0; please ensure newlib's external requirements for malloc-family are unchanged!"
#endif

#include "FreeRTOS.h" // defines public interface we're implementing here
#if !defined(configUSE_NEWLIB_REENTRANT) ||  (configUSE_NEWLIB_REENTRANT!=1)
  #warning "#define configUSE_NEWLIB_REENTRANT 1 // Required for thread-safety of newlib sprintf, strtok, etc..."
  // If you're *REALLY* sure you don't need FreeRTOS's newlib reentrancy support, remove this warning...
#endif
#include "task.h"

// ================================================================================================
// External routines required by newlib's malloc (sbrk/_sbrk, __malloc_lock/unlock)
// ================================================================================================

extern char HEAP_SIZE;
extern char __HeapBase, __HeapLimit;
extern char __HeapBase0, __HeapLimit0;

static int heapBytesRemaining = (int)&HEAP_SIZE;

static struct {
    char *sbrk;
    char *limit;
} heapRegion[2] = {
    {(char*)&__HeapBase0, (char *)&__HeapLimit0},
    {(char*)&__HeapBase, (char *)&__HeapLimit},
};

#ifndef NDEBUG
    static int totalBytesProvidedBySBRK = 0;
#endif

//! _sbrk_r version supporting reentrant newlib (depends upon above symbols defined by linker control file).
void * _sbrk_r(struct _reent *pReent, int incr) {
    vTaskSuspendAll();

    // Newlib will ask for extra padding if return address is not CHUNK_ALIGN aligned
    // See https://sourceware.org/git/gitweb.cgi?p=newlib-cygwin.git;a=blob;f=newlib/libc/stdlib/nano-mallocr.c;h=13b72;hb=HEAD
    #define CHUNK_ALIGN (sizeof(void*))
    #define ALIGN_TO(size, align) \
        (((size) + (align) -1L) & ~((align) -1L))

    for (size_t i = 0; i < sizeof(heapRegion)/sizeof(*heapRegion); i++) {
        char *addr = (char*)ALIGN_TO((unsigned long) heapRegion[i].sbrk, CHUNK_ALIGN);
        char *newHeapEnd = addr + incr;
        if (newHeapEnd <= heapRegion[i].limit) {
            int alloc_size = newHeapEnd - heapRegion[i].sbrk;
            heapRegion[i].sbrk = newHeapEnd;
            heapBytesRemaining -= alloc_size;
            #ifndef NDEBUG
                totalBytesProvidedBySBRK += alloc_size;
            #endif
            (void)xTaskResumeAll();
            return addr;
        }
    }

    // Ooops, no more memory available...
    (void)xTaskResumeAll();
    return (char *)-1; // the malloc-family routine that called sbrk will return 0
}
//! non-reentrant sbrk uses is actually reentrant by using current context
// ... because the current _reent structure is pointed to by global _impure_ptr
char * sbrk(int incr) { return _sbrk_r(_impure_ptr, incr); }
//! _sbrk is a synonym for sbrk.
char * _sbrk(int incr) { return sbrk(incr); };

void __malloc_lock(struct _reent *r)     {
    bool insideAnISR = xPortIsInsideInterrupt();
    configASSERT( !insideAnISR ); // Make damn sure no more mallocs inside ISRs!!
    vTaskSuspendAll();
};
void __malloc_unlock(struct _reent *r)   {
    (void)xTaskResumeAll();
};

// newlib also requires implementing locks for the application's environment memory space,
// accessed by newlib's setenv() and getenv() functions.
// As these are trivial functions, momentarily suspend task switching (rather than semaphore).
// ToDo: Move __env_lock/unlock to a separate newlib helper file.
void __env_lock()    {       vTaskSuspendAll(); };
void __env_unlock()  { (void)xTaskResumeAll();  };

#if 0 // Provide malloc debug and accounting wrappers
  /// /brief  Wrap malloc/malloc_r to help debug who requests memory and why.
  /// To use these, add linker options: -Xlinker --wrap=malloc -Xlinker --wrap=_malloc_r
  // Note: These functions are normally unused and stripped by linker.
  int TotalMallocdBytes;
  int MallocCallCnt;
  static bool inside_malloc;
  void *__wrap_malloc(size_t nbytes) {
    extern void * __real_malloc(size_t nbytes);
    MallocCallCnt++;
    TotalMallocdBytes += nbytes;
    inside_malloc = true;
      void *p = __real_malloc(nbytes); // will call malloc_r...
    inside_malloc = false;
    return p;
  };
  void *__wrap__malloc_r(void *reent, size_t nbytes) {
    extern void * __real__malloc_r(size_t nbytes);
    if(!inside_malloc) {
      MallocCallCnt++;
      TotalMallocdBytes += nbytes;
    };
    void *p = __real__malloc_r(nbytes);
    return p;
  };
#endif


typedef struct malloc_chunk {
  long size;
  struct malloc_chunk* next;
} chunk;
extern chunk* __malloc_free_list;

double newlib_malloc_fragmentation() {
  vTaskSuspendAll();

  long free_total = heapBytesRemaining;
  long free_max = 0;

  for (chunk* p = __malloc_free_list; p; p = p->next) {
    free_total += p->size;
    if (p->size > free_max) {
      free_max = p->size;
    }
  }

  for (size_t i = 0; i < sizeof(heapRegion)/sizeof(*heapRegion); i++) {
    long unused_region = heapRegion[i].limit - heapRegion[i].sbrk;
    if (unused_region > free_max) {
      free_max = unused_region;
    }
  }

  (void)xTaskResumeAll();

  return (free_total == 0 ? 0 : 1 - (free_max / (double)free_total));
}


// ================================================================================================
// Implement FreeRTOS's memory API using newlib-provided malloc family.
// ================================================================================================

void *pvPortMalloc( size_t xSize ) PRIVILEGED_FUNCTION {
  void *pvReturn = malloc(xSize);
  traceMALLOC(pvReturn, xSize);

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  if (pvReturn == NULL) {
    extern void vApplicationMallocFailedHook(void);
    vApplicationMallocFailedHook();
  }
#endif

  return pvReturn;
}
void vPortFree( void *pv ) PRIVILEGED_FUNCTION {
  if (pv) {
    free(pv);
    traceFREE(pv, 0);
  }
};

size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION {
    struct mallinfo mi = mallinfo(); // available space now managed by newlib
    return mi.fordblks + heapBytesRemaining; // plus space not yet handed to newlib by sbrk
}

// GetMinimumEverFree is not available in newlib's malloc implementation.
// So, no implementation is provided: size_t xPortGetMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;

//! No implementation needed, but stub provided in case application already calls vPortInitialiseBlocks
void vPortInitialiseBlocks( void ) PRIVILEGED_FUNCTION {};
