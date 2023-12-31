/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 * Author: Stefano Oliveri <stefano.oliveri@st.com>
 *
 */
#ifndef __CC_H__
#define __CC_H__

#include <stdlib.h>

#define LWIP_PROVIDE_ERRNO 1
#include "lwip_errno.h"
#include "sys_arch.h"  //FSL

#if __GNUC__
#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>
#endif

#undef LWIP_NOASSERT  // FSL
//#define LWIP_NOASSERT//FSL

// Typedefs for the types used by lwip

typedef unsigned char u8_t;
typedef signed char s8_t;
typedef unsigned short u16_t;
typedef signed short s16_t;
typedef unsigned long u32_t;
typedef signed long s32_t;
typedef u32_t mem_ptr_t;
typedef int sys_prot_t;

typedef void (*sendToPPP_fn)(u8_t *s, int l);

// Compiler hints for packing lwip's structures
// FSL: very important at high optimization level
/* */
#if __GNUC__
#define PACK_STRUCT_BEGIN
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_BEGIN _Pragma("pack(2)")
#elif defined(__CWCC__)
#error "Need CW reserved word"
#else
#error "No compiler defined"
#endif

#if __GNUC__
#define PACK_STRUCT_STRUCT __attribute__((__packed__))
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_STRUCT
#elif defined(__CWCC__)
#error "Need CW reserved word"
#else
#error "No compiler defined"
#endif

#if __GNUC__
#define PACK_STRUCT_END
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACK_STRUCT_END _Pragma("pack()")
#elif defined(__CWCC__)
#error "Need CW reserved word"
#else
#error "No compiler defined"
#endif

#define PACK_STRUCT_FIELD(x) x

// Platform specific diagnostic output

// non-fatal, print a message.
#define LWIP_PLATFORM_DIAG(x) sys_debug(x)
// fatal, print message and abandon execution.
#define LWIP_PLATFORM_ASSERT(x) sys_assert(x)
#define LWIP_RAND() ((u32_t)rand())

// "lightweight" synchronization mechanisms

#if 0
// declare a protection state variable.
#define SYS_ARCH_DECL_PROTECT(x)
// enter protection mode.
#define SYS_ARCH_PROTECT(x)
// leave protection mode.
#define SYS_ARCH_UNPROTECT(x)
#endif

#endif /* __CC_H__ */
