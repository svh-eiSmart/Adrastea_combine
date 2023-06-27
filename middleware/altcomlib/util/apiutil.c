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
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>

#include "altcom_osal.h"
#include "thrdpool.h"
#include "dbg_if.h"
#include "apicmdgw.h"
#include "altcom_errno.h"
#include "altcom_seterrno.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"

/****************************************************************************
 * Public functions
 ****************************************************************************/
void altcom_callback_lock(void) { altcom_sys_lock_mutex(&g_altcom_apicallback_mtx); }

void altcom_callback_unlock(void) { altcom_sys_unlock_mutex(&g_altcom_apicallback_mtx); }

bool altcom_isinit(void) {
  bool is_init;

  altcom_lock();
  is_init = g_altcom_initialized;
  altcom_unlock();

  return is_init;
}

void altcom_free_cmd(FAR uint8_t *dat) {
  int32_t freeret = apicmdgw_freebuff(dat);
  if (freeret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_freebuff() failure. ret:%ld\n", freeret);
  }
}

int32_t altcom_send_and_free(FAR uint8_t *dat) {
  int32_t ret = APICMDGW_SEND_ONLY(dat);
  if (ret < 0) {
    DBGIF_LOG1_ERROR("Failed to send API command. err=[%ld]\n", ret);
  }

  altcom_free_cmd(dat);

  return ret;
}

void *altcom_alloc_cmdbuff(int32_t cmdid, uint16_t len) {
  FAR void *buff = NULL;

  buff = apicmdgw_cmd_allocbuff(cmdid, len);
  if (!buff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
  }
  return buff;
}

void *altcom_alloc_resbuff(uint16_t len) {
  FAR void *res = NULL;

  res = BUFFPOOL_ALLOC(len);
  if (!res) {
    DBGIF_LOG_ERROR("Failed to allocate response buffer.\n");
    altcom_seterrno((int32_t)ALTCOM_ENOMEM);
  }

  return res;
}

void altcom_sock_free_cmdandresbuff(FAR void *cmdbuff, FAR void *resbuff) {
  if (cmdbuff) {
    altcom_free_cmd((FAR uint8_t *)cmdbuff);
  }
  if (resbuff) {
    (void)BUFFPOOL_FREE(resbuff);
  }
}

bool altcom_sock_alloc_cmdandresbuff(FAR void **buff, int32_t id, uint16_t bufflen, FAR void **res,
                                     uint16_t reslen) {
  if (!ALTCOM_SOCK_ALLOC_CMDBUFF(*buff, id, bufflen)) {
    altcom_seterrno((int32_t)ALTCOM_ENOMEM);
    return false;
  }

  if (!ALTCOM_SOCK_ALLOC_RESBUFF(*res, reslen)) {
    altcom_free_cmd((FAR uint8_t *)*buff);
    altcom_seterrno((int32_t)ALTCOM_ENOMEM);
    return false;
  }

  return true;
}

void altcom_generic_free_cmdandresbuff(FAR void *cmdbuff, FAR void *resbuff) {
  if (cmdbuff) {
    altcom_free_cmd((FAR uint8_t *)cmdbuff);
  }
  if (resbuff) {
    (void)BUFFPOOL_FREE(resbuff);
  }
}

bool altcom_generic_alloc_cmdandresbuff(FAR void **buff, int32_t id, uint16_t bufflen,
                                        FAR void **res, uint16_t reslen) {
  if (!ALTCOM_SOCK_ALLOC_CMDBUFF(*buff, id, bufflen)) {
    return false;
  }

  if (!ALTCOM_SOCK_ALLOC_RESBUFF(*res, reslen)) {
    altcom_free_cmd((FAR uint8_t *)*buff);
    return false;
  }

  return true;
}

bool altcom_isCbRegOnly(void) {
  bool isCbRegOnly;
  altcom_callback_lock();
  isCbRegOnly = g_altcom_cbreg_only;
  altcom_callback_unlock();
  DBGIF_LOG1_DEBUG("isCbRegOnly: %d\n", (int)isCbRegOnly);
  return isCbRegOnly;
}

void altcom_SetCallbackReg(bool cbreg_only) {
  altcom_callback_lock();
  g_altcom_cbreg_only = cbreg_only;
  altcom_callback_unlock();
}