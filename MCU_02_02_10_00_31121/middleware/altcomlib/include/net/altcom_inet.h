/****************************************************************************
 * modules/lte/include/net/altcom_inet.h
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

/**
 * @file altcom_inet.h
 */

#ifndef __MODULES_LTE_INCLUDE_NET_ALTCOM_INET_H
#define __MODULES_LTE_INCLUDE_NET_ALTCOM_INET_H

/**
 * @defgroup net NET Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include "altcom_socket.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*  Length of the string form for IP address (excludes NULL termination) */

#define ALTCOM_INET_ADDRSTRLEN 16

/*  Length of the string form for IPv6 address (excludes NULL termination) */

#define ALTCOM_INET6_ADDRSTRLEN 46

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @typedef altcom_in_addr_t
 * Definition of IPv4 IP address.
 */

typedef uint32_t altcom_in_addr_t;

/**
 * @struct altcom_in_addr
 * Definition of IPv4 IP address wrapper.
 */

struct altcom_in_addr {
  altcom_in_addr_t s_addr;
};

/**
 * @struct altcom_in6_addr
 * Definition of IPv6 IP address wrapper.
 */

struct altcom_in6_addr {
  union {
    uint32_t u32_addr[4];
    uint16_t u16_addr[8];
    uint8_t u8_addr[16];
  } un;
#define altcom_s6_addr un.u8_addr
};

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup inet_funcs INET APIs
 * @{
 */

/**
 * Name: altcom_htonl
 *
 *   Convert from host byte order to network byte order.
 *
 *   @param [in] x - unsigned long value
 *
 *   @return Value converted to network byte order
 *
 */

uint32_t altcom_htonl(uint32_t x);

/**
 * Name: altcom_htons
 *
 *   Convert from host byte order to network byte order.
 *
 *   @param [in] x - unsigned short value
 *
 *   @return Value converted to network byte order
 *
 */

uint16_t altcom_htons(uint16_t x);

/**
 * Name: altcom_ntohl
 *
 *   Convert from network byte order to host byte order.
 *
 *   @param [in] x - unsigned long value
 *
 *   @return Value converted to host byte order
 *
 */

uint32_t altcom_ntohl(uint32_t x);

/**
 * Name: altcom_ntohs
 *
 *   Convert from network byte order to host byte order.
 *
 *   @param [in] x - unsigned short value
 *
 *   @return Value converted to host byte order
 *
 */

uint16_t altcom_ntohs(uint16_t x);

/**
 * Name: altcom_inet_addr
 *
 *   Converts the string pointed to by cp, in the standard IPv4 dotted
 *   decimal notation, to an integer value suitable for use as an Internet
 *   address.
 *
 *   @param [in] cp - Pointer to string in the standard IPv4 dotted decimal notation
 *
 *   @return Internet address
 *
 */

altcom_in_addr_t altcom_inet_addr(const char *cp);

/**
 * Name: altcom_inet_ntoa
 *
 *   Converts the Internet host address given in network byte order to a
 *   string in standard numbers-and-dots notation. The string is returned
 *   in a statically allocated buffer, which subsequent calls will overwrite.
 *
 *   @param [in] addr - Internet address
 *
 *   @return Pointer to string in the standard IPv4 dotted decimal notation
 *
 */

char *altcom_inet_ntoa(struct altcom_in_addr addr);

/**
 * Name: altcom_inet_ntop
 *
 *  The altcom_inet_ntop() function converts a numeric address into a text
 *  string suitable for presentation.
 *
 *   @param [in] af   - The af argument specifies the family of the address. This can be
 *          AF_INET or AF_INET6.
 *   @param [in] src  - The src argument points to a buffer holding an address of the
 *          specified type.  The address must be in network byte order.
 *   @param [out] dst  - The dest argument points to a buffer where the function stores
 *          the resulting text string; it shall not be NULL.
 *   @param [in] size - The size argument specifies the size of this buffer, which must
 *          be large enough to hold the text string (ALTCOM_INET_ADDRSTRLEN
 *          characters for IPv4, ALTCOM_INET6_ADDRSTRLEN characters for
 *          IPv6).
 *
 *   @return  altcom_inet_ntop() returns a pointer to the buffer containing the text
 *   string if the conversion succeeds. Otherwise, NULL is returned and the
 *   errno is set to indicate the error.
 *   There follow errno values may be set:
 *
 *   EAFNOSUPPORT - The af argument is invalid.
 *   ENOSPC - The size of the inet_ntop() result buffer is inadequate
 *
 */

const char *altcom_inet_ntop(int af, const void *src, char *dst, altcom_socklen_t size);

/**
 * Name: altcom_inet_pton
 *
 *  The altcom_inet_pton() function converts an address in its standard text
 *  presentation form into its numeric binary form.
 *
 *  If the af argument of altcom_inet_pton() is ALTCOM_AF_INET, the src
 *  string will be in the standard IPv4 dotted-decimal form:
 *
 *    ddd.ddd.ddd.ddd
 *
 *  where "ddd" is a one to three digit decimal number between 0 and 255.
 *
 *  If the af argument of altcom_inet_pton() is ALTCOM_AF_INET6, the src
 *  string will be in one of the following standard IPv6 text forms:
 *
 *  1. The preferred form is "x:x:x:x:x:x:x:x", where the 'x' s are the
 *     hexadecimal values of the eight 16-bit pieces of the address. Leading
 *     zeros in individual fields can be omitted, but there must be at least
 *     one numeral in every field.
 *
 *  2. A string of contiguous zero fields in the preferred form can be shown
 *     as "::". The "::" can only appear once in an address. Unspecified
 *     addresses ( "0:0:0:0:0:0:0:0" ) may be represented simply as "::".
 *
 *  3. A third form that is sometimes more convenient when dealing with a
 *     mixed environment of IPv4 and IPv6 nodes is "x:x:x:x:x:x:d.d.d.d",
 *     where the 'x' s are the hexadecimal values of the six high-order
 *     16-bit pieces of the address, and the 'd' s are the decimal values
 *     of the four low-order 8-bit pieces of the address (standard IPv4
 *     representation).
 *
 *   @param [in] af   - The af argument specifies the family of the address. This can be
 *          ALTCOM_AF_INET or ALTCOM_AF_INET6.
 *   @param [in] src  - The src argument points to the string being passed in.
 *   @param [out] dst  - The dest argument points to memory into which the function stores
 *          the numeric address; this must be large enough to hold the numeric
 *          address (32 bits for ALTCOM_AF_INET, 128 bits for
 *          ALTCOM_AF_INET6).
 *
 *   @return The altcom_inet_pton() function returns 1 if the conversion succeeds,
 *   with the address pointed to by dest in network byte order. It will
 *   return 0 if the input is not a valid IPv4 dotted-decimal string or
 *   a valid IPv6 address string, or -1 with errno set to EAFNOSUPPORT
 *   if the af argument is unknown.
 *
 */

int altcom_inet_pton(int af, const char *src, void *dst);

/**
 * Name: altcom_inet_aton
 *
 *   Converts the address cp of the Internet host from IPv4 value and dot
 *   notation to binary value (network byte order) and stores the conversion
 *   result in the structure pointed to by inp.
 *   Returns nonzero if the address is valid, 0 if not.
 *
 *   @param [in] cp - Pointer to string in the standard IPv4 dotted decimal notation
 *   @param [out] inp - Pointer to conversion result
 *
 *   @return nonzero if the address is valid, 0 if not.
 *
 */

int altcom_inet_aton(const char *cp, struct altcom_in_addr *inp);

/** @} inet_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} net */

#endif /* __MODULES_LTE_INCLUDE_NET_ALTCOM_INET_H */
