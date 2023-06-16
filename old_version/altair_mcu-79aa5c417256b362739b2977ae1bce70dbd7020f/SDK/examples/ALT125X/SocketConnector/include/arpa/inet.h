/****************************************************************************
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

/* Stub header for socket test */

#ifndef __ALTCOM_TEST_INET_H
#define __ALTCOM_TEST_INET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_inet.h"

#include "sys/socket.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*  Length of the string form for IP address (excludes NULL termination) */

#define INET_ADDRSTRLEN ALTCOM_INET_ADDRSTRLEN

/*  Length of the string form for IPv6 address (excludes NULL termination) */

#define INET6_ADDRSTRLEN ALTCOM_INET6_ADDRSTRLEN

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef altcom_in_addr_t in_addr_t;
#define in_addr altcom_in_addr
#define in6_addr altcom_in6_addr
#define s6_addr altcom_s6_addr

extern const struct in6_addr in6addr_any;

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_htonl
 *
 * Description:
 *   Convert from host byte order to network byte order.
 *
 * Input parameters:
 *   x - unsigned long value
 *
 *  Return:
 *   Value converted to network byte order
 *
 ****************************************************************************/

#define htonl(x) altcom_htonl(x)

/****************************************************************************
 * Name: altcom_htons
 *
 * Description:
 *   Convert from host byte order to network byte order.
 *
 * Input parameters:
 *   x - unsigned short value
 *
 *  Return:
 *   Value converted to network byte order
 *
 ****************************************************************************/

#define htons(x) altcom_htons(x)

/****************************************************************************
 * Name: altcom_ntohl
 *
 * Description:
 *   Convert from network byte order to host byte order.
 *
 * Input parameters:
 *   x - unsigned long value
 *
 *  Return:
 *   Value converted to host byte order
 *
 ****************************************************************************/

#define ntohl(x) altcom_ntohl(x)

/****************************************************************************
 * Name: altcom_ntohs
 *
 * Description:
 *   Convert from network byte order to host byte order.
 *
 * Input parameters:
 *   x - unsigned short value
 *
 *  Return:
 *   Value converted to host byte order
 *
 ****************************************************************************/

#define ntohs(x) altcom_ntohs(x)

/****************************************************************************
 * Name: altcom_inet_addr
 *
 * Description:
 *   Converts the string pointed to by cp, in the standard IPv4 dotted
 *   decimal notation, to an integer value suitable for use as an Internet
 *   address.
 *
 * Input parameters:
 *   cp - Pointer to string in the standard IPv4 dotted decimal notation
 *
 *  Return:
 *   Internet address
 *
 ****************************************************************************/

#define inet_addr(cp) altcom_inet_addr(cp)

/****************************************************************************
 * Name: altcom_inet_ntoa
 *
 * Description:
 *   Converts the Internet host address given in network byte order to a
 *   string in standard numbers-and-dots notation. The string is returned
 *   in a statically allocated buffer, which subsequent calls will overwrite.
 *
 * Input parameters:
 *   addr - Internet address
 *
 *  Return:
 *   Pointer to string in the standard IPv4 dotted decimal notation
 *
 ****************************************************************************/

#define inet_ntoa(addr) altcom_inet_ntoa(addr)

/****************************************************************************
 * Name: altcom_inet_ntop
 *
 * Description:
 *  The altcom_inet_ntop() function converts a numeric address into a text
 *  string suitable for presentation.
 *
 * Input Parameters:
 *   af   - The af argument specifies the family of the address. This can be
 *          AF_INET or AF_INET6.
 *   src  - The src argument points to a buffer holding an address of the
 *          specified type.  The address must be in network byte order.
 *   dst  - The dest argument points to a buffer where the function stores
 *          the resulting text string; it shall not be NULL.
 *   size - The size argument specifies the size of this buffer, which must
 *          be large enough to hold the text string (ALTCOM_INET_ADDRSTRLEN
 *          characters for IPv4, ALTCOM_INET6_ADDRSTRLEN characters for
 *          IPv6).
 *
 * Returned Value:
 *   altcom_inet_ntop() returns a pointer to the buffer containing the text
 *   string if the conversion succeeds. Otherwise, NULL is returned and the
 *   errno is set to indicate the error.
 *   There follow errno values may be set:
 *
 *   EAFNOSUPPORT - The af argument is invalid.
 *   ENOSPC - The size of the inet_ntop() result buffer is inadequate
 *
 ****************************************************************************/

#define inet_ntop(af, src, dst, size) altcom_inet_ntop(af, src, dst, size)

/****************************************************************************
 * Name: altcom_inet_pton
 *
 * Description:
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
 * Input Parameters:
 *   af   - The af argument specifies the family of the address. This can be
 *          ALTCOM_AF_INET or ALTCOM_AF_INET6.
 *   src  - The src argument points to the string being passed in.
 *   dst  - The dest argument points to memory into which the function stores
 *          the numeric address; this must be large enough to hold the numeric
 *          address (32 bits for ALTCOM_AF_INET, 128 bits for
 *          ALTCOM_AF_INET6).
 *
 * Returned Value:
 *   The altcom_inet_pton() function returns 1 if the conversion succeeds,
 *   with the address pointed to by dest in network byte order. It will
 *   return 0 if the input is not a valid IPv4 dotted-decimal string or
 *   a valid IPv6 address string, or -1 with errno set to EAFNOSUPPORT
 *   if the af argument is unknown.
 *
 ****************************************************************************/

#define inet_pton(af, src, dst) altcom_inet_pton(af, src, dst)

/****************************************************************************
 * Name: altcom_inet_aton
 *
 * Description:
 *   Converts the address cp of the Internet host from IPv4 value and dot
 *   notation to binary value (network byte order) and stores the conversion
 *   result in the structure pointed to by inp.
 *   Returns nonzero if the address is valid, 0 if not.
 *
 * Input parameters:
 *   cp - Pointer to string in the standard IPv4 dotted decimal notation
 *   inp - Pointer to conversion result
 *
 *  Return:
 *   nonzero if the address is valid, 0 if not.
 *
 ****************************************************************************/

#define inet_aton(cp, inp) altcom_inet_aton(cp, inp)

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ALTCOM_TEST_INET_H */
