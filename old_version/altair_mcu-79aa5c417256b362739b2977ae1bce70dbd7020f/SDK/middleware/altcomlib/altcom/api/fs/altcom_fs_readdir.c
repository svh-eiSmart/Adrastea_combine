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

#include "altcom_fs.h"
#include "apicmd_fs.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_FS_READDIR_DATA_LEN (sizeof(struct apicmd_cmddat_fs_readdir_s))
#define ALTCOM_FS_READDIR_RES_DATA_LEN (sizeof(struct apicmd_cmddat_fs_readdirres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

altcom_fs_err_t altcom_fs_readdir(altcom_fs_dir_t dir, altcom_fs_dirent_t *dirent) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_fs_readdir_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_fs_readdirres_s *resbuff = NULL;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return FS_ERR_NOINIT;
  }

  /* Check parameter */

  if (!dirent) {
    DBGIF_LOG_ERROR("Invalid parameter\n");
    return FS_ERR_INVALIDPARAM;
  }

  /* Allocate API command buffer to send */

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_FS_READDIR,
                                         ALTCOM_FS_READDIR_DATA_LEN, (FAR void **)&resbuff,
                                         ALTCOM_FS_READDIR_RES_DATA_LEN)) {
    cmdbuff->dir_handle = htonl(dir);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return FS_ERR_NOMEM;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff,
                      ALTCOM_FS_READDIR_RES_DATA_LEN, &reslen, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
    ret = FS_ERR_REQERR;
  } else if (ALTCOM_FS_READDIR_RES_DATA_LEN != reslen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = FS_ERR_REQERR;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG2_INFO("%s[%d]\n", __FUNCTION__, ret);
    if (ret == 0) {
      dirent->d_type = (altcom_fs_type_t)resbuff->dirent.ent_type;
      DBGIF_ASSERT(dirent->d_type < FS_TYPE_MAX, "Invalid d_type %lu", (uint32_t)dirent->d_type);

      snprintf(dirent->d_name, ALTCOM_FS_PATH_MAXLEN, "%s", resbuff->dirent.ent_name);
    } else {
      if ((altcom_fs_err_t)ntohl(resbuff->errcode) != FS_ERR_NOMOREENTRY) {
        DBGIF_LOG2_ERROR("%s ret %ld\n", __FUNCTION__, ret);
        ret = ntohl(resbuff->errcode);
        DBGIF_LOG2_ERROR("%s errcode %ld\n", __FUNCTION__, ret);
      } else {
        ret = ntohl(resbuff->errcode);
      }
    }
  }

  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);
  return (altcom_fs_err_t)ret;
}
