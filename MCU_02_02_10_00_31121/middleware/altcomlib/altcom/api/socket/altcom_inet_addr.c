/****************************************************************************
 * modules/lte/altcom/api/socket/altcom_inet_addr.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "dbg_if.h"
#include "altcom_inet.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

altcom_in_addr_t altcom_inet_addr(const char *cp) {
  unsigned int addr[4] = {0};
  uint32_t result;
  char cpstr[ALTCOM_INET_ADDRSTRLEN];

  memset(cpstr, 0, sizeof(cpstr));
  strncpy(cpstr, cp, sizeof(cpstr) - 1);

  char *strgp, *tk;
  int i;

  strgp = cpstr;
  result = 0;
  for (i = 0; i < 4; i++) {
    tk = strsep(&strgp, ".");
    if (tk) {
      addr[i] = strtoul(tk, NULL, 10);
      result |= addr[i];
      if (i < 3) {
        result <<= 8;
      }
    } else {
      DBGIF_LOG1_ERROR("Incorrect string format of ipv4 addr \"%s\"\r\n", cp);
    }
  }

  return altcom_htonl(result);
}
