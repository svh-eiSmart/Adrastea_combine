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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_APIUTIL_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_APIUTIL_H

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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define altcom_lock()              \
  do {                             \
    altcom_sys_disable_dispatch(); \
  } while (0)
#define altcom_unlock()           \
  do {                            \
    altcom_sys_enable_dispatch(); \
  } while (0)

#define ALTCOM_REG_CALLBACK(ret, dst_callback, src_callback, dst_priv, src_priv) \
  do {                                                                           \
    altcom_callback_lock();                                                      \
    if (dst_callback) {                                                          \
      ret = -EBUSY;                                                              \
    } else {                                                                     \
      dst_callback = src_callback;                                               \
      dst_priv = src_priv;                                                       \
      ret = 0;                                                                   \
    }                                                                            \
    altcom_callback_unlock();                                                    \
  } while (0)

#define ALTCOM_GET_AND_CLR_CALLBACK(ret, src_callback, dst_callback, src_priv, dst_priv) \
  do {                                                                                   \
    altcom_callback_lock();                                                              \
    if (!src_callback) {                                                                 \
      ret = -EPERM;                                                                      \
    } else {                                                                             \
      dst_callback = src_callback;                                                       \
      src_callback = NULL;                                                               \
      dst_priv = src_priv;                                                               \
      src_priv = NULL;                                                                   \
      ret = 0;                                                                           \
    }                                                                                    \
    altcom_callback_unlock();                                                            \
  } while (0)

#define ALTCOM_CLR_CALLBACK(tgt_callback, tgt_priv) \
  do {                                              \
    altcom_callback_lock();                         \
    tgt_callback = NULL;                            \
    tgt_priv = NULL;                                \
    altcom_callback_unlock();                       \
  } while (0)

#define ALTCOM_GET_CALLBACK(src_callback, dst_callback, src_userpriv, dst_userpriv) \
  do {                                                                              \
    altcom_callback_lock();                                                         \
    dst_callback = src_callback;                                                    \
    dst_userpriv = src_userpriv;                                                    \
    altcom_callback_unlock();                                                       \
  } while (0)

#define ALTCOM_SOCK_ALLOC_CMDBUFF(buff, id, len) (((buff) = altcom_alloc_cmdbuff(id, len)) != NULL)

#define ALTCOM_SOCK_ALLOC_RESBUFF(buff, len) ((buff = altcom_alloc_resbuff(len)) != NULL)

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern bool g_altcom_initialized;
extern bool g_altcom_cbreg_only;
extern altcom_sys_mutex_t g_altcom_apicallback_mtx;

/****************************************************************************
 * Inline functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_callback_createlock
 *
 * Description:
 *   Create the lock for access to the API callback.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static inline void altcom_callback_createlock(void) {
  int32_t ret;
  altcom_sys_cremtx_s param = {0};

  ret = altcom_sys_create_mutex(&g_altcom_apicallback_mtx, &param);
  if (ret < 0) {
    DBGIF_LOG1_ERROR("Failed to create mutex :%ld\n", ret);
  }
}

/****************************************************************************
 * Name: altcom_callback_deletelock
 *
 * Description:
 *   Delete the lock for access to the API callback.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static inline void altcom_callback_deletelock(void) {
  int32_t ret;

  ret = altcom_sys_delete_mutex(&g_altcom_apicallback_mtx);
  if (ret < 0) {
    DBGIF_LOG1_ERROR("Failed to delete mutex :%ld\n", ret);
  }
}

static inline int32_t altcom_check_initialized_and_set(void) {
  int32_t ret;

  altcom_lock();
  if (g_altcom_initialized) {
    ret = -EBUSY;
  } else {
    g_altcom_initialized = true;
    altcom_callback_createlock();
    ret = 0;
  }
  altcom_unlock();

  return ret;
}

static inline void altcom_set_initialized(void) {
  altcom_lock();
  g_altcom_initialized = true;
  altcom_callback_createlock();
  altcom_unlock();
}

static inline int32_t altcom_check_finalized_and_set(void) {
  int32_t ret;

  altcom_lock();
  if (!g_altcom_initialized) {
    ret = -EPERM;
  } else {
    g_altcom_initialized = false;
    altcom_callback_deletelock();
    ret = 0;
  }
  altcom_unlock();

  return ret;
}

static inline void altcom_set_finalized(void) {
  altcom_lock();
  g_altcom_initialized = false;
  altcom_callback_deletelock();
  altcom_unlock();
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
void altcom_callback_lock(void);
void altcom_callback_unlock(void);
bool altcom_isinit(void);
void altcom_free_cmd(FAR uint8_t *dat);
int32_t altcom_send_and_free(FAR uint8_t *dat);
void *altcom_alloc_cmdbuff(int32_t cmdid, uint16_t len);
void *altcom_alloc_resbuff(uint16_t len);
void altcom_sock_free_cmdandresbuff(FAR void *cmdbuff, FAR void *resbuff);
bool altcom_sock_alloc_cmdandresbuff(FAR void **buff, int32_t id, uint16_t bufflen, FAR void **res,
                                     uint16_t reslen);
void altcom_generic_free_cmdandresbuff(FAR void *cmdbuff, FAR void *resbuff);
bool altcom_generic_alloc_cmdandresbuff(FAR void **buff, int32_t id, uint16_t bufflen,
                                        FAR void **res, uint16_t reslen);
bool altcom_isCbRegOnly(void);
void altcom_SetCallbackReg(bool cbreg_only);
#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_APIUTIL_H */
