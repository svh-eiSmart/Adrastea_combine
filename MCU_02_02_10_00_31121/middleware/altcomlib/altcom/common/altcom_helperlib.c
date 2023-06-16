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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>

#include "dbg_if.h"
#include "buffpoolwrapper.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int altcom_helperlib_conv_dot2colon_v6(const char *src, char *dst) {
  if (!src || !dst) {
    return -1;
  }

  size_t srclen = strlen(src);

  if (!srclen) {
    return -1;
  }

  char *src_cpy, *src_tmp, *saveptr;
  int i, parse_cnt;

  src_cpy = BUFFPOOL_ALLOC(srclen + 1);
  DBGIF_ASSERT(src_cpy, "BUFFPOOL_ALLOC fail\n");

  memcpy((void *)src_cpy, (void *)src, srclen + 1);
  parse_cnt = 8; /* 15 dots and parse twice in the loop */

  for (i = 0, src_tmp = src_cpy; i < parse_cnt; i++) {
    char *msb, *lsb, msb_lsb[6]; /* ':' + 4digit + '\0' */

    msb = strtok_r(src_tmp, ".", &saveptr);
    if (msb == NULL) {
      break;
    }

    src_tmp = NULL;
    lsb = strtok_r(src_tmp, ".", &saveptr);
    if (lsb == NULL) {
      break;
    }

    int j;
    msb_lsb[0] = (i == 0 ? '\0' : ':');
    j = snprintf((i == 0 ? &msb_lsb[0] : &msb_lsb[1]), sizeof(msb_lsb), "%02X%02X",
                 (unsigned)strtoul(msb, NULL, 10), (unsigned)strtoul(lsb, NULL, 10));
    if (j != 4) {
      BUFFPOOL_FREE(src_cpy);
      return -1;
    }

    strncat(dst, msb_lsb, (i == 0 ? 6 : 5));
  }

  BUFFPOOL_FREE(src_cpy);
  return i == parse_cnt ? 0 : -1;
}
