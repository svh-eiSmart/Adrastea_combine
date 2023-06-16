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

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "DRV_FLASH.h"
#include "kvpfs_api.h"

int do_setenv(const char *param) {
  char *pch;
  pch = strstr(param, " ");
  int varname_len, varvalue_len;
  char *varname, *varvalue;

  int32_t ret;
  if (pch == NULL) pch = (char *)(param + strlen(param));  // set pch to the end of string

  varname_len = pch - param;
  varvalue_len = param + strlen(param) - pch;
  varname = (char *)pvPortMalloc(varname_len + 1);
  varvalue = (char *)pvPortMalloc(varvalue_len + 1);
  memset(varname, 0, varname_len + 1);
  memset(varvalue, 0, varvalue_len + 1);

  strncpy(varname, param, varname_len);
  strncpy(varvalue, pch + 1, varvalue_len);

  if (varvalue_len == 0)
    ret = set_env(varname, NULL);
  else
    ret = set_env(varname, varvalue);

  switch (ret) {
    case -KVPFS_UNINITIALIZED:
      printf("%s\n", "kvpfs uninitialized");
      break;
    case -KVPFS_NULL_POINTER:
      printf("%s\n", "NULL pointer");
      break;
    case -KVPFS_RESERVE_KEYWD:
      printf("%s\n", "reserved CRC keyword");
      break;
    case -KVPFS_OUT_OF_MEM:
      printf("%s\n", "out of mem");
      break;
    case -KVPFS_KEY_NOT_FOUND:
      printf("%s\n", "key not found");
      break;
    case 0:
      printf("%s\n", "OK");
      break;
    default:
      printf("unknown error. ret=%ld\n", ret);
      break;
  }
  vPortFree(varname);
  vPortFree(varvalue);
  return 0;
}
int do_fss(char *name) {
  kvpfs_fss();
  return 0;
}
int do_getenv(char *name) {
  char *value = NULL;
  int32_t ret = 0;

  if (*name) {
    ret = get_env(name, &value);

    if (value) printf("%s = %s\n", name, value);
  } else {
    ret = get_env(NULL, &value);
    if (value) printf("%s\n", value);
  }

  switch (ret) {
    case -KVPFS_UNINITIALIZED:
      printf("%s\n", "kvpfs uninitialized");
      break;
    case -KVPFS_KEY_NOT_FOUND:
      printf("%s\n", "key not found");
      break;
    case 0:
      printf("%s\n", "OK");
      break;
    default:
      printf("unknown error. ret=%ld\n", ret);
      break;
  }

  vPortFree(value);

  return 0;
}

int do_saveenv(char *name) {
  int32_t ret;
  ret = save_env();
  switch (ret) {
    case -KVPFS_UNINITIALIZED:
      printf("%s\n", "kvpfs uninitialized");
      break;
    case -KVPFS_NULL_POINTER:
      printf("%s\n", "NULL pointer");
      break;
    case -KVPFS_RESERVE_KEYWD:
      printf("%s\n", "reserved CRC keyword");
      break;
    case -KVPFS_OUT_OF_MEM:
      printf("%s\n", "out of mem");
      break;
    case -KVPFS_KEY_NOT_FOUND:
      printf("%s\n", "key not found");
      break;
    case -KVPFS_FLASH_SPACE_NOT_ENOUGH:
      printf("%s\n", "space not enough");
      break;
    case 0:
      printf("%s\n", "OK");
      break;
    default:
      printf("unknown error. ret=%ld\n", ret);
      break;
  }
  return 0;
}
